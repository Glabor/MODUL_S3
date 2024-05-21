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

void mainTask() {
    Serial.println("main");
    pins.rainbowLoop(10);
}

void loop() {
    charge.routinecharge(&mainTask);
}
