// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"
#include "binfile.h"
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
void testBin() {
    preferences.begin("prefid", false);
    int id=preferences.getUInt("id",-1);
    int R=preferences.getUInt("radius",4500);
    preferences.end();
    bool async=true;
    String name;
    if(async){
        name="/test_async.sbf";
    }
    else{
        name="/test_sync.sbf";
    }
    binFile file(name);
    measurement lsm;
    lsm.addField(field("time","microsecond",UNSIGNED_4BYTES_L,1));
    lsm.addField(field("acc_x","m/s^-2",FLOAT,1));
    lsm.addField(field("acc_y","m/s^-2",FLOAT,1));
    lsm.addField(field("acc_z","m/s^-2",FLOAT,1));
    lsm.addField(field("gyro_x","rad/s^-2",FLOAT,1));
    lsm.addField(field("gyro_y","rad/s^-2",FLOAT,1));
    lsm.addField(field("gyro_z","rad/s^-2",FLOAT,1));
    if(async){
        lsm.nRow=0;
    }
    else{
        lsm.nRow=1;
    }
    int32_t rien=-123000;
    file.header.addMeasurement(lsm);
    measurement sick;
    sick.addField(field("time","microsecond",UNSIGNED_4BYTES_L,1));
    sick.addField(field("sick","number",UNSIGNED_2BYTES_L,1));
    if(async){
        sick.nRow=0;
    }
    else{
        sick.nRow=100;
    }
    file.header.addMeasurement(sick);
    uint32_t t=micros();
    uint32_t t0=millis();
    file.bind("time",&t);
    uint32_t v=0;
    file.bind("sick",&v);    
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    file.bind("acc_x",&accel.acceleration.x);
    file.bind("acc_y",&accel.acceleration.y);
    file.bind("acc_z",&accel.acceleration.z);
    file.bind("gyro_x",&gyro.gyro.x);
    file.bind("gyro_y",&gyro.gyro.y);
    file.bind("gyro_z",&gyro.gyro.z);
    file.header.addMetaData("date",rtc.rtc.now());
    file.header.addMetaData("bat",cap.measBatt());
    file.header.addMetaData("temp",rtc.rtc.getTemperature());
    file.header.addMetaData("id",id);
    file.header.addMetaData("radius",R);
    file.header.print();
    file.writeHeader();
    cap.lsmSetup();
    analogReadResolution(12);
    digitalWrite(pins.ON_SICK, HIGH);
    Serial.println("start measure");
    //while(millis()-t0<10000){
    for(int k=0;k<1000;k++){
        cap.dsox.getEvent(&accel, &gyro, &temp);
        file.writeMeasurement(0);
        for(int i=0;i<100;i++){
            t  =micros();
            v=analogRead(pins.SICK1);
            file.writeMeasurement(1);
        }
    }
    digitalWrite(pins.ON_SICK, LOW);
    file.close();
    delay(100);
    Serial.println("measure done");
}
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
    //cap.initSens("LDC1");
    //cap.initSens("LDC2");
    // cap.adxlSetup();
    charge.initSPIFFS();
    charge.initWebSocket();
    charge.setup();
    testBin();
}
void rien(){};
byte message[100];
int ind;
#define addbyte(val){ message[ind]=(byte)val; ind++;}
#define add2byte(val){ message[ind]=lowByte(val); ind++; message[ind]=highByte(val); ind++;}
#define add3byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++;}
#define add4byte(val){message[ind]=lowByte(val); ind++; message[ind]=lowByte(val>>8); ind++; message[ind]=lowByte(val>>16); ind++; message[ind]=lowByte(val>>24); ind++;}


void loop() {
    // LDCTest();
    //cap.ldc1->mesure2f();
    //cap.ldc2->mesure2f();
    //mainRipper();
    rtc.chg = true;
    charge.routinecharge(&rien);
}
