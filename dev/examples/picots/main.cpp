// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"
#include "algo.h"
String cardModel = "v3.1";
String breakout = "";
String etrierModel="etrier17";
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(cardModel,breakout);
rtcClass rtc(&preferences);
capteurs cap(&pins, &rtc, SD_MMC, &preferences,etrierModel);
comLORA lora(&pins, &cap);
charger charge(&pins, &rtc, SD_MMC, &preferences, &cap, &server, &ws, &lora);
algo alg;
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

byte message[100];
int ind;
#define addbyte(val){ message[ind]=val; ind++;}
#define add2byte(val){ message[ind]=lowByte(val); ind++; message[ind]=highByte(val); ind++;}
void mainPicot() {
    // init lsm
    // init sick
    // mesure 
    // mettre resultats dans message
    // rafale message
    cap.initSens("lsm");
    cap.initSens("sick");
    cap.mesurePicot(60);
    alg.runFromFile(cap.wf,228.6,4500,cap.newName);
    pins.all_CS_high();
    neopixelWrite(pins.LED, 0, 12, 0);
    int batt = cap.measBatt() * 100;
    ind=0;
    int id = 12;
    add2byte(id);
    add2byte(alg.usureMu);
    add2byte(alg.usuremu);
    add2byte((int)alg.usuremoyu);
    add2byte(alg.usureMd);
    add2byte(alg.usuremd);
    add2byte((int)alg.usuremoyd);
    addbyte( (byte) alg.pat1f*100);
    addbyte( (byte) alg.nPic);
    addbyte( (byte) alg.mincor);
    add2byte( lowByte((int)alg.minsum));
    add2byte(batt);
    for(int i=0;i<45;i++){
        add2byte(alg.probfilb[i]);
    }
    lora.rafale(message, ind, id);
    rtc.goSleep(20);
}

void loop() {
    // LDCTest();
    charge.routinecharge(&mainPicot);
}
