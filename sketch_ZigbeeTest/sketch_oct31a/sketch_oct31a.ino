/***********************************************
 *  includes
 ***********************************************/
#include "Zigbee.h"
#include "MHZ19.h"
#include "DHT.h"
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

/***********************************************
 *  defines
 ***********************************************/
//Zigbee
#define TEMP_SENSOR_ENDPOINT_NUMBER 10
#define CO2_SENSOR_ENDPOINT_NUMBER 11
//CO2 MHZ19
#define RX_PIN 5         //MH-Z19 !!RX!! (an ESP32-TX)
#define TX_PIN 4         //MH-Z19 !!TX!! (an ESP32-RX)
#define BAUDRATE 9600    //UART
//E-ink display
#define BUSY_PIN (15)
#define RES_PIN  (2)
#define DC_PIN   (1)
#define CS_PIN   (6)
//DHT22
#define DHTPIN (11)
//General
#define DEBUG_SERIAL_PRINT

/***********************************************
 *  global variables
 ***********************************************/
struct tm timeinfo;
struct tm *localTime;
int32_t timezone;

struct {          
  float TempValue     = 0;  
  float HumidityValue = 0;  
  int   Co2Value      = 0;
} MeasureValues;   

// Zigbee-Sensoren
ZigbeeTempSensor          zbTempSensor = ZigbeeTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);
ZigbeeCarbonDioxideSensor zbCO2Sensor  = ZigbeeCarbonDioxideSensor(CO2_SENSOR_ENDPOINT_NUMBER);

// CO2 MHZ19
MHZ19 myMHZ19;
HardwareSerial mySerial(1); // UART2 des ESP32

// E-Ink Display
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(CS_PIN, DC_PIN, RES_PIN, BUSY_PIN));
const char Unit_temp[]     = "Temp [°C]:";
const char Unit_humidity[] = "rF [%]:";
const char Unit_ppm[]      = "CO2 [ppm]:";

// DHT22
DHT dht(DHTPIN, DHT22);

/***********************************************
 *  ZigbeeUpdateTask()
 ***********************************************/
static void ZigbeeUpdateTask(void *arg) 
{
    while (true) {
        zbTempSensor.setTemperature(MeasureValues.TempValue);
        zbTempSensor.setHumidity(MeasureValues.HumidityValue);
        zbCO2Sensor.setCarbonDioxide((int)MeasureValues.Co2Value);

#ifdef DEBUG_SERIAL_PRINT
        Serial.printf("Send via ZigBee: Temperature: %.2f °C | Humidity: %.2f %% | CO2: %d ppm\r\n",
                      MeasureValues.TempValue, MeasureValues.HumidityValue, MeasureValues.Co2Value);
#endif
        delay(60000);
    }
}

/***********************************************
 *  ReadSensorUpdateTask()
 ***********************************************/
static void ReadSensorUpdateTask(void *arg) 
{
    while (true) {
        MeasureValues.TempValue = dht.readTemperature();  
        MeasureValues.HumidityValue = round(dht.readHumidity());
        MeasureValues.Co2Value = myMHZ19.getCO2();

#ifdef DEBUG_SERIAL_PRINT
        Serial.printf("Read from sensor: -> Temp: %.2f °C, Humidity: %.2f %%, CO2: %d ppm\n",
                      MeasureValues.TempValue, MeasureValues.HumidityValue, MeasureValues.Co2Value);
#endif
        delay(10000);
    }
}

/***********************************************
 *  ShowValuesTask()
 ***********************************************/
