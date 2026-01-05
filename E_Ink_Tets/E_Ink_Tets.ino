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
enum SettingsMenuItem {
    SETTING_MENU_BATTERY,
    SETTING_MENU_ZIGBEE,
    SETTING_MENU_REFRESH_MEASUREMENT,
    SETTING_MENU_DISPLAY_REBUILD,
    SETTING_MENU_CO2_CAL,
    SETTING_MENU_TEMP_OFFSET,
    SETTING_MENU_EXIT,
    SETTING_MENU_MAX_ROWS_PLACEHOLDER
};


// 
#define SETTING_MENU_MAX_ROWS 7
#define SETTING_MENU_ROW_1   40
#define SETTING_MENU_ROW_2   65
#define SETTING_MENU_ROW_3   90
#define SETTING_MENU_ROW_4   115
#define SETTING_MENU_ROW_5   140
#define SETTING_MENU_ROW_6   165
#define SETTING_MENU_ROW_7   190

static const int SETTING_MENU_ROW_Y[SETTING_MENU_MAX_ROWS] = {SETTING_MENU_ROW_1,
                                                              SETTING_MENU_ROW_2,
                                                              SETTING_MENU_ROW_3,
                                                              SETTING_MENU_ROW_4,
                                                              SETTING_MENU_ROW_5,
                                                              SETTING_MENU_ROW_6, 
                                                              SETTING_MENU_ROW_7};
#define SETTING_MENU_ROW_H 20


