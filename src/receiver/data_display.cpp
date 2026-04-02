#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "data_display.h"

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  170
#define TFT_CS         D9
#define TFT_DC         D3
#define TFT_RST        -1   // shared RST already pulsed by eye display init

extern SPIClass dispSPI;
static Adafruit_ST7789 display(&dispSPI, TFT_CS, TFT_DC, TFT_RST);

DataDisplayController dataCtrl;

void DataDisplayController::init() {
    display.init(170, 320);
    display.setRotation(1);
    display.fillScreen(ST77XX_BLACK);
}

void DataDisplayController::test() {
    display.fillScreen(ST77XX_WHITE);
    delay(500);
    display.fillScreen(ST77XX_BLACK);
    display.setTextSize(2);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(0, 0);
    display.println("dataDisplay OK");
    delay(2000);
    display.fillScreen(ST77XX_BLACK);
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ST77XX_WHITE);
}

void DataDisplayController::drawCenteredText(const char* text, int textSize) {
    int16_t x1, y1;
    uint16_t w, h;
    display.fillScreen(ST77XX_BLACK);
    display.setTextSize(textSize);
    display.setTextColor(ST77XX_WHITE);
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
    display.print(text);
}

void DataDisplayController::drawTextWithProgress(const char* text, int progress) {
    const int textSz  = 2;
    const int charH   = 16;  // textSize 2 = 8px * 2
    const int barH    = 15;
    const int gap     = 8;
    const int totalH  = charH + gap + barH;
    const int startY  = (SCREEN_HEIGHT - totalH) / 2;
    const int barY    = startY + charH + gap;
    const int barX    = 10;
    const int barW    = SCREEN_WIDTH - 20;
    const int fillW   = (barW * constrain(progress, 0, 100)) / 100;

    int16_t x1, y1;
    uint16_t w, h;

    display.fillScreen(ST77XX_BLACK);
    display.setTextSize(textSz);
    display.setTextColor(ST77XX_WHITE);
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, startY);
    display.print(text);

    display.drawRect(barX, barY, barW, barH, ST77XX_WHITE);
    if (fillW > 0)
        display.fillRect(barX, barY, fillW, barH, ST77XX_WHITE);
}

void DataDisplayController::clear() {
    display.fillScreen(ST77XX_BLACK);
}
