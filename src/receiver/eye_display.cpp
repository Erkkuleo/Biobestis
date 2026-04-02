#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <pgmspace.h>
#include "eye_display.h"
#include "art/bitmaps.h"

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  170
#define TFT_CS         D7
#define TFT_DC         D3
#define TFT_RST        D2

SPIClass dispSPI(FSPI);
static Adafruit_ST7789 display(&dispSPI, TFT_CS, TFT_DC, TFT_RST);

EyeDisplayController eyeCtrl;

void EyeDisplayController::init() {
    dispSPI.begin(D8, -1, D10);
    display.init(170, 320);
    display.setRotation(1);
    display.fillScreen(ST77XX_BLACK);
}

void EyeDisplayController::test() {
    display.fillScreen(ST77XX_WHITE);
    delay(500);
    display.fillScreen(ST77XX_BLACK);
    display.setTextSize(2);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(0, 0);
    display.println("eyeDisplay OK");
    delay(2000);
    display.fillScreen(ST77XX_BLACK);
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ST77XX_WHITE);
}

void EyeDisplayController::drawBitmap(const unsigned char* bitmap) {
    const int SCALE = 2;
    const int BMP_W = EYE_W;
    const int BMP_H = EYE_H;
    const int ox = (SCREEN_WIDTH  - BMP_W * SCALE) / 2;
    const int oy = (SCREEN_HEIGHT - BMP_H * SCALE) / 2;

    display.startWrite();
    display.writeFillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ST77XX_BLACK);
    for (int y = 0; y < BMP_H; y++) {
        for (int x = 0; x < BMP_W; x++) {
            uint8_t b = pgm_read_byte(&bitmap[y * (BMP_W / 8) + x / 8]);
            if (b & (0x80 >> (x % 8))) {
                display.writeFillRect(ox + x * SCALE, oy + y * SCALE, SCALE, SCALE, ST77XX_WHITE);
            }
        }
    }
    display.endWrite();
}

void EyeDisplayController::clear() {
    display.fillScreen(ST77XX_BLACK);
}
