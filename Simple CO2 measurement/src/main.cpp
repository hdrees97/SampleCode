#include <Arduino.h>
#include "MHZ19.h"

// Dont forget to twist the RX/TX
#define RX_PIN 16         // MH-Z19 RX (an ESP32-TX)
#define TX_PIN 17         // MH-Z19 TX (an ESP32-RX)
#define BAUDRATE 9600

MHZ19 myMHZ19;
HardwareSerial mySerial(2); // UART2 des ESP32

unsigned long getDataTimer = 0;

void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.println("MH-Z19 CO2 Sensor startet...");

  // UART2 konfigurieren (Pins RX=5, TX=4)
  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  myMHZ19.begin(mySerial);

  Serial.println("Sensor initialisiert!");
}

void loop() {
  if (millis() - getDataTimer >= 2000) {

    int CO2 = myMHZ19.getCO2();
    float f_Temp = myMHZ19.getTemperature();

    // Get CO2 value
    Serial.print("CO2 (ppm): ");
    Serial.println(CO2);

    // Get temp value
    Serial.print("Temp (°C): ");
    Serial.println(f_Temp);

    getDataTimer = millis();
  }
}