static void ShowValuesTask(void *arg) 
{
  char tempStr[10];
  char humStr[10];
  char co2Str[10];

  static int refreshCounter = 0;
  static bool refreshValues = false; // Hier weiter
  static float lastTemp = -999;
  static float lastHum  = -999;
  static int   lastCO2  = -999;

  //Execute display
  while (true) 
  {
    if (fabs(MeasureValues.TempValue - lastTemp)    < 0.1 &&
        fabs(MeasureValues.HumidityValue - lastHum) < 1.0 &&
        abs(MeasureValues.Co2Value - lastCO2)       < 10)
    {
      //Break the execution
#ifdef DEBUG_SERIAL_PRINT
      Serial.println("Werte fast unverändert – kein Display-Update.");
#endif
      delay(10000);
      continue;
    }
    else
    {
      lastTemp = MeasureValues.TempValue;
      lastHum  = MeasureValues.HumidityValue;
      lastCO2  = MeasureValues.Co2Value;
    }

    dtostrf(MeasureValues.TempValue, 0, 1, tempStr);
    sprintf(humStr, "%.0f", MeasureValues.HumidityValue);
    sprintf(co2Str, "%d", MeasureValues.Co2Value);

    bool doFullRefresh = false;

    if (refreshCounter >= 50) 
    {
      doFullRefresh = true;
    }

    if (doFullRefresh) 
    {
      display.setFullWindow();
    } 
    else 
    {
      display.setPartialWindow(0, 0, display.width(), display.height());
    }

    display.firstPage();
    do 
    {
        if (doFullRefresh) 
        {
            display.fillScreen(GxEPD_WHITE);
        }

        display.setRotation(1);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_BLACK);

        int16_t tbx, tby;
        uint16_t tbw, tbh;
        uint16_t x, y;

        // ===== Zeile 1: Temperatur =====
        display.getTextBounds(Unit_temp, 0, 0, &tbx, &tby, &tbw, &tbh);
        y = 20;
        x = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(x, y);
        display.print(Unit_temp);

        display.setFont(&FreeMonoBold18pt7b);
        display.getTextBounds(tempStr, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = ((display.width() - tbw) / 2) - tbx;
        y += 30;
        display.setCursor(x, y);
        display.print(tempStr);

        // ===== Zeile 2: Luftfeuchtigkeit =====
        display.setFont(&FreeMonoBold9pt7b);
        display.getTextBounds(Unit_humidity, 0, 0, &tbx, &tby, &tbw, &tbh);
        y += 40;
        x = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(x, y);
        display.print(Unit_humidity);

        display.setFont(&FreeMonoBold18pt7b);
        display.getTextBounds(humStr, 0, 0, &tbx, &tby, &tbw, &tbh);
        y += 30;
        x = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(x, y);
        display.print(humStr);

        // ===== Zeile 3: CO2 =====
        display.setFont(&FreeMonoBold9pt7b);
        display.getTextBounds(Unit_ppm, 0, 0, &tbx, &tby, &tbw, &tbh);
        y += 40;
        x = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(x, y);
        display.print(Unit_ppm);

        display.setFont(&FreeMonoBold18pt7b);
        display.getTextBounds(co2Str, 0, 0, &tbx, &tby, &tbw, &tbh);
        y += 30;
        x = ((display.width() - tbw) / 2) - tbx;
        display.setCursor(x, y);
        display.print(co2Str);

    } while (display.nextPage());

    if (doFullRefresh) 
    {
      refreshCounter = 0;
    } 
    else 
    {
      refreshCounter = refreshCounter + 1;
    }
    delay(10000);
  }
}

/***********************************************
 *  ZigbeeSetUp()
 ***********************************************/
static void ZigbeeSetUp() 
{
    zbTempSensor.setManufacturerAndModel("Espressif", "ZigbeeTempHumiditySensor");
    zbTempSensor.setMinMaxValue(-5, 50);
    zbTempSensor.setTolerance(1);
    zbTempSensor.addHumiditySensor(0, 100, 1);
    zbTempSensor.addTimeCluster();
    Zigbee.addEndpoint(&zbTempSensor);

    zbCO2Sensor.setManufacturerAndModel("Espressif", "ZigbeeCO2Sensor");
    zbCO2Sensor.setMinMaxValue(300, 5000);
    zbCO2Sensor.setTolerance(20);
    Zigbee.addEndpoint(&zbCO2Sensor);

#ifdef DEBUG_SERIAL_PRINT
    Serial.println("Starting Zigbee...");
#endif
    if (!Zigbee.begin()) {
#ifdef DEBUG_SERIAL_PRINT
        Serial.println("Zigbee failed to start!");
#endif
        delay(1000);
        ESP.restart();
    }

#ifdef DEBUG_SERIAL_PRINT
    Serial.println("Zigbee started successfully!");
    Serial.println("Connecting to network");
#endif

    TickType_t startTime = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(20000);

    while (!Zigbee.connected()) {
#ifdef DEBUG_SERIAL_PRINT
        Serial.print(".");
#endif
        vTaskDelay(pdMS_TO_TICKS(100));

        if ((xTaskGetTickCount() - startTime) >= timeout) {
#ifdef DEBUG_SERIAL_PRINT
            Serial.println("Continuing without Zigbee.");
#endif
            return;
        }
    }

#ifdef DEBUG_SERIAL_PRINT
    Serial.println("\nConnected to Zigbee network.");
#endif

    timeinfo = zbTempSensor.getTime();
    timezone = zbTempSensor.getTimezone();
    time_t local = mktime(&timeinfo) + timezone;
    localTime = localtime(&local);

    zbTempSensor.setReporting(1, 60, 0);
    zbTempSensor.setHumidityReporting(1, 60, 0);
    zbCO2Sensor.setReporting(1, 60, 0);
}

/***********************************************
 *  Arduino setup()
 ***********************************************/
void setup() 
{
#ifdef DEBUG_SERIAL_PRINT
    Serial.begin(115200);
#endif

    ZigbeeSetUp();

    mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
    myMHZ19.begin(mySerial);

    SPI.begin(21, -1, 22, 5);
    display.init(115200, true, 50, false);

    dht.begin();

    xTaskCreate(ReadSensorUpdateTask, "read_sensor_values", 4096, NULL, 10, NULL);
    xTaskCreate(ShowValuesTask, "show_values_e_ink", 4096, NULL, 10, NULL);
    if (Zigbee.connected()) {
        xTaskCreate(ZigbeeUpdateTask, "sensors_value_2_zigbee", 4096, NULL, 10, NULL);
    }
}

/***********************************************
 *  Arduino loop()
 ***********************************************/
void loop() 
{
    delay(10000);
}
