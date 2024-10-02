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
rtcClass rtc(&preferences,SD_MMC);
capteurs cap(&pins, &rtc, SD_MMC, &preferences,etrierModel);
comLORA lora(&pins, &cap,&preferences);
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
    float w=cap.rot->wheelRot2();
    randomSeed(analogRead(pins.SICK1));
    w=((float)random(0,2))*0.5*2*M_PI/60;
    float batvolt = cap.measBatt();
    rtc.log(batvolt, waitingtrans, w);
    int sleepNoMeas =preferences.getUInt("sleepNoMeas",30);
    int transTime =preferences.getUInt("transTime",0);
    int measTime =preferences.getUInt("measTime",0);
    int sleepMeas =preferences.getUInt("sleepMeas",8);
    preferences.end();
    if(abs(w)<0.1*2*M_PI/60){//rotation <0.1rpm
        
        lora.rfSend("sleeping");
        if(waitingtrans){
            //rtc.goSleepMinuteFixe(sleepNoMeas,transTime);
            rtc.goSleep(60);
        }
        else{
            //rtc.goSleepMinuteFixe(sleepNoMeas,measTime);
            rtc.goSleep(60);
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
        if(alg.runFromFile(w,r,R,path)){
            pins.all_CS_high();
            int batt = cap.measBatt() * 100;
            ind=0;
           preferences.begin("prefid", false);
            addbyte(id);
            addbyte(77); //"M"
            add4byte(preferences.getLong("timestamp",0));
            add2byte(batt);
            add2byte((int)preferences.getFloat("ROTSPEED",0)*100);
            add2byte((int)preferences.getFloat("ROTSPEEDF",0)*100);
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
            for(int i=0;i<45;i++){
                addbyte(alg.probfilb[i]);
            }
            preferences.end();
            lora.rafale(message, ind, id);
        }
        else{
            File logFile = SD_MMC.open("/log.txt", FILE_APPEND);
            if (logFile) {
                logFile.println(alg.error);
            }
            logFile.close();
        }
        //rtc.goSleepHeureFixe(sleepMeas,measTime);
        rtc.goSleep(60);
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
        preferences.putBool("waitingtrans",true);
        preferences.end();
        //rtc.goSleepMinuteFixe(0,transTime);
        rtc.goSleep(60);
    }
}

void loop() {
    // LDCTest();
    charge.routinecharge(&mainPicot);
}
