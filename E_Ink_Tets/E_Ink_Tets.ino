
// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>



// ESP32 CS(SS)=5,SCL(SCK)=18,SDA(MOSI)=23,BUSY=4,RES(RST)=17,DC=16
#define CS_PIN (5)
#define BUSY_PIN (4)
#define RES_PIN (17)
#define DC_PIN (16)

// 3.7'' EPD Module
GxEPD2_BW<GxEPD2_370_GDEY037T03, GxEPD2_370_GDEY037T03::HEIGHT> display(GxEPD2_370_GDEY037T03(/*CS=5*/ CS_PIN, /*DC=*/ DC_PIN, /*RES=*/ RES_PIN, /*BUSY=*/ BUSY_PIN)); // GDEY037T03 240x416, UC8253

void setup()
{
  display.init();
  display.setRotation(1); // 1 = Querformat
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  drawLayout();
}

void loop()
{
  // nichts
}

void drawCenteredText(const char* text, int x, int y, int w, int h, const GFXfont* font)
{
  int16_t tbx, tby;
  uint16_t tbw, tbh;

  display.setFont(font);
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

  int cx = x + (w - tbw) / 2;
  int cy = y + (h + tbh) / 2;

  display.setCursor(cx, cy);
  display.print(text);
}


void drawLayout()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    int width  = display.width();   // 416
    int height = display.height();  // 240

    int topHeight = 120;
    int leftWidth = width / 2;

    // ---------- Rahmen ----------
    display.drawRect(0, 0, width, height, GxEPD_BLACK);
    display.drawLine(0, topHeight, width, topHeight, GxEPD_BLACK);
    display.drawLine(leftWidth, topHeight, leftWidth, height, GxEPD_BLACK);

    // ===== Sektion 1 (oben) =====
    drawCenteredText(
      "CO2 [ppm]",
      0, 8,
      width, 22,
      &FreeMonoBold9pt7b
    );

    display.setTextSize(2);
    drawCenteredText(
      "1231",
      0, 30,
      width, 80,
      &FreeMonoBold24pt7b
    );
    display.setTextSize(1);

    // ===== Sektion 2 (unten links) =====
    drawCenteredText(
      "Temp [C]",
      0, topHeight + 8,
      leftWidth, 22,
      &FreeMonoBold9pt7b
    );

    display.setTextSize(2);
    drawCenteredText(
      "19.7",
      0, topHeight + 30,
      leftWidth, height - topHeight - 30,
      &FreeMonoBold24pt7b
    );
    display.setTextSize(1);

    // ===== Sektion 3 (unten rechts) =====
    drawCenteredText(
      "rF [%]",
      leftWidth, topHeight + 8,
      leftWidth, 22,
      &FreeMonoBold9pt7b
    );

    display.setTextSize(2);
    drawCenteredText(
      "67",
      leftWidth, topHeight + 30,
      leftWidth, height - topHeight - 30,
      &FreeMonoBold24pt7b
    );
    display.setTextSize(1);

  } while (display.nextPage());
}


