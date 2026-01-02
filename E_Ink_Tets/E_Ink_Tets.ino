// =====================================================
// GxEPD2 Konfiguration
// =====================================================
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

// ESP32 Pins
#define CS_PIN   5
#define BUSY_PIN 4
#define RES_PIN  17
#define DC_PIN   16

// 3.7'' EPD Modul (UC8253)
GxEPD2_BW<GxEPD2_370_GDEY037T03,
          GxEPD2_370_GDEY037T03::HEIGHT>
display(GxEPD2_370_GDEY037T03(CS_PIN, DC_PIN, RES_PIN, BUSY_PIN));

// =====================================================
// Timing
// =====================================================
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000; // 5 Sekunden

// =====================================================
// Hilfsfunktionen
// =====================================================
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

void drawBatteryIcon(int x, int y, int w, int h, int percent)
{
  percent = constrain(percent, 0, 100);

  int tipWidth  = w / 8;
  int bodyWidth = w - tipWidth - 2;

  display.drawRect(x, y, bodyWidth, h, GxEPD_BLACK);
  display.fillRect(x + bodyWidth, y + h / 4, tipWidth, h / 2, GxEPD_BLACK);

  int fillWidth = (bodyWidth - 4) * percent / 100;
  display.fillRect(x + 2, y + 2, fillWidth, h - 4, GxEPD_BLACK);
}

// =====================================================
// Statisches Layout (einmal)
// =====================================================
void drawStaticLayout()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    int width  = display.width();
    int height = display.height();

    int topHeight = 120;
    int leftWidth = width / 2;

    // Rahmen
    display.drawRect(0, 0, width, height, GxEPD_BLACK);
    display.drawLine(0, topHeight, width, topHeight, GxEPD_BLACK);
    display.drawLine(leftWidth, topHeight, leftWidth, height, GxEPD_BLACK);

    // Überschriften
    drawCenteredText("CO2 [ppm]", 0, 8, width, 24, &FreeMonoBold9pt7b);
    drawCenteredText("Temp [C]", 0, topHeight + 8, leftWidth, 24, &FreeMonoBold9pt7b);
    drawCenteredText("rF [%]", leftWidth, topHeight + 8, leftWidth, 24, &FreeMonoBold9pt7b);

  } while (display.nextPage());
}

// =====================================================
// Dynamische Inhalte (Partial Updates)
// =====================================================
void updateValues(int co2, float temp, int hum, int batteryPercent)
{
  int width  = display.width();
  int height = display.height();
  int topHeight = 120;
  int leftWidth = width / 2;

  const int margin = 8; // 8-Pixel-Raster

  // ===== CO2 =====
  display.setPartialWindow(margin, 40, width - margin * 2, 64);
  display.firstPage();
  do
  {
    display.fillRect(margin, 40, width - margin * 2, 64, GxEPD_WHITE);
    drawCenteredText(String(co2).c_str(),
                     margin, 40,
                     width - margin * 2, 64,
                     &FreeMonoBold24pt7b);
  } while (display.nextPage());

  // ===== Temperatur =====
  display.setPartialWindow(margin, 152, leftWidth - margin * 2, 64);
  display.firstPage();
  do
  {
    display.fillRect(margin, 152, leftWidth - margin * 2, 64, GxEPD_WHITE);
    drawCenteredText(String(temp, 1).c_str(),
                     margin, 152,
                     leftWidth - margin * 2, 64,
                     &FreeMonoBold24pt7b);
  } while (display.nextPage());

  // ===== Luftfeuchte =====
  display.setPartialWindow(leftWidth + margin, 152, leftWidth - margin * 2, 64);
  display.firstPage();
  do
  {
    display.fillRect(leftWidth + margin, 152, leftWidth - margin * 2, 64, GxEPD_WHITE);
    drawCenteredText(String(hum).c_str(),
                     leftWidth + margin, 152,
                     leftWidth - margin * 2, 64,
                     &FreeMonoBold24pt7b);
  } while (display.nextPage());

  // ===== Batterie =====
  display.setPartialWindow(width - 48, 8, 40, 16);
  display.firstPage();
  do
  {
    display.fillRect(width - 48, 8, 40, 16, GxEPD_WHITE);
    drawBatteryIcon(width - 45, 8, 40, 16, batteryPercent);
  } while (display.nextPage());
}

// =====================================================
// Zufallswerte
// =====================================================
void generateRandomValues(int &co2, float &temp, int &hum, int &battery)
{
  co2     = random(400, 2000);
  temp    = random(180, 300) / 10.0;
  hum     = random(30, 80);
  battery = random(10, 100);
}

// =====================================================
// Arduino
// =====================================================
void setup()
{
  display.init();
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);

  randomSeed(esp_random());

  drawStaticLayout();

  int co2, hum, battery;
  float temp;
  generateRandomValues(co2, temp, hum, battery);
  updateValues(co2, temp, hum, battery);
}

void loop()
{
  if (millis() - lastUpdate >= updateInterval)
  {
    lastUpdate = millis();

    int co2, hum, battery;
    float temp;
    generateRandomValues(co2, temp, hum, battery);
    updateValues(co2, temp, hum, battery);
  }
}
