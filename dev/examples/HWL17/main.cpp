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
String etrierModel = "hmcL17";

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
byte message[100];
int ind;
#define addbyte(val){ message[ind]=(byte)val; ind++;}
#define add2byte(val){ message[ind]=lowByte(val); ind++; message[ind]=highByte(val); ind++;}
#define add3byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++;}
#define add4byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++; message[ind]=lowByte(val>>24); ind++;}
void mainHMC() {
    preferences.begin("prefid", false);
    int id=preferences.getUInt("id",-1);
    if(id==-1){
        return;
    }
    preferences.end();
    float w=cap.rot->wheelRot2();
    //randomSeed(analogRead(pins.SICK1));
    //w=((float)random(0,2))*0.5*2*M_PI/60;
    float batvolt = cap.measBatt();
    rtc.log(batvolt, false, w);
    preferences.begin("prefid", false);
    int sleepNoMeas =preferences.getUInt("sleepNoMeas",30);
    int transTime =preferences.getUInt("transTime",id);
    int measTime =preferences.getUInt("measTime",0);
    int sleepMeas =preferences.getUInt("sleepMeas",8);
    preferences.end();
    if(abs(w)<2*M_PI/60){//rotation <1rpm
        lora.rfSend("cannot measure " + String(abs(w) * 30 / M_PI) + "rpm");
        rtc.goSleepMinuteFixe(sleepNoMeas,measTime);
    }
    pins.all_CS_high();
    preferences.begin("prefid", false);
    int duration=preferences.getUInt("sleep",60);
    preferences.end();
    neopixelWrite(pins.LED, 0, 0, 12);
    cap.mesureLisse(duration);
    lora.rfSend("measure done");
    rtc.goSleepHeureFixe(sleepMeas,measTime);
}
void loop() {
    charge.routinecharge(&mainHMC);
}