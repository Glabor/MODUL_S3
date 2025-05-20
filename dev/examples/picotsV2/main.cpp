// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"
#include "algoPicots.h"
String cardModel = "v3.1";
String breakout = "";
String etrierModel="etrier17";
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(cardModel,breakout);
rtcClass rtc(&preferences,SD_MMC);
capteurs cap(&pins, &rtc, SD_MMC, &preferences,etrierModel);
comLORA lora(&pins, &cap,&preferences);
charger charge(&pins, &rtc, SD_MMC, &preferences, &cap, &server, &ws, &lora);
algoPicots alg;
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

byte message[150];
int ind;
#define addbyte(val){ message[ind]=val; ind++;}
#define add2byte(val){ message[ind]=lowByte(val); ind++; message[ind]=highByte(val); ind++;}
#define add4byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++; message[ind]=lowByte(val>>24); ind++;}

void mainPicot() {
    preferences.begin("prefid", false);
    int id=preferences.getUInt("id",-1);
    if(id==-1){
        return;
    }
    bool waitingtrans = preferences.getBool("waitingtrans",false);//pas de mesure en attente de transmission
    preferences.end();
    float w=cap.rot->wheelRot2();
    randomSeed(analogRead(pins.SICK1));
    w=((float)random(0,2))*4*M_PI/60;
    float batvolt = cap.measBatt();
    preferences.begin("prefid", false);
    int sleepNoMeas =preferences.getUInt("sleepNoMeas",30);
    int transTime =preferences.getUInt("transTime",id);
    int measTime =preferences.getUInt("measTime",0);
    int sleepMeas =preferences.getUInt("sleepMeas",4);
    preferences.end();
    rtc.log(batvolt, waitingtrans, w);
    if(abs(w)<0.5*2*M_PI/60){//rotation <0.5rpm
        
        lora.rfSend("sleeping"+String(batvolt)+","+String(rtc.rtc.getTemperature()));
        if(waitingtrans){
            lora.rfSend("cannot transmit "+String(abs(w)*30/M_PI)+"rpm");
            rtc.goSleepMinuteFixe(sleepNoMeas,transTime);
        }
        else{
           lora.rfSend("cannot measure "+String(abs(w)*30/M_PI)+"rpm");
           rtc.goSleepMinuteFixe(sleepNoMeas,measTime);
        }    
    }
    pins.all_CS_high();
    if(waitingtrans){
        neopixelWrite(pins.LED, 0, 12, 0);
        preferences.begin("prefid", false);
        preferences.putBool("waitingtrans",false);
        float w=preferences.getFloat("ROTSPEED",1);
        int r=preferences.getUInt("RAYONMOLETTE",229);
        int R=preferences.getUInt("radius",4500);
        String path=preferences.getString("NEWNAME","");
        preferences.end();
        if(alg.runFromFile(0,r,R,path)){
            pins.all_CS_high();
            int batt = cap.measBatt() * 100;
            ind=0;
           preferences.begin("prefid", false);
            addbyte(id);
            addbyte(77); //"M"
            add4byte(preferences.getLong("timestamp",0));   
            add2byte((int)(preferences.getFloat("ROTSPEED",0)*100));
            add2byte((int)(preferences.getFloat("ROTSPEEDF",0)*100));
            add2byte(alg.usure->usureMu);
            add2byte(alg.usure->usuremu);
            add2byte((int)alg.usure->usuremoyu);
            add2byte(alg.usure->usureMd);
            add2byte(alg.usure->usuremd);
            add2byte((int)alg.usure->usuremoyd);
            addbyte( (byte) (alg.patinage->pat1f*100));
            addbyte( (byte) alg.comptage->nPic);
            addbyte( (byte) alg.comptage->mincor);
            add2byte( lowByte((int)alg.comptage->minsum));
            for(int i=0;i<45;i++){
                addbyte(alg.patinage->probfilb[i]);
            }
            add2byte((int)(preferences.getFloat("rtcTemp",0)*10));
            add2byte(batt);
            preferences.end();
            lora.rafale(message, ind, id);
        }
        else{
            File logFile = SD_MMC.open("/log.txt", FILE_APPEND);
            if (logFile) {
                logFile.println(alg.error);
            }
            logFile.close();
            lora.rfSend(alg.error);
        }
        rtc.goSleepHeureFixe(sleepMeas,measTime);
    }
    else{
        neopixelWrite(pins.LED, 0, 0, 12);
        cap.initSens("lsm");
        cap.initSens("sick");
        long timestamp=rtc.rtc.now().unixtime();
        preferences.begin("prefid", false);
        preferences.putLong("timestamp",timestamp);
        int duration=preferences.getUInt("sleep",60);
        preferences.end();
        cap.mesurePicot(duration);
        preferences.begin("prefid", false);
        preferences.putString("NEWNAME",cap.newName);
        preferences.putFloat("ROTSPEED",w);
        preferences.putFloat("ROTSPEEDF",cap.wf);
        float RTCtemp=rtc.rtc.getTemperature();
        preferences.putFloat("rtcTemp",RTCtemp);
        preferences.putBool("waitingtrans",true);
        preferences.end();
        if(transTime==measTime){
            rtc.safeRestart();
        }
        lora.rfSend("measure done");
        rtc.goSleepMinuteFixe(0,transTime);
    }
}

void loop() {
    // LDCTest(); //test push (GLE)
    charge.routinecharge(&mainPicot);
}
