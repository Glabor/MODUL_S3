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
rtcClass rtc(&preferences);
capteurs cap(&pins, &rtc, SD_MMC, &preferences);
comLORA lora(&pins, &cap);
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

void mainRipper() {
    // check wheel rot speed
    //  init LDC
    //  mesure LDC
    //  mettre resultats dans message
    //  rafale message

    int beginRotW = cap.wheelRot(2000) / M_PI * 30;
    Serial.println(beginRotW);
    int id = cap.id;

    if (beginRotW > 100) {
        Serial.println("turning");

        long max;
        long min;
        long moy;
        max = 30;
        min = 10;
        moy = 20;
        digitalWrite(pins.RFM95_CS, HIGH);
        digitalWrite(pins.ADXL375_CS, HIGH);
        cap.LDC_LHRSetup();
        cap.LDC_LHRMesure(&max, &min, &moy, 3000);
        neopixelWrite(pins.LED, 12, 12, 0);
        delay(50);

        byte message[30];
        int batt = cap.measBatt() * 100;
        int rtcTemp = rtc.rtc.getTemperature() * 10;
        long tsMeas = rtc.rtc.now().unixtime();
        int bufIndex = 0;
        for (size_t i = 0; i < 4; i++) {
            message[i] = lowByte(tsMeas >> 8 * (3 - i));
            bufIndex++;
        }
        message[bufIndex++] = lowByte(id);
        message[bufIndex++] = lowByte(batt);
        message[bufIndex++] = highByte(batt);
        message[bufIndex++] = lowByte(rtcTemp);
        message[bufIndex++] = highByte(rtcTemp);
        message[bufIndex++] = lowByte(beginRotW);
        message[bufIndex++] = highByte(beginRotW);
        message[bufIndex++] = lowByte(min >> 8 * 2);
        message[bufIndex++] = lowByte(min >> 8 * 1);
        message[bufIndex++] = lowByte(min >> 8 * 0);
        message[bufIndex++] = lowByte(max >> 8 * 2);
        message[bufIndex++] = lowByte(max >> 8 * 1);
        message[bufIndex++] = lowByte(max >> 8 * 0);
        message[bufIndex++] = lowByte(moy >> 8 * 2);
        message[bufIndex++] = lowByte(moy >> 8 * 1);
        message[bufIndex++] = lowByte(moy >> 8 * 0);
        lora.rafale(message, bufIndex, id);
        // message[0] = lowByte(7);
        // message[1] = highByte(7);
        // message[2] = lowByte(beginRotW);
        // message[3] = highByte(beginRotW);
        // lora.rafale(message, 4, 7);
        lora.rfSend("sleep turning");
        rtc.syncSleep(2, id);
    } else {
        Serial.println("still");
        lora.rfSend("sleeping still");
        rtc.syncSleep(1, id);
    }
}

void mainTask() {
    Serial.println("main");
    // digitalWrite(pins.ON_SICK, HIGH);
    // pins.rainbowLoop(10);
    cap.type = "sick";
    cap.saveSens(cap.type, 10);
    if (charge.wifiConnect()) {
        charge.sendSens(cap.type);
    }
    delay(2000);
    WiFi.disconnect(true);

    cap.type = "adxl";
    cap.saveSens(cap.type, 10);
    if (charge.wifiConnect()) {
        charge.sendSens(cap.type);
    }
    delay(2000);
    WiFi.disconnect(true);
}

void loop() {
    // LDCTest();
    charge.routinecharge(&mainRipper);
}