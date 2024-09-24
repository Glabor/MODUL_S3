// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"

String cardModel = "v3.1";
String breakout = "ripperv1";
String etrierModel="ripperL17";
Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(cardModel,breakout);
rtcClass rtc(&preferences,SD_MMC);
capteurs cap(&pins, &rtc, SD_MMC, &preferences,etrierModel);
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
}
byte message[100];
int ind;
#define addbyte(val){ message[ind]=val; ind++;}
#define add2byte(val){ message[ind]=lowByte(val); ind++; message[ind]=highByte(val); ind++;}
#define add3byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++;}
void mainRipper() {
    int id=preferences.getUInt("id",-1);
    if(id==-1){
        return;
    }
    bool waitingtrans = preferences.getBool("WAITINGTRANS",false);//pas de mesure en attente de transmission
    float w=cap.rot->wheelRot2();
    float batvolt = cap.measBatt();
    float RTCtemp=rtc.rtc.getTemperature();
    File logFile = SD_MMC.open("log.txt", FILE_WRITE);
    if (logFile) {
        logFile.print("\n");
        logFile.print("ON : ");
        logFile.println(rtc.dateRTC(rtc.rtc.now()));
        logFile.print("bat");
        logFile.print(String(batvolt));
        logFile.println("V");
        logFile.print("RTC temp: ");
        logFile.println(String(RTCtemp));
        logFile.print("RDC speed: ");
        logFile.print(String(w/2/M_PI*60));
        logFile.println("rpm");
        if(w<0.1*2*M_PI/60){logFile.println("back to sleep");}
        else{
            if(waitingtrans){
                logFile.println("begin transmission");
            }
            else{
                logFile.println("begin measurement");
            }
        }
    }
    logFile.flush();
    logFile.close();
    if(w<0.1*2*M_PI/60){//rotation <0.1rpm
        if(waitingtrans){
            rtc.goSleepMinuteFixe(preferences.getUInt("sleepNoMeas",30),preferences.getUInt("transTime",0));
        }
        else{
            rtc.goSleepMinuteFixe(preferences.getUInt("sleepNoMeas",30),preferences.getUInt("measTime",0));
        }
        lora.rfSend("sleeping");
    }
    pins.all_CS_high();
    if(waitingtrans){
        int batt = cap.measBatt() * 100;
        neopixelWrite(pins.LED, 0, 12, 0);
        add2byte(id);
        add3byte(preferences.getLong("f1Max",0));
        add3byte(preferences.getLong("f1Min",0));
        add3byte(preferences.getLong("f1moy",0));
        add3byte(preferences.getLong("f2Max",0));
        add3byte(preferences.getLong("f2Min",0));
        add3byte(preferences.getLong("f2moy",0));
        add2byte(batt);
        lora.rafale(message, 14, id);
        preferences.putBool("waitingtrans",false);
        rtc.goSleepHeureFixe(preferences.getUInt("sleepMeas",8),preferences.getUInt("measTime",0));
    }
    else{
        neopixelWrite(pins.LED, 0, 0, 12);
        cap.mesureRipper(10,"LDC1");
        preferences.putLong("f1Max",cap.ldc1->f1Max);
        preferences.putLong("f1Min",cap.ldc1->f1Min);
        preferences.putLong("f1moy",cap.ldc1->f1moy);
        preferences.putLong("f2Max",cap.ldc1->f2Max);
        preferences.putLong("f2Min",cap.ldc1->f2Min);
        preferences.putLong("f2moy",cap.ldc1->f2moy);
        preferences.putBool("waitingtrans",true);
        rtc.goSleepMinuteFixe(0,preferences.getUInt("transTime",0));
    }
}
void loop() {
    // LDCTest();
    charge.routinecharge(&mainRipper);
}
