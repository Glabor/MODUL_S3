// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"

String cardModel = "v3.1";
String breakout = "HMCv2";
String etrierModel = "ripperL17";

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(cardModel, breakout);
rtcClass rtc(&preferences, SD_MMC);
capteurs cap(&pins, &rtc, SD_MMC, &preferences, etrierModel);
comLORA lora(&pins, &cap, &preferences);
charger charge(&pins, &rtc, SD_MMC, &preferences, &cap, &server, &ws, &lora);

void setup() {
    Serial.begin(115200);
    Serial.println("begin");

    pins.pinSetup();
    delay(1000);
    cap.pinSetup();
    lora.pinSetup();
    // lora.rf95Setup();
    pins.sdmmcSetup();
    rtc.rtcSetup();
    cap.lsmSetup();
    // cap.adxlSetup();
    charge.initSPIFFS();
    charge.initWebSocket();
    charge.setup();
    cap.measBatt();
}

void mainHMC() {
    Serial.println("main");
    // charge.sendSens("start");
    cap.type = "hmc";
    String sensName = cap.getName(cap.type);
    int measTime = 10;
    cap.HW->measureHMR(measTime, sensName);
    cap.newName = sensName;
    if (charge.wifiConnect()) {
        charge.sendSens(cap.type);
    }
    delay(2000);
    WiFi.disconnect(true);
}

void loop() {
    // LDCTest();
    charge.routinecharge(&mainHMC);
}