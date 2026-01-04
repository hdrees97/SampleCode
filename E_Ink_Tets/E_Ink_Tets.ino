// =====================================================
// GxEPD2 + Settings Menu mit Highlight + Longpress Toggle
// =====================================================
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <Preferences.h>

// ESP32 Pins
#define CS_PIN   5
#define BUSY_PIN 4
#define RES_PIN  17
#define DC_PIN   16
#define BTN_PIN  27   // Tasterpin

// 3.7'' EPD Modul
GxEPD2_BW<GxEPD2_370_GDEY037T03,
          GxEPD2_370_GDEY037T03::HEIGHT>
display(GxEPD2_370_GDEY037T03(CS_PIN, DC_PIN, RES_PIN, BUSY_PIN));

// =====================================================
// Menü-Zustände
// =====================================================
enum MenuItem {
    MENU_BATTERY,
    MENU_ZIGBEE,
    MENU_CO2_CAL,
    MENU_TEMP_OFFSET,
    MENU_EXIT,
    MENU_MAX
};


struct configTransferValues {
  bool boBatteryMode;
  bool boZigBee;
  bool boStartCo2Calibration;
  float fTemperatureOffset;
};


typedef struct {
    float       temp;
    float       hum;
    uint8_t     battery;
    uint16_t    co2;
    uint32_t    wakeCounter;
    bool        rtcInitialized;
} RtcState;
RtcState rtcState;


MenuItem currentMenu = MENU_BATTERY;
MenuItem lastMenu = MENU_BATTERY; // global, merkt sich letzten Menüpunkt
Preferences prefs;



// =====================================================
// Hilfsfunktion zum Text zentrieren
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

// =====================================================
// Statisches Menü zeichnen
// =====================================================
void drawStaticMenu()
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);

        int width  = display.width();

        // Überschrift
        drawCenteredText("Settings", 0, 8, width, 24, &FreeMonoBold12pt7b);

        // Menüpunkte
        drawCenteredText("Battery mode:", 0, 40, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("Zigbee:", 0, 70, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("CO2 calibration:", 0, 100, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("Temp offset:", 0, 130, width/2, 20, &FreeMonoBold9pt7b);

    } while(display.nextPage());
}

// =====================================================
// Werte aktualisieren
// =====================================================
void updateMenuValues(configTransferValues configMenu, MenuItem actualSelectedMenu)
{
    int width  = display.width();
  configTransferValues test = configMenu;
    int highlightY[] = {40, 70, 100, 130};

    // Batterie Status
    display.setPartialWindow(width/2, 40, width/2, 20);
    display.firstPage();
    do
    {
        display.fillRect(width/2, 40, width/2, 20, GxEPD_WHITE);
        if (actualSelectedMenu == MENU_BATTERY)
        {
          drawCenteredText(configMenu.boBatteryMode ? "Enabled" : "Disabled", width/2, 40, width/2, 20, &FreeMonoBold9pt7b);
        }
        else
        {
          drawCenteredText(configMenu.boBatteryMode ? "Enabled" : "Disabled", width/2, 40, width/2, 20, &FreeMono9pt7b);
        }
    } while(display.nextPage());

    // Zigbee Status
    display.setPartialWindow(width/2, 70, width/2, 20);
    display.firstPage();
    do
    {
        display.fillRect(width/2, 70, width/2, 20, GxEPD_WHITE);
        
        if (actualSelectedMenu == MENU_ZIGBEE)
        {
          drawCenteredText(configMenu.boZigBee ? "Enabled" : "Disabled", width/2, 70, width/2, 20, &FreeMonoBold9pt7b);
        }
        else
        {
          drawCenteredText(configMenu.boZigBee ? "Enabled" : "Disabled", width/2, 70, width/2, 20, &FreeMono9pt7b);
        }
    } while(display.nextPage());

    // Co2 start measurement
    display.setPartialWindow(width/2, 100, width/2, 20);
    display.firstPage();
    do
    {
        display.fillRect(width/2, 100, width/2, 20, GxEPD_WHITE);
        
        if (actualSelectedMenu == MENU_CO2_CAL)
        {
          drawCenteredText("Run", width/2, 100, width/2, 20, &FreeMonoBold9pt7b);
        }
        else
        {
          drawCenteredText("Run", width/2, 100, width/2, 20, &FreeMono9pt7b);
        }
    } while(display.nextPage());


    // Temp Offset
    display.setPartialWindow(width/2, 130, width/2, 20);
    display.firstPage();
    do
    {
        display.fillRect(width/2, 130, width/2, 20, GxEPD_WHITE);

        if (actualSelectedMenu == MENU_TEMP_OFFSET)
        {
          drawCenteredText((String(configMenu.fTemperatureOffset, 1)).c_str(), width/2, 130, width/2, 20, &FreeMonoBold9pt7b);
        }
        else
        {
          drawCenteredText((String(configMenu.fTemperatureOffset, 1)).c_str(), width/2, 130, width/2, 20, &FreeMono9pt7b);
        }
    } while(display.nextPage());

    // Exit Setting
    display.setPartialWindow(width/2, 160, width/2, 20);
    display.firstPage();
    do
    {
        display.fillRect(width/2, 160, width/2, 20, GxEPD_WHITE);

        if (actualSelectedMenu == MENU_EXIT)
        {
            drawCenteredText("Exit", width/2, 160, width/2, 20, &FreeMonoBold9pt7b);
        }
        else
        {
            drawCenteredText("Exit", width/2, 160, width/2, 20, &FreeMono9pt7b);
        }
    } while(display.nextPage());
}

// =====================================================
// Taster Navigation + Longpress
// =====================================================
void handleButton(configTransferValues *configMenu)
{
    static unsigned long lastPress = 0;
    static unsigned long pressStart = 0;
    unsigned long now = millis();
    bool buttonPressed = (digitalRead(BTN_PIN) == HIGH); // Taster gedrückt (Pullup)

    // Navigation bei kurzem Druck
    if(buttonPressed && pressStart == 0) 
    {
      pressStart = now;
    }

    if(!buttonPressed && pressStart != 0)
    {
        unsigned long duration = now - pressStart;
        pressStart = 0;

        if(duration < 1000) // kurzer Druck -> weiter
        {
            currentMenu = static_cast<MenuItem>((currentMenu + 1) % MENU_MAX);
        }
        else if(duration >= 3000) // langer Druck -> Toggle
        {
            switch(currentMenu)
            {
                case MENU_BATTERY:
                    configMenu->boBatteryMode = !configMenu->boBatteryMode;
                    prefs.begin("system", false);
                    prefs.putBool("BatteryMode",configMenu->boBatteryMode);
                    prefs.end();
                    break;
                case MENU_ZIGBEE:
                    configMenu->boZigBee = !configMenu->boZigBee;
                    prefs.begin("system", false);
                    prefs.putBool("ZigBee",configMenu->boZigBee);
                    prefs.end();
                    break;
                case MENU_CO2_CAL:
                    //add co2 calibration menu
                    break;
                case MENU_TEMP_OFFSET:
                    prefs.begin("system", false);
                    prefs.putFloat("TemperatureOffset", 2.2f);
                    prefs.end();
                    break;
                case MENU_EXIT:
                    break;
                default:
                    break; // andere Felder ignorieren langen Druck
            }
        updateMenuValues(*configMenu, currentMenu);
        }
    }
}


// =====================================================
// Setup & Loop
// =====================================================
void setup()
{
    Serial.begin(115200);
    delay(200);
    pinMode(BTN_PIN, INPUT_PULLUP);


    if (!rtcState.rtcInitialized)
    {
      Serial.println("First run after flash");
        prefs.begin("system", false);

        if (!prefs.getBool("everInitialized", false))
        {
            prefs.putBool("everInitialized", true);

            prefs.putBool("BatteryMode", true);
            prefs.putBool("ZigBee", false);

        }

        prefs.end();
        rtcState.rtcInitialized = true;
    }


    // Here add menu open

    configTransferValues configMenu;

    prefs.begin("system", false);
    configMenu.boBatteryMode = prefs.getBool("BatteryMode");
    configMenu.boZigBee = prefs.getBool("ZigBee");
    configMenu.boStartCo2Calibration = false;
    configMenu.fTemperatureOffset = 12.0f; //@ToDo  read from sensor
    prefs.end();
    
    //Print does work
    Serial.println("Test: "); // Prüfe, was wirklich drin steht
    Serial.println(configMenu.fTemperatureOffset); // Prüfe, was wirklich drin steht

    display.init();
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);

    drawStaticMenu();
    updateMenuValues(configMenu, currentMenu);

    delay(200);
    while(1)
    {
      handleButton(&configMenu);

      if(currentMenu != lastMenu) 
      {
          updateMenuValues(configMenu, currentMenu);
          lastMenu = currentMenu;
      }
    }

}

void loop()
{

}

