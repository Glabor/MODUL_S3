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
    preferences.begin("prefid", false);
    int id=preferences.getUInt("id",-1);
    if(id==-1){
        return;
    }
    bool waitingtrans = preferences.getBool("WAITINGTRANS",false);//pas de mesure en attente de transmission
    float w=cap.rot->wheelRot2();
    if(w<0.1*2*M_PI/60){//rotation <0.1rpm
        if(waitingtrans){
            rtc.goSleep2(preferences.getUInt("sleepNoMeas",10000),preferences.getUInt("transTime",0));
        }
        else{
            rtc.goSleep2(preferences.getUInt("sleepNoMeas",10000),preferences.getUInt("measTime",0));
        }
        lora.rfSend("sleeping");
    }
    pins.all_CS_high();
    if(waitingtrans){
        alg.runFromFile(preferences.getFloat("ROTSPEED",1),preferences.getUInt("RAYONMOLETTE",229),preferences.getUInt("radius",4500),preferences.getString("NEWNAME",""));
        pins.all_CS_high();
        neopixelWrite(pins.LED, 0, 12, 0);
        int batt = cap.measBatt() * 100;
        ind=0;
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
            addbyte(alg.probfilb[i]);
        }
        lora.rafale(message, ind, id);
        preferences.putBool("waitingtrans",false);
        rtc.goSleep2(preferences.getUInt("sleepMeas",10000),preferences.getUInt("measTime",0));
    }
    else{
        neopixelWrite(pins.LED, 0, 0, 12);
        cap.initSens("lsm");
        cap.initSens("sick");
        cap.mesurePicot(preferences.getUInt("measDuration",60));
        preferences.putString("NEWNAME",cap.newName);
        preferences.putFloat("ROTSPEED",cap.wf);
        preferences.putBool("waitingtrans",true);
        rtc.goSleep2(0,preferences.getUInt("transTime",0));
    }
    preferences.end();
}

void loop() {
    // LDCTest();
    charge.routinecharge(&mainPicot);
}
