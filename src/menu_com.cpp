#include <Arduino.h>

#include <config.h>

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>

#include <menu.h>
#include <menuIO/u8g2Out.h>

#include "AiEsp32RotaryEncoder.h"

#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>

#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>

StaticJsonDocument<1024> doc;

#define PIN_LED LED_BUILTIN
#define SCREEN_SDA D2
#define SCREEN_SCL D1

#define USE_SSD1306

#define ROTARY_ENCODER_A_PIN D5
#define ROTARY_ENCODER_B_PIN D6
#define ROTARY_ENCODER_BUTTON_PIN D7
#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 4
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

#if defined(USE_SSD1306)

#include <Wire.h>
#define fontName u8g2_font_7x13_t_cyrillic //u8g2_font_7x13_mf
#define fontX 7
#define fontY 16
#define offsetX 0
#define offsetY 3
#define U8_Width 128
#define U8_Height 64
#define USE_HWI2C
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);//, SCL, SDA);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, 4, 5);
// U8G2_SSD1306_128X64_VCOMH0_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, 4, 5);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCREEN_SDA, SCREEN_SCL);
#else
#error DEFINE YOUR OUTPUT HERE.
#endif

const colorDef<uint8_t> colors[6] MEMMODE = {
    {{0, 0}, {0, 1, 1}}, // bgColor
    {{1, 1}, {1, 0, 0}}, // fgColor
    {{1, 1}, {1, 0, 0}}, // valColor
    {{1, 1}, {1, 0, 0}}, // unitColor
    {{0, 1}, {0, 0, 1}}, // cursorColor
    {{1, 1}, {1, 0, 0}}, // titleColor
};

result doAlert(eventMask e, prompt &item);

int test = 55;
int nextMenuReload = 0;
int encoderDownStarted = 0;
String nextMenuURLToLoad = "";
String latestMenuURL = "";
bool preventMenuAction = true;

// //customizing a prompt look!
// //by extending the prompt class
// class altPrompt:public prompt {
// public:
//   altPrompt(constMEM promptShadow& p):prompt(p) {}
//   Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t panelNr) override {
//     return out.printRaw(F("special prompt!"),len);;
//   }
// };

// define a pad style menu (single line menu)
// here with a set of fields to enter a date in YYYY/MM/DD format

char *constMEM hexDigit MEMMODE = "0123456789ABCDEF";
char *constMEM hexNr[] MEMMODE = {"0", "x", hexDigit, hexDigit};
char buf1[] = "0x11";

#define MAX_DEPTH 2

RotaryEventIn reIn(
    RotaryEventIn::EventType::BUTTON_CLICKED |        // select
    RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED | // back
    RotaryEventIn::EventType::BUTTON_LONG_PRESSED |   // also back
    RotaryEventIn::EventType::ROTARY_CCW |            // up
    RotaryEventIn::EventType::ROTARY_CW               // down
);

serialIn serial(Serial);

void buzzerClick()
{
#ifdef BUZZER_PIN
    tone(BUZZER_PIN, 1100);
    delay(100);
    noTone(BUZZER_PIN);
#endif
}

void buzzerEnter()
{
#ifdef BUZZER_PIN
    tone(BUZZER_PIN, 1100);
    delay(100);
    noTone(BUZZER_PIN);
    delay(125);
    tone(BUZZER_PIN, 1100);
    delay(100);
    noTone(BUZZER_PIN);
#endif
}

void ledOn() {
    digitalWrite(LEDPIN, LOW);
}

void ledOff() {
    digitalWrite(LEDPIN, HIGH);
}

MENU_INPUTS(in, &serial, &reIn);
MENU_OUTPUTS(out, MAX_DEPTH, U8G2_OUT(u8g2, colors, fontX, fontY, offsetX, offsetY, {0, 0, U8_Width / fontX, U8_Height / fontY}), SERIAL_OUT(Serial));

