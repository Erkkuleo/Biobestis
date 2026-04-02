#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_VL53L0X.h>
#include "../shared/protocol.h"

#define DHT_PIN  D10
#define DHT_TYPE DHT22

// Distance thresholds: sensor mounted above the litter box
// DIST_EMPTY = litter box empty (sensor far from litter surface)
// DIST_FULL  = litter box full  (sensor close to litter surface)
#define DIST_EMPTY_MM  300
#define DIST_FULL_MM    50

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_VL53L0X tof;
bool tofReady = false;

static uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static unsigned long lastSend = 0;
static int lastDistMm = -1;

void setup() {
    Serial.begin(115200);
    dht.begin();

    Wire.begin();
    if (tof.begin()) {
        tofReady = true;
        Serial.println("VL53L0X ready.");
    } else {
        Serial.println("VL53L0X not found — fullness will read 0.");
    }

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcastMac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    Serial.println("Transmitter ready.");
}

void loop() {
    if (millis() - lastSend < 100) return;
    lastSend = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("DHT read failed — check wiring");
        return;
    }

    int fullness = 0;
    if (tofReady) {
        VL53L0X_RangingMeasurementData_t measure;
        tof.rangingTest(&measure, false);
        if (measure.RangeStatus != 4) {
            int dist = measure.RangeMilliMeter;
            if (lastDistMm < 0 || abs(dist - lastDistMm) >= 10) {
                lastDistMm = dist;
            }
            fullness = constrain(
                map(lastDistMm, DIST_EMPTY_MM, DIST_FULL_MM, 0, 100),
                0, 100
            );
        }
    }

    Serial.printf("Temp: %.1f C  Humidity: %.1f %%  Fullness: %d %%\n", t, h, fullness);

    SensorPacket pkt;
    pkt.magic[0] = 0xCA;
    pkt.magic[1] = 0xFE;
    pkt.fullness = (int8_t)fullness;
    pkt.humidity = (int8_t)constrain((int)h, 0, 100);

    esp_now_send(broadcastMac, (uint8_t*)&pkt, sizeof(pkt));
}