struct settingsTransferValues {
  bool  boBatteryMode;
  bool  boZigBee;
  int   intMeasurementIntervall;
  int   intDisplayRebuild;
  bool  boStartCo2Calibration;
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

SettingsMenuItem CurrentSettingMenuMode = SETTING_MENU_BATTERY;
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
void vdrawStaticSettingsMenu()
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);

        int width = display.width();

        // Header
        drawCenteredText("Settings", 0, 8, width, 24, &FreeMonoBold12pt7b);

        // Settingsmenu rows
        drawCenteredText("Battery mode:",         0, SETTING_MENU_ROW_1, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("Zigbee:",               0, SETTING_MENU_ROW_2, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("Measurement cycle:",    0, SETTING_MENU_ROW_3, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("Screen refresh:",       0, SETTING_MENU_ROW_4, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("CO2 calibration:",      0, SETTING_MENU_ROW_5, width/2, 20, &FreeMonoBold9pt7b);
        drawCenteredText("Temp offset:",          0, SETTING_MENU_ROW_6, width/2, 20, &FreeMonoBold9pt7b);

    } while(display.nextPage());
}

// =====================================================
// Werte aktualisieren
// =====================================================
void vdrawSettingsMenuRow(settingsTransferValues configMenu, SettingsMenuItem row, bool selected)
{
    int w = display.width();
    int y = SETTING_MENU_ROW_Y[row];

    display.setPartialWindow(w/2, y, w/2, SETTING_MENU_ROW_H);
    display.firstPage();
    do {
        display.fillRect(w/2, y, w/2, SETTING_MENU_ROW_H, GxEPD_WHITE);

        const GFXfont* f = selected ? &FreeMonoBold9pt7b : &FreeMono9pt7b;

        switch(row)
        {
            case SETTING_MENU_BATTERY:
                drawCenteredText(configMenu.boBatteryMode ? "Enabled" : "Disabled", w/2, y, w/2, SETTING_MENU_ROW_H, f);
                break;

            case SETTING_MENU_ZIGBEE:
                drawCenteredText(configMenu.boZigBee ? "Enabled" : "Disabled", w/2, y, w/2, SETTING_MENU_ROW_H, f);
                break;

            case SETTING_MENU_REFRESH_MEASUREMENT:
                drawCenteredText(String(configMenu.intMeasurementIntervall).c_str(), w/2, y, w/2, SETTING_MENU_ROW_H, f);
                break;

            case SETTING_MENU_DISPLAY_REBUILD:
                drawCenteredText(String(configMenu.intDisplayRebuild).c_str(), w/2, y, w/2, SETTING_MENU_ROW_H, f);
                break;

            case SETTING_MENU_CO2_CAL:
                drawCenteredText("Start", w/2, y, w/2, SETTING_MENU_ROW_H, f);
                break;

            case SETTING_MENU_TEMP_OFFSET:
                drawCenteredText(String(12.0,1).c_str(), w/2, y, w/2, SETTING_MENU_ROW_H, f);
                break;

            case SETTING_MENU_EXIT:
                drawCenteredText("Exit", w/2, y, w/2, SETTING_MENU_ROW_H, f);
                break;

            default: break;
        }
    } while(display.nextPage());
}

// =====================================================
// Taster Navigation + Longpress
// =====================================================
bool bohandleButton(settingsTransferValues *configMenu,
                  bool *btnWasPressed,
                  unsigned long *pressStart)
{
    bool boReturn = false;

    static unsigned long lastRepeat = 0;

    unsigned long now = millis();
    bool buttonPressed = (digitalRead(BTN_PIN) == HIGH);

    //Button pressed again
    if (buttonPressed && !*btnWasPressed)
    {
        *pressStart = now;
        lastRepeat  = now;
        *btnWasPressed = true;
    }

    //Button hold
    if (buttonPressed && *btnWasPressed)
    {
        unsigned long duration = now - *pressStart;

        //Long press and repeated
        if (duration >= 2000 && (now - lastRepeat) >= 800)
        {
            lastRepeat = now;

            switch (CurrentSettingMenuMode)
            {
                case SETTING_MENU_BATTERY:
                    configMenu->boBatteryMode = !configMenu->boBatteryMode;
                    prefs.begin("system", false);
                    prefs.putBool("BatteryMode", configMenu->boBatteryMode);
                    prefs.end();
                    vdrawSettingsMenuRow(*configMenu, SETTING_MENU_BATTERY, true);
                    break;

                case SETTING_MENU_ZIGBEE:
                    configMenu->boZigBee = !configMenu->boZigBee;
                    prefs.begin("system", false);
                    prefs.putBool("ZigBee", configMenu->boZigBee);
                    prefs.end();
                    vdrawSettingsMenuRow(*configMenu, SETTING_MENU_ZIGBEE, true);
                    break;

                case SETTING_MENU_REFRESH_MEASUREMENT:
                    configMenu->intMeasurementIntervall++;
                    if(configMenu->intMeasurementIntervall > 30)
                    {
                        configMenu->intMeasurementIntervall = 1;
                    }
                    prefs.begin("system", false);
                    prefs.putInt("MeasInt", configMenu->intMeasurementIntervall);
                    prefs.end();
                    vdrawSettingsMenuRow(*configMenu, SETTING_MENU_REFRESH_MEASUREMENT, true);
                    break;

                case SETTING_MENU_DISPLAY_REBUILD:
                    configMenu->intDisplayRebuild = configMenu->intDisplayRebuild + 5;
                    if(configMenu->intDisplayRebuild > 60)
                    {
                        configMenu->intDisplayRebuild = 5;
                    }
                    prefs.begin("system", false);
                    prefs.putInt("DispReb", configMenu->intDisplayRebuild);
                    prefs.end();
                    vdrawSettingsMenuRow(*configMenu, SETTING_MENU_DISPLAY_REBUILD, true);
                    break;

                case SETTING_MENU_CO2_CAL:

                    break;

                case SETTING_MENU_TEMP_OFFSET:
                    vdrawSettingsMenuRow(*configMenu, SETTING_MENU_TEMP_OFFSET, true);
                    break;

                case SETTING_MENU_EXIT:
                    boReturn = true;
                    break;

                default:
                    break;
            }
        }
    }

    //Button released
    if (!buttonPressed && *btnWasPressed)
    {
        unsigned long duration = now - *pressStart;

        //Short press
        if (duration < 1000)
        {
            //Lowlight old row
            vdrawSettingsMenuRow(*configMenu, CurrentSettingMenuMode, false);
            //Set new mode
            CurrentSettingMenuMode = static_cast<SettingsMenuItem>((CurrentSettingMenuMode + 1) % SETTING_MENU_MAX_ROWS);
            //Highlight new row
            vdrawSettingsMenuRow(*configMenu, CurrentSettingMenuMode, true);
        }

        *btnWasPressed = false;
    }
    return boReturn;
}


//
void s_vPrefsHandleFirstRun()
{
    if (!rtcState.rtcInitialized)
    {
        Serial.println("First run after flash"); //@Todo add debug define here
        prefs.begin("system", false);

        if (!prefs.getBool("everInitialized", false))
        {
            //Note: Do not change the naming or anything otherwise first delete all with:
            //prefs.begin("system", false); prefs.clear(); prefs.end();
            prefs.putBool("everInitialized", true);
            prefs.putBool("BatteryMode", true);
            prefs.putBool("ZigBee", false);
            prefs.putInt("MeasInt", 5);
            prefs.putInt("DispReb", 25);
        }

        prefs.end();
        //Set in rtc for cheaper code
        rtcState.rtcInitialized = true;
    }
}

void vExecuteSettingMenu()
{
    //Create struct for transfer values
    settingsTransferValues configMenu;

    //Get values from flash
    prefs.begin("system", false);
    configMenu.boBatteryMode         = prefs.getBool("BatteryMode");
    configMenu.boZigBee              = prefs.getBool("ZigBee");
    configMenu.intMeasurementIntervall  = prefs.getInt("MeasInt");
    configMenu.intDisplayRebuild        = prefs.getInt("DispReb");
    prefs.end();
    //@Todo add here printf to show values from flash

    //Get values from sensor
    configMenu.boStartCo2Calibration = false; //@Todo delete
    configMenu.fTemperatureOffset    = 12.0f; //@ToDo read from sensor

    //Init display
    //@Todo wrap in func
    display.init();
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);

    //Draw static menu headers
    vdrawStaticSettingsMenu();
    for (int iRowMenus = 0; iRowMenus < SETTING_MENU_MAX_ROWS; iRowMenus++)
    {
        vdrawSettingsMenuRow(configMenu, (SettingsMenuItem)iRowMenus, iRowMenus == CurrentSettingMenuMode);
    }

    //Values for detecting short and long press
    bool          boLastPressBtn = false;
    unsigned long lastPressBtn   = 0;
    pinMode(BTN_PIN, INPUT_PULLUP);

    /***********************************************
    *  Execute settings menu
    ***********************************************/
    while(!bohandleButton(&configMenu, &boLastPressBtn, &lastPressBtn));
    //-----------------------------------
}




// =====================================================
// Setup & Loop
// =====================================================
void setup()
{
    //Handle first run
    s_vPrefsHandleFirstRun();

    // Here add menu open
    vExecuteSettingMenu();
}

void loop()
{

}