result alert(menuOut &o, idleEvent e)
{
    if (e == idling)
    {
        o.setCursor(0, 0);
        o.print("alert test");
        o.setCursor(0, 1);
        o.print("press [select]");
        o.setCursor(0, 2);
        o.print("to continue...");
    }
    return proceed;
}

// when menu is suspended
result idle(menuOut &o, idleEvent e)
{
    o.clear();
    switch (e)
    {
    case idleStart:
        o.println("suspending menu!");
        break;
    case idling:
        o.println("suspended...");
        break;
    case idleEnd:
        o.println("resuming menu.");
        break;
    }
    return proceed;
}

#define MAX_DATA_SIZE 4
int currentDataSize = 4;

prompt *mainData[MAX_DATA_SIZE] = {
    new prompt("Op 2"), // we can set/change text, function and event mask latter
    new menuField<typeof(test)>(test, "Bright", "", 0, 255, 10, 1, doNothing, noEvent),
    new textField("Addr", buf1, 4, hexNr),
    new Exit("<Exit.")};

menuNode &mainMenu = *new menuNode("Main menu", sizeof(mainData) / sizeof(prompt *),
                                   mainData /*,doNothing,noEvent,wrapStyle*/);

NAVROOT(nav, mainMenu, MAX_DEPTH, in, out);

String fetchURL(String URL)
{
    ledOn();
    Serial.print("Fetching URL: ");
    Serial.println(URL);
    String payload = "";
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, URL.c_str());
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0)
        {
            payload = http.getString();
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        // Free resources
        http.end();
    }
    ledOff();
    return payload;
}

void setNextMenuToLoad(String url)
{
    nextMenuURLToLoad = url;
}

void setMenuReloadTimer(int timeout)
{
    Serial.print("Reload timer: ");
    Serial.println(timeout);
    if (timeout > 0)
    {
        int currentTime = millis() / 1000;
        nextMenuReload = currentTime + timeout;
    }
    else
    {
        nextMenuReload = 0;
    }
}

void checkMenuReloadTimer()
{
    if (nextMenuReload == 0 || latestMenuURL == "")
        return;

    int currentTime = millis() / 1000;
    if (nextMenuReload <= currentTime)
    {
        Serial.println("Reloading menu");
        setNextMenuToLoad(latestMenuURL);
        nextMenuReload = 0;
    }
}

result menuCallback(eventMask e, navNode &nav, prompt &p)
{
    if (preventMenuAction)
    {
        return result::proceed;
    }
    switch (e)
    {
    case enterEvent:
    {
        String serverURL = SERVER_URL;
        Serial.print("Menu clicked: ");
        String itemClickedTitle = p.getText();
        Serial.println(itemClickedTitle);
        int itemsCount = doc["items"].size();
        if (itemsCount > MAX_DATA_SIZE)
            itemsCount = MAX_DATA_SIZE;
        for (int i = 0; i < itemsCount; i++)
        {
            if (doc["items"][i]["title"] == itemClickedTitle)
            {
                if (doc["items"][i]["request"])
                {
                    Serial.print("Doing HTTP request: ");
                    String url = doc["items"][i]["request"];
                    Serial.println(url);
                    if (url.startsWith("http://") || url.startsWith("https://"))
                    {
                        String content = fetchURL(url);
                    }
                    else
                    {
                        String content = fetchURL(serverURL + url);
                    }
                    
                }
                if (doc["items"][i]["url"])
                {
                    Serial.print("Go to menu: ");
                    String url = doc["items"][i]["url"];
                    Serial.println(url);
                    if (url.startsWith("http://") || url.startsWith("https://"))
                    {
                        setNextMenuToLoad(url);
                    }
                    else
                    {
                        setNextMenuToLoad(serverURL + url);
                    }
                }
                buzzerEnter();
            }
        }
        break;
    }
    case focusEvent:
    {
        buzzerClick();
    }
    }

    return result::proceed;
}

