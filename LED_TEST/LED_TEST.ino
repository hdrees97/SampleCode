#include <Adafruit_NeoPixel.h>

#define LED_PIN    14
#define LED_COUNT  8

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t currentColor; // aktuelle LED-Farbe
uint32_t targetColor;  // Ziel-Farbe
unsigned long lastUpdate = 0;
const int transitionTime = 5000; // 2 Sekunden für Übergang
const int steps = 200;            // Anzahl Übergangsschritte
int stepCount = 0;

void setup() {
  strip.begin();
  strip.show();
  strip.setBrightness(50);

  currentColor = strip.Color(0, 255, 0); // Start: Grün
  targetColor  = currentColor;
  setAll(currentColor);
}

void loop() {
  static unsigned long lastValueTime = 0;

  // *** alle 30 Sekunden neuen Wert simulieren ***
  if (millis() - lastValueTime > 30000) {
    lastValueTime = millis();

    int newValue = random(350, 2001);  // <- HIER ersetze durch echten Messwert
    Serial.println(newValue);

    targetColor = mapValueToColor(newValue);
    stepCount = 0;
  }

  smoothUpdate();
}


/* -----------------------------
      Smooth Transition
   ----------------------------- */
void smoothUpdate() {
  if (stepCount >= steps) return;

  if (millis() - lastUpdate < (transitionTime / steps)) return;
  lastUpdate = millis();

  // RGB zerlegen
  byte r1 = (currentColor >> 16) & 0xFF;
  byte g1 = (currentColor >>  8) & 0xFF;
  byte b1 =  currentColor        & 0xFF;

  byte r2 = (targetColor >> 16) & 0xFF;
  byte g2 = (targetColor >>  8) & 0xFF;
  byte b2 =  targetColor        & 0xFF;

  // Interpolate
  float t = (float)stepCount / steps;
  byte r = r1 + (r2 - r1) * t;
  byte g = g1 + (g2 - g1) * t;
  byte b = b1 + (b2 - b1) * t;

  uint32_t newColor = strip.Color(r, g, b);
  setAll(newColor);

  stepCount++;
  if (stepCount >= steps) currentColor = targetColor;
}


/* -----------------------------
      Farb-Mapping: 350–2000
   ----------------------------- */
uint32_t mapValueToColor(int value) {
  value = constrain(value, 350, 2000);

  // Farbstufen:
  // 350   = Grün
  // 900   = Gelb
  // 1400  = Orange
  // 2000  = Rot

  if (value <= 900) {
    // Grün → Gelb
    float t = (value - 350.0) / (900 - 350);
    return strip.Color(
      0 + (255 * t),   // R: 0 → 255
      255,             // G bleibt 255
      0                // B 0
    );
  }
  else if (value <= 1400) {
    // Gelb → Orange
    float t = (value - 900.0) / (1400 - 900);
    return strip.Color(
      255,             // R bleibt 255
      255 - (100 * t), // G: 255 → 155
      0
    );
  }
  else {
    // Orange → Rot
    float t = (value - 1400.0) / (2000 - 1400);
    return strip.Color(
      255,
      155 - (155 * t), // G: 155 → 0
      0
    );
  }
}


/* -----------------------------
    Hilfsfunktion für LEDs
   ----------------------------- */
void setAll(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}
