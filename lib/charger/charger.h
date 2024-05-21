#ifndef CHARGER_H
#define CHARGER_H

#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WiFi.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <ElegantOTA.h>
#include <HTTPClient.h>
#include <RTClib.h>
// #include "pinout.h"
#include "capteurs.h"
#include <Preferences.h>
#include <SD_MMC.h>
// #include "rtcClass.h"
#include "comLORA.h"
class charger {
public:
    charger(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr, capteurs *c, AsyncWebServer *s, AsyncWebSocket *w, comLORA *l);
    void routinecharge(void (*task)(void));
    bool initSPIFFS();
    void setup();
    void initWebSocket();
    void serverRoutes();
    bool wifiConnect();
    int sendSens(String type);
    int httpPostRequest(String serverName, String postText);
    String host = "http://LAPTOP-TF0BBSC1:5000";

private:
    pinout *pins;
    rtcClass *rtc;
    fs::FS *fs;
    Preferences *preferences;
    capteurs *cap;
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    static void handleFileList(AsyncWebServerRequest *request, String folderPath);
    void syncRTC();
    String printLocalTime();
    String processor(const String &var);
    int sendFlask();
    bool manageCOM();
    void loopWS();
    int manageLoop();
    void normalTask();

    // Replace with your network credentials
    String ssid = "GL-AR300M-c40";
    String password = "goodlife";

    String soft_ap_ssid = "MyESP32AP";
    String soft_ap_password = "testpassword";

    const char *ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 0;
    const int daylightOffset_sec = 0;

    int blink = 0;
    int printInt = 0;
    bool bLSM = false;
    bool bS_LSM = false;
    bool bS_ADXL = false;
    bool bS_SICK = false;
    bool bLora = false;
    bool bWifi = true;
    bool bBoot0 = false;
    bool bBlink = false;
    // unsigned int to = 2;
    // String uid = "04 71 F1 79 B6 2A 81";
    //  Create AsyncWebServer object on port 80
    //  AsyncWebServer* server;
    AsyncWebServer *server = nullptr;
    AsyncWebSocket *ws = nullptr;
    JsonDocument prints;
    comLORA *lora;
    long loopTO = 0;
    long wsTO = 0;
    long blinkTO = 0;
    int wsDelay = 100;
    bool taskDone = false;
    void callbaque(AsyncWebServerRequest *request);
};

#endif