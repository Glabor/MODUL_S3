// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"

String cardModel = "v3.1";
String breakout = "rippersimplev1";
String etrierModel="ripperL17";
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(cardModel,breakout);
rtcClass rtc(&preferences,SD_MMC);
capteurs cap(&pins, &rtc, SD_MMC, &preferences,etrierModel);
comLORA lora(&pins, &cap,&preferences);
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
}
byte message[100];
int ind;
#define addbyte(val){ message[ind]=val; ind++;}
#define add2byte(val){ message[ind]=lowByte(val); ind++; message[ind]=highByte(val); ind++;}
#define add3byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++;}
void mainRipper() {
    preferences.begin("prefid", false);
    int id=preferences.getUInt("id",-1);
    if(id==-1){
        return;
    }
    bool waitingtrans = preferences.getBool("WAITINGTRANS",false);//pas de mesure en attente de transmission
    float w=cap.rot->wheelRot2();
    float batvolt = cap.measBatt();
    rtc.log(batvolt, waitingtrans, w);
    int sleepNoMeas =preferences.getUInt("sleepNoMeas",30);
    int transTime =preferences.getUInt("transTime",0);
    int measTime =preferences.getUInt("measTime",0);
    int sleepMeas =preferences.getUInt("sleepMeas",8);
    if(w<0.1*2*M_PI/60){//rotation <0.1rpm
        preferences.end();
        lora.rfSend("sleeping");
        if(waitingtrans){
            rtc.goSleepMinuteFixe(sleepNoMeas,transTime);
        }
        else{
            rtc.goSleepMinuteFixe(sleepNoMeas,measTime);
        }
        
    }
    pins.all_CS_high();
    if(waitingtrans){
        int batt = cap.measBatt() * 100;
        neopixelWrite(pins.LED, 0, 12, 0);
        add2byte(id);
        add3byte(preferences.getLong("f1Max1",0));
        add3byte(preferences.getLong("f1Min1",0));
        add3byte(preferences.getLong("f1moy1",0));
        add3byte(preferences.getLong("f2Max1",0));
        add3byte(preferences.getLong("f2Min1",0));
        add3byte(preferences.getLong("f2moy1",0));
        if(breakout == "ripperdoublev1"){
            add3byte(preferences.getLong("f1Max2",0));
            add3byte(preferences.getLong("f1Min2",0));
            add3byte(preferences.getLong("f1moy2",0));
            add3byte(preferences.getLong("f2Max2",0));
            add3byte(preferences.getLong("f2Min2",0));
            add3byte(preferences.getLong("f2moy2",0));
        }
        add2byte(batt);
        lora.rafale(message, 14, id);
        preferences.putBool("waitingtrans",false);
        preferences.end();
        rtc.goSleepHeureFixe(sleepMeas,measTime);
    }
    else{
        neopixelWrite(pins.LED, 0, 0, 12);
        cap.mesureRipper(10,"LDC1");
        if(cap.ldc1->count>0){
            preferences.putLong("f1Max1",cap.ldc1->f1Max);
            preferences.putLong("f1Min1",cap.ldc1->f1Min);
            preferences.putLong("f1moy1",cap.ldc1->f1moy);
            preferences.putLong("f2Max1",cap.ldc1->f2Max);
            preferences.putLong("f2Min1",cap.ldc1->f2Min);
            preferences.putLong("f2moy1",cap.ldc1->f2moy);
        }
        if(breakout == "ripperdoublev1"){
            cap.mesureRipper(10,"LDC2");
            if(cap.ldc2->count>0){
                preferences.putLong("f1Max2",cap.ldc1->f1Max);
                preferences.putLong("f2Min2",cap.ldc1->f1Min);
                preferences.putLong("f1moy2",cap.ldc1->f1moy);
                preferences.putLong("f2Max2",cap.ldc1->f2Max);
                preferences.putLong("f2Min2",cap.ldc1->f2Min);
                preferences.putLong("f2moy2",cap.ldc1->f2moy);
            }
        }
        preferences.putBool("waitingtrans",true);
        preferences.end();
        rtc.goSleepMinuteFixe(0,transTime);
    }
}
void loop() {
    // LDCTest();
    charge.routinecharge(&mainRipper);
}