void buildMenuFromJSON(String content)
{
    Serial.println("Building menu from JSON.");
    DeserializationError error = deserializeJson(doc, content);
    if (error)
        Serial.println(F("Failed to read file, using default configuration"));

    const int menuReload = doc["refresh"] | 0;
    setMenuReloadTimer(menuReload);

    const char *menuTitle = doc["title"] | "Untitled";
    int itemsCount = doc["items"].size();
    if (itemsCount > MAX_DATA_SIZE)
        itemsCount = MAX_DATA_SIZE;
    for (int i = 0; i < itemsCount; i++)
    {
        mainData[i] = new prompt(doc["items"][i]["title"] | "Item " + i, menuCallback, enterEvent | focusEvent, wrapStyle);
    }
    mainMenu = *new menuNode(menuTitle, itemsCount, mainData /*,doNothing,noEvent,wrapStyle*/);
    preventMenuAction = true;
    nav.doNav(navCmd(idxCmd, 0));
    preventMenuAction = false;
    mainMenu.dirty = true;
}

void loadMenuByURL(String URL)
{
    latestMenuURL = URL;
    buildMenuFromJSON(fetchURL(URL));
}

void checkNextMenuToLoad()
{
    if (nextMenuURLToLoad.startsWith("http"))
    {
        String url = nextMenuURLToLoad;
        nextMenuURLToLoad = "";
        loadMenuByURL(url);
    }
}

void IRAM_ATTR readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();
}

void rotary_onButtonClick()
{
    static unsigned long lastTimePressed = 0;
    if (millis() - lastTimePressed < 500)
    {
        return;
    }
    lastTimePressed = millis();
    reIn.registerEvent(RotaryEventIn::EventType::BUTTON_CLICKED);
}

void rotary_loop()
{
    static unsigned long prevValue;
    unsigned long newValue;
    if (rotaryEncoder.encoderChanged())
    {
        newValue = rotaryEncoder.readEncoder();
        if (prevValue > newValue)
        {
            reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CCW);
        }
        else
        {
            reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CW);
        }
        prevValue = newValue;
        encoderDownStarted = 0;
    }
    if (rotaryEncoder.isEncoderButtonClicked())
    {
        rotary_onButtonClick();
        encoderDownStarted = 0;
    }
    if (rotaryEncoder.isEncoderButtonDown()) {
        int timeNow = round(millis() / 1000);
        if (encoderDownStarted>0) {
           if ((timeNow-encoderDownStarted)>5) {
            ESP.restart();
           }
        } else {
            encoderDownStarted = timeNow;
        }
    } else {
        encoderDownStarted = 0;
    }
}

result doAlert(eventMask e, prompt &item)
{
    nav.idleOn(alert);
    return proceed;
}

void startMenu()
{

#ifdef BUZZER_PIN
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);
#endif

    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);

    bool circleValues = false;
    rotaryEncoder.setBoundaries(0, 1000, circleValues);

    // rotaryEncoder.setAcceleration(250);
    rotaryEncoder.disableAcceleration();

    Serial.println("menu 4.x test");
    Serial.flush();
    pinMode(LEDPIN, OUTPUT);

#if defined(USE_HWSPI)
    SPI.begin();
    u8g2.begin();
#elif defined(USE_HWI2C)
    Wire.begin();
    u8g2.begin();
#else
#error "please choose your interface (I2c,SPI)"
#endif

    u8g2.setFont(fontName);
    // u8g2.setBitmapMode(0);

    // disable second option
    // mainMenu[0].enabled = disabledStatus;

    nav.idleTask = idle; // point a function to be used when menu is suspended
                         // nav.showTitle = false;

    String serverURL = SERVER_URL;
    String startMenu = START_MENU;
    setNextMenuToLoad(serverURL + startMenu);
}

void handleMenu()
{
    checkNextMenuToLoad();
    checkMenuReloadTimer();
    nav.doInput();
    if (nav.changed(0))
    {
        u8g2.firstPage();
        do
            nav.doOutput();
        while (u8g2.nextPage());
    }
    rotary_loop();
}