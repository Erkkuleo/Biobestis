#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>
#include "eye_display.h"
#include "data_display.h"
#include "art/bitmaps.h"
#include "../shared/protocol.h"

const int led = D10;
const int resetBtn = D1;

Preferences prefs;

int fullnessamount = 0;
int humidity = 65;

static volatile bool newData = false;
static SensorPacket pendingPkt;

void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (len < (int)sizeof(SensorPacket)) return;
    SensorPacket pkt;
    memcpy(&pkt, data, sizeof(pkt));
    if (!packetValid(pkt)) return;
    memcpy(&pendingPkt, &pkt, sizeof(pkt));
    newData = true;
}
unsigned long lastDataSwitch = 0;
unsigned long lastDayCheck = 0;
int dataState = 0;  // 0=fullness, 1=humidity, 2=emptied days


void saveDaysSinceEmpty(uint16_t days) {
    prefs.putUShort("days", days);
}

int getDaysSinceEmpty() {
    return (int)prefs.getUShort("days", 0);
}

static int lastEyeSegment = -1;

void updateEyeStatus() {
    int segFullness  = constrain(fullnessamount / 20, 0, 5);
    int segHumidity  = constrain((humidity - 30) / 14, 0, 5);
    int segDays      = constrain(getDaysSinceEmpty(), 0, 5);
    int worstSegment = max({segFullness, segHumidity, segDays});
    if (worstSegment == lastEyeSegment) return;
    lastEyeSegment = worstSegment;
    eyeCtrl.drawBitmap((const unsigned char*)pgm_read_ptr(&statusBitmaps[worstSegment]));
}

void drawCurrentDataScreen() {
    if (dataState == 0) {
        dataCtrl.drawTextWithProgress("Fullness state", fullnessamount);
    } else if (dataState == 1) {
        char buf[20];
        snprintf(buf, sizeof(buf), "Humidity: %d %%", humidity);
        dataCtrl.drawCenteredText(buf, 2);
    } else {
        char buf[24];
        snprintf(buf, sizeof(buf), "Emptied %d days ago", getDaysSinceEmpty());
        dataCtrl.drawCenteredText(buf, 2);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    pinMode(resetBtn, INPUT_PULLUP);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
    } else {
        esp_now_register_recv_cb(onReceive);
        Serial.println("ESP-NOW ready");
    }

    prefs.begin("kayttiselain", false);

    eyeCtrl.init();
    updateEyeStatus();

    dataCtrl.init();
    dataCtrl.test();
    drawCurrentDataScreen();
}

void handleSerial() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();

    line.toLowerCase();

    if (line.startsWith("humidity ")) {
        humidity = line.substring(9).toInt();
        Serial.print("Humidity set to ");
        Serial.println(humidity);
        if (dataState == 1) drawCurrentDataScreen();
        updateEyeStatus();
    } else if (line.startsWith("fullness ")) {
        fullnessamount = constrain(line.substring(9).toInt(), 0, 100);
        Serial.print("Fullness set to ");
        Serial.println(fullnessamount);
        if (dataState == 0) drawCurrentDataScreen();
        updateEyeStatus();
    } else if (line == "day" || line.startsWith("day ")) {
        int days = (line == "day") ? getDaysSinceEmpty() + 1 : line.substring(4).toInt();
        saveDaysSinceEmpty((uint16_t)constrain(days, 0, 65535));
        Serial.print("Days since empty set to ");
        Serial.println(days);
        if (dataState == 2) drawCurrentDataScreen();
        updateEyeStatus();
    } else if (line == "status") {
        Serial.printf("Days since empty: %d\n", getDaysSinceEmpty());
        Serial.printf("Fullness: %d  Humidity: %d\n", fullnessamount, humidity);
    }
}

void loop() {
    if (newData) {
        newData = false;
        bool fullnessChanged = (pendingPkt.fullness != fullnessamount);
        bool humidityChanged = (pendingPkt.humidity  != humidity);
        fullnessamount = pendingPkt.fullness;
        humidity       = pendingPkt.humidity;
        if (fullnessChanged || humidityChanged) {
            updateEyeStatus();
        }
        bool affectsScreen = (dataState == 0 && fullnessChanged) ||
                             (dataState == 1 && humidityChanged);
        if (affectsScreen) {
            drawCurrentDataScreen();
        }
    }

    handleSerial();

    static bool btnWasPressed = false;
    bool btnPressed = digitalRead(resetBtn) == LOW;
    if (btnPressed && !btnWasPressed) {
        Serial.println("Button pressed! Resetting fullness and empty timestamp.");
        fullnessamount = 0;
        humidity = 0;
        saveDaysSinceEmpty(0);
        drawCurrentDataScreen();
        updateEyeStatus();
    }
    btnWasPressed = btnPressed;

    if (millis() - lastDataSwitch >= 5000) {
        lastDataSwitch = millis();
        dataState = (dataState + 1) % 3;
        drawCurrentDataScreen();
    }
}
