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
//algo alg;
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
    rtc.rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(rtc.dateRTC(rtc.rtc.now()));
    cap.lsmSetup();
    // cap.adxlSetup();
    charge.initSPIFFS();
    charge.initWebSocket();
    charge.setup();
}

byte message[150];
int ind;
#define addbyte(val){ message[ind]=(byte)val; ind++;}
#define add2byte(val){ message[ind]=lowByte(val); ind++; message[ind]=highByte(val); ind++;}
#define add3byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++;}
#define add4byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++; message[ind]=lowByte(val>>24); ind++;}

void mainSimu() {
    preferences.begin("prefid", false);
    int id=preferences.getUInt("id",-1);
    if(id==-1){
        return;
    }
    randomSeed(analogRead(pins.SICK1));
    int rr=random(0,5);
    int batt = cap.measBatt() * 100;
    preferences.end();
    if(rr==0){
        
        lora.rfSend("sleeping"+String(batt)+","+String(rtc.rtc.getTemperature()));
        delay(20000);
        ESP.restart();
    }
    else if(rr==1){
        addbyte(id);
        addbyte(77); //"M"
        add4byte(rtc.rtc.now().unixtime());   
        add2byte(4);
        add2byte(5);
        add2byte(6);
        add2byte(7);
        add2byte(8);
        add2byte(9);
        add2byte(10);
        add2byte(11);
        addbyte( 12);
        addbyte( 13);
        addbyte( 14);
        add2byte( 15);
        for(int i=0;i<45;i++){
            addbyte(15);
        }
        add2byte((int)rtc.rtc.getTemperature()*10);
        add2byte(batt);
        lora.rafale(message, ind, id);
        delay(20000);
        ESP.restart();
    }
    else if(rr>1){
        addbyte(id);
        addbyte(82); //"R"
        add4byte(rtc.rtc.now().unixtime()); 
        add3byte(4);
        add3byte(5);
        add3byte(6);
        add3byte(7);
        add3byte(8);
        add3byte(9);
        if(rr==3){
            add3byte(10);
            add3byte(11);
            add3byte(12);
            add3byte(13);
            add3byte(14);
            add3byte(15);
        }
        add2byte((int)rtc.rtc.getTemperature()*10);
        add2byte(batt);
        lora.rafale(message, ind, id);
        delay(20000);
        ESP.restart();
    }
        
}

void loop() {
    mainSimu();
}
