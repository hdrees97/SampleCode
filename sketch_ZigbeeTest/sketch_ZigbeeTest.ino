// Copyright 2025 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// http://www.apache.org/licenses/LICENSE-2.0
//
// Example: Zigbee Temperature, Humidity & CO2 sleepy sensor for ESP32-C6
//
// Based on Espressif example + CO2 sensor extension
//

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

#define USE_GLOBAL_ON_RESPONSE_CALLBACK 1

/* Endpunkte definieren */
#define TEMP_SENSOR_ENDPOINT_NUMBER 10
#define CO2_SENSOR_ENDPOINT_NUMBER  11

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  10
#define REPORT_TIMEOUT 2000

uint8_t button = BOOT_PIN;

/* Sensor-Objekte */
ZigbeeTempSensor zbTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);
ZigbeeCarbonDioxideSensor zbCO2Sensor(CO2_SENSOR_ENDPOINT_NUMBER);

/* Globale Statusvariablen */
uint8_t dataToSend = 3;  // 3 Werte: Temp, Humidity, CO2
bool resend = false;

/************************ Callbacks *****************************/
#if USE_GLOBAL_ON_RESPONSE_CALLBACK
void onGlobalResponse(zb_cmd_type_t command, esp_zb_zcl_status_t status,
                      uint8_t endpoint, uint16_t cluster) {
  Serial.printf("Global response command: %d, status: %s, endpoint: %d, cluster: 0x%04x\r\n",
                command, esp_zb_zcl_status_to_name(status), endpoint, cluster);

  if ((command == ZB_CMD_REPORT_ATTRIBUTE) &&
      (endpoint == TEMP_SENSOR_ENDPOINT_NUMBER || endpoint == CO2_SENSOR_ENDPOINT_NUMBER)) {
    switch (status) {
      case ESP_ZB_ZCL_STATUS_SUCCESS:
        dataToSend--;
        break;
      case ESP_ZB_ZCL_STATUS_FAIL:
        resend = true;
        break;
      default:
        break;
    }
  }
}
#else
void onResponse(zb_cmd_type_t command, esp_zb_zcl_status_t status) {
  Serial.printf("Response command: %d, status: %s\r\n",
                command, esp_zb_zcl_status_to_name(status));
  if (command == ZB_CMD_REPORT_ATTRIBUTE) {
    switch (status) {
      case ESP_ZB_ZCL_STATUS_SUCCESS:
        dataToSend--;
        break;
      case ESP_ZB_ZCL_STATUS_FAIL:
        resend = true;
        break;
      default:
        break;
    }
  }
}
#endif

/************************ Messfunktion *****************************/
static void measureAndSleep(void *arg) {
  // Dummy-Messwerte (hier kannst du später echte Sensoren anbinden)
  float temperature = temperatureRead();
  float humidity = temperature;  // Dummy
  float co2_ppm = 400.0 + (esp_random() % 300);  // Zufall zwischen 400–700 ppm

  // Werte setzen
  zbTempSensor.setTemperature(temperature);
  zbTempSensor.setHumidity(humidity);
  zbCO2Sensor.setCarbonDioxide(co2_ppm);

  // Reports senden
  zbTempSensor.report();  // Temperatur + Luftfeuchtigkeit
  zbCO2Sensor.report();   // CO₂ separat

  Serial.printf("Reported → Temp: %.2f°C, Hum: %.2f%%, CO2: %.0f ppm\r\n",
                temperature, humidity, co2_ppm);

  unsigned long startTime = millis();
  const unsigned long timeout = REPORT_TIMEOUT;
  Serial.println("Waiting for Zigbee confirmations...");

  int tries = 0;
  const int maxTries = 3;
  while (dataToSend != 0 && tries < maxTries) {
    if (resend) {
      Serial.println("Resending data due to failure!");
      resend = false;
      dataToSend = 3;
      zbTempSensor.report();
      zbCO2Sensor.report();
    }
    if (millis() - startTime >= timeout) {
      Serial.println("\nReport timeout → Resending");
      dataToSend = 3;
      zbTempSensor.report();
      zbCO2Sensor.report();
      startTime = millis();
      tries++;
    }
    delay(50);
  }

  // Kurzes Warten, damit Zigbee Stack senden & ACK empfangen kann
  delay(2000);  // 1 Sekunde reicht in der Regel völlig aus

  Serial.println("Going to deep sleep...");
  esp_deep_sleep_start();
}

/************************ Arduino setup *****************************/
void setup() {
  Serial.begin(115200);
  pinMode(button, INPUT_PULLUP);

  // Timer-Wakeup aktivieren
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // --- Temperatur + Feuchtigkeit ---
  zbTempSensor.setManufacturerAndModel("Esp32", "ZigbeeTempHumSensor");
  zbTempSensor.setMinMaxValue(-10, 60);
  zbTempSensor.setTolerance(1);
  zbTempSensor.addHumiditySensor(0, 100, 1);

  // --- CO2 Sensor konfigurieren ---
  zbCO2Sensor.setManufacturerAndModel("Esp32", "ZigbeeCO2Sensor");
  zbCO2Sensor.setMinMaxValue(0, 5000);  // ppm Bereich
  zbCO2Sensor.setTolerance(20);         // ±20 ppm

#if USE_GLOBAL_ON_RESPONSE_CALLBACK
  Zigbee.onGlobalDefaultResponse(onGlobalResponse);
#else
  zbTempSensor.onDefaultResponse(onResponse);
  zbCO2Sensor.onDefaultResponse(onResponse);
#endif

  // Endpunkte hinzufügen
  Zigbee.addEndpoint(&zbTempSensor);
  Zigbee.addEndpoint(&zbCO2Sensor);

  // Zigbee initialisieren
  esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  zigbeeConfig.nwk_cfg.zed_cfg.keep_alive = 10000;
  Zigbee.setTimeout(10000);

  if (!Zigbee.begin(&zigbeeConfig, false)) {
    Serial.println("Zigbee failed to start. Restarting...");
    ESP.restart();
  }

  Serial.println("Connecting to Zigbee network...");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\n✅ Connected to Zigbee network.");

  // Mess-Task starten
  xTaskCreate(measureAndSleep, "measure_and_sleep", 4096, NULL, 10, NULL);
}

/************************ Arduino loop *****************************/
void loop() {
  if (digitalRead(button) == LOW) {
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 10000) {
        Serial.println("Factory reset Zigbee and sleep...");
        delay(1000);
        Zigbee.factoryReset(false);
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        esp_deep_sleep_start();
      }
    }
  }
  delay(100);
}
