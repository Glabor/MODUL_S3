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
String breakout = "rippersimplev1";
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
    //rtc.rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(rtc.dateRTC(rtc.rtc.now()));
    cap.lsmSetup();
    // cap.adxlSetup();
    charge.initSPIFFS();
    charge.initWebSocket();
    charge.setup();
}
void tMax() {
    String line="";
    String date=rtc.dateRTC(rtc.rtc.now());
    line+=date+",";
    float temp=rtc.rtc.getTemperature();  
    line+=String(temp)+",";
    SPI.end();
    cap.pinSetup();
    if (pins.LHR_CS_1 >= 0) {
        if(!cap.ldc1->LHRSetup()){return;}
    }
    cap.ldc1->mesure2f();
    line+=String((int)cap.ldc1->f1) + ',' +String((int)cap.ldc1->f2);
    Serial.println(line);
    File logFile = SD_MMC.open("/testThemacs.txt", FILE_APPEND);
    if (logFile) { 
        logFile.println(line);
        logFile.flush();
        logFile.close();
        }
    delay(30000);//1min
}
void loop() {
    tMax();
}
