// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"

String model = "v3.1";

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(model);
rtcClass rtc;
capteurs cap(&pins, &rtc, SD_MMC, &preferences);
comLORA lora(&pins);
charger charge(&pins, &rtc, SD_MMC, &preferences, &cap, &server, &ws, &lora);

void setup() {
    Serial.begin(115200);
    Serial.println("begin");

    pins.pinSetup();
    cap.pinSetup();
    lora.pinSetup();
    pins.sdmmcSetup();
    rtc.rtcSetup();
    cap.lsmSetup();
    cap.adxlSetup();
    lora.rf95Setup();
    charge.initSPIFFS();
    charge.initWebSocket();
    charge.setup();
    cap.measBatt();
}

void sendRSSI() {
    pins.rainbowLoop(10);
    int milli0 = millis();
    while (millis() - milli0 < 30e3) {
        int responseCode = -2;
        if (charge.wifiConnect()) {
            int RSSI = WiFi.RSSI();
            JsonDocument info;
            info["RSSI"] = RSSI;
            info["millis"] = millis() - milli0;
            String infoPost;
            serializeJson(info, infoPost);
            Serial.println(infoPost);
            responseCode = charge.httpPostRequest(charge.host + "/rssi", infoPost);
        }
        Serial.print("response : ");
        Serial.println(responseCode);
        delay(500);
    }
}

void mainTask() {
    Serial.println("main");
    // digitalWrite(pins.ON_SICK, HIGH);
    // pins.rainbowLoop(10);
    cap.type = "sick";
    int genVar = 10;
    cap.saveSens(cap.type, genVar);
    if (charge.wifiConnect()) {
        charge.sendSens(cap.type);
    }
    delay(2000);
    WiFi.disconnect(true);

    cap.type = "adxl";
    genVar = 10;
    cap.saveSens(cap.type, genVar);
    if (charge.wifiConnect()) {
        charge.sendSens(cap.type);
    }
    delay(2000);
    WiFi.disconnect(true);
}

void loop() {
    charge.routinecharge(&mainTask);
}
