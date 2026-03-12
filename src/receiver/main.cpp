#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Preferences.h>
#include <wifi_controller.h>
#include "eye_display.h"
#include "data_display.h"
#include "art/bitmaps.h"

const int led = D10;
const int resetBtn = D1;

Preferences prefs;

int fullnessamount = 0;
int humidity = 65;
unsigned long lastDataSwitch = 0;
unsigned long lastDayCheck = 0;
int dataState = 0;  // 0=fullness, 1=humidity, 2=emptied days


void saveDaysSinceEmpty(uint16_t days) {
    prefs.putUShort("days", days);
}

int getDaysSinceEmpty() {
    return (int)prefs.getUShort("days", 0);
}

void updateEyeStatus() {
    int days = getDaysSinceEmpty();
    // int worstSegment = max({min(fullnessamount / 17, 5), min(humidity / 17, 5), min(days, 5)});
    int worstSegment = min(days, 5);
    eyeCtrl.drawBitmap((const unsigned char*)pgm_read_ptr(&statusBitmaps[worstSegment]));
}

void drawCurrentDataScreen() {
    if (dataState == 0) {
        dataCtrl.drawTextWithProgress("Fullness state", fullnessamount);
    } else if (dataState == 1) {
        char buf[20];
        snprintf(buf, sizeof(buf), "Humidity: %d %%", humidity);
        dataCtrl.drawCenteredText(buf, 1);
    } else {
        char buf[24];
        snprintf(buf, sizeof(buf), "Emptied %d days ago", getDaysSinceEmpty());
        dataCtrl.drawCenteredText(buf, 1);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    pinMode(resetBtn, INPUT_PULLUP);
    wifi_connect("aalto open", "");

    prefs.begin("kayttiselain", false);

    Wire.begin(D4, D5);
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
