
#include "DHT.h"
#include <GxEPD2_BW.h>
//#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

/************************************
Defines
*************************************/
#define CS_PIN   (5)  
#define BUSY_PIN (4)
#define RES_PIN  (21)
#define DC_PIN   (22)
// SCL(SCK)=18,SDA(MOSI)=23
#define DHTPIN (15)     // Digital pin connected to the DHT sensor

/************************************
Global variables
*************************************/
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=5*/ CS_PIN, /*DC=*/ DC_PIN, /*RES=*/ RES_PIN, /*BUSY=*/ BUSY_PIN)); // 1.54'' EPD Module
DHT dht(DHTPIN, DHT22);

const char Unit_temp[]     = "Temp [°C]:";
const char Unit_humidity[] = "rF [%]:";
const char Unit_ppm[]      = "CO2 [ppm]:";

struct {          
  float fTemp;  
  float fRf;  
  int   iCo2;
} myMeasureValues;   

/************************************
User code
*************************************/

void showValues(float tempValue, float humValue, int co2Value)
{
  char tempStr[10];
  char humStr[10];
  char co2Str[10];

  // Float-Werte in String umwandeln
  dtostrf(tempValue, 0, 1, tempStr);    // Temperatur: 1 Nachkommastelle
  sprintf(humStr, "%.0f", humValue);    // Feuchte: keine Nachkommastelle
  sprintf(co2Str, "%d", co2Value);      // CO₂: Ganzzahl

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    int16_t tbx, tby; uint16_t tbw, tbh;
    uint16_t x, y;

    // ===== Zeile 1: Temperatur =====
    display.setFont(&FreeMonoBold9pt7b);
    display.getTextBounds(Unit_temp, 0, 0, &tbx, &tby, &tbw, &tbh);
    y = 20; // Startposition
    x = ((display.width() - tbw) / 2) - tbx;
    display.setCursor(x, y);
    display.setTextColor(GxEPD_BLACK);
    display.print(Unit_temp);

    // Temperaturwert (groß)
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
}


bool hasDeviation(float newValue, float oldValue, float tolerance)
{
  return fabs(newValue - oldValue) > tolerance;
}

void setup()
{

    Serial.begin(115200);
  Serial.println(F("DHTxx test!"));



  // Init ink-display
  display.init(115200,true,50,false);

  dht.begin();
  
}

void loop() {
  float fTempAv = dht.readTemperature();  
  float fRf     = round(dht.readHumidity());
  int   iCo2    = 1234;

  // only for debug
  Serial.print(F("Humidity: "));
  Serial.print(fRf);
    Serial.print(" == ");
  Serial.println(myMeasureValues.fRf);

  // Only refresh if there are any changes
  if(hasDeviation(fTempAv, myMeasureValues.fTemp, 0.1) ||
     hasDeviation(fRf, myMeasureValues.fRf, 1)         ||
     hasDeviation(iCo2, myMeasureValues.iCo2, 5))
  {
    showValues(fTempAv, fRf, iCo2);
    display.hibernate();  
    Serial.print("Refresh");

    //Save old values
    myMeasureValues.fTemp = fTempAv;
    myMeasureValues.fRf   = fRf;    
    myMeasureValues.iCo2  = iCo2;    
  }
  

  delay(10000);  
}