#ifndef RTCCLASS_H
#define RTCCLASS_H
#include <Preferences.h>
#include <RTClib.h>
#include <SD_MMC.h>

class rtcClass {
public:
    Preferences *preferences;
    rtcClass(Preferences *pr,fs::FS &f) {
        preferences = pr;
        fs = &f;
    };
    fs::FS *fs;
    RTC_DS3231 rtc;
    bool chg;
    bool rtcConnected;
    void rtcSetup() {
        chg = false;
        rtcConnected = false;
        if (rtc.begin()) {
            rtcConnected = true;
            rtc.disable32K();
            rtc.writeSqwPinMode(DS3231_OFF);
            rtc.disableAlarm(2);
            Serial.print("alarm fired : ");
            Serial.println(rtc.alarmFired(1));
            if (!rtc.alarmFired(1)) {
                chg = true;
            }
            rtc.setAlarm1(rtc.now() + TimeSpan(1), DS3231_A1_Date);
            Serial.println("alarm setup");
        } else {
            chg = true;
            Serial.println("Couldn't find RTC");
            // Serial.flush();
            // ESP.restart();
        }
        Serial.print("chg is ");
        Serial.println(chg);
    };
    String dateRTC(DateTime dateDate) {
        /*return RTC date in good format*/
        String dateString = "";
        dateString += String(dateDate.year(), DEC);
        dateString += "/";
        dateString += String(dateDate.month(), DEC);
        dateString += "/";
        dateString += String(dateDate.day(), DEC);
        dateString += "    ";
        dateString += String(dateDate.hour(), DEC);
        dateString += ":";
        dateString += String(dateDate.minute(), DEC);
        dateString += ":";
        dateString += String(dateDate.second(), DEC);
        return dateString;
    }
    void log(float batvolt,bool waitingtrans,float w){
        float RTCtemp=rtc.getTemperature();
        File logFile = SD_MMC.open("/log.txt", FILE_APPEND);
        if (logFile) {
            logFile.print("\n");
            logFile.print("ON : ");
            logFile.println(dateRTC(rtc.now()));
            logFile.print("bat");
            logFile.print(String(batvolt));
            logFile.println("V");
            logFile.print("RTC temp: ");
            logFile.println(String(RTCtemp));
            logFile.print("RDC speed: ");
            logFile.print(String(w/2/M_PI*60));
            logFile.println("rpm");
            if(abs(w)<0.1*2*M_PI/60){logFile.println("back to sleep");}
            else{
                if(waitingtrans){
                    preferences->begin("prefid", false);
                    logFile.print("begin transmission: ");
                    logFile.println(preferences->getString("NEWNAME",""));
                    preferences->end();
                }
                else{
                    logFile.println("begin measurement");
                }
            }
        }
        logFile.flush();
        logFile.close();
    }
    void goSleepMinuteFixe(int sleepMinutes,int minute){
        DateTime d0=rtc.now();
        DateTime d1=DateTime(d0.year(), d0.month(), d0.day(), d0.hour(),minute, 0);
        while(d1<d0+TimeSpan(sleepMinutes*60)-TimeSpan(30*60)){
            d1=d1+TimeSpan(3600);
        }
        while(d1<d0+TimeSpan(60)){//5minutes minimum
            d1=d1+TimeSpan(3600);
        }
        File logFile = SD_MMC.open("/log.txt", FILE_APPEND);
        if (logFile) {    
              logFile.println("wakeup set to:") ;
              logFile.println(dateRTC(d1)); 
        }
        logFile.flush();
        logFile.close();
        rtc.setAlarm1(d1, DS3231_A1_Date);
        rtc.clearAlarm(1);
        delay(500);
        ESP.restart();
    };
    void safeRestart(){
        rtc.setAlarm1(rtc.now() + TimeSpan(3600), DS3231_A1_Date);
        ESP.restart();
    }
    void goSleep2MinuteFixe(int sleepMinutes,int minute1,int minute2){//2 crenaux de reveil
        DateTime d0=rtc.now();
        int seuil=min(abs(minute1-minute2),60-abs(minute1-minute2))/2+2;
        DateTime d1=DateTime(d0.year(), d0.month(), d0.day(), d0.hour(),minute1, 0);
        DateTime d2=DateTime(d0.year(), d0.month(), d0.day(), d0.hour(),minute2, 0);
        DateTime d3=d0+TimeSpan(sleepMinutes*60);
        DateTime d4;
        bool test1=true;
        bool test2=true;
        while(test1&test2){
            if(d1>d0+TimeSpan(60)){
                if(d3>d1&&d3<=d1+TimeSpan(seuil*60)){
                    d4=d1;
                    test1=false;
                }
                if(d3<d1&&d3>=d1-TimeSpan(seuil*60)){
                    d4=d1;
                    test1=false;
                }
            }
            if(test1){
                d1=d1+TimeSpan(3600);
            }
            if(d2>d0+TimeSpan(60)){
                if(d3>d2&&d3<=d2+TimeSpan(seuil*60)){
                    d4=d2;
                    test2=false;
                }
                if(d3<d2&&d3>=d2-TimeSpan(seuil*60)){
                    test2=false;
                    d4=d2;
                }
            }
            if(test2){
                d2=d2+TimeSpan(3600);
            }
        }
        File logFile = SD_MMC.open("/log.txt", FILE_APPEND);
        if (logFile) {    
              logFile.println("wakeup set to:") ;
              logFile.println(dateRTC(d4)); 
        }
        logFile.flush();
        logFile.close();
        rtc.setAlarm1(d4, DS3231_A1_Date);
        rtc.clearAlarm(1);
        delay(500);
        ESP.restart();
    };
    void goSleepHeureFixe(int sleepHour,int minute){
        DateTime t0=rtc.now();
        int h0=t0.hour();
        int h1=h0-(h0%sleepHour)+sleepHour;
        if(h1==24){
            h1=0;  
        }
        DateTime t1=DateTime(t0.year(), t0.month(), t0.day(), h1,minute, 0);
        if(h1==0){
            t1=t1+TimeSpan(24*60*60);
        }
        File logFile = SD_MMC.open("/log.txt", FILE_APPEND);
        if (logFile) {    
              logFile.println("wakeup set to:") ;
              logFile.println(dateRTC(t1)); 
        }
        logFile.flush();
        logFile.close();
        rtc.setAlarm1(t1, DS3231_A1_Date);
        rtc.clearAlarm(1);
        delay(500);
        ESP.restart();
        
    };
    void goSleep(int sleepTime) {
        Serial.println("go sleeping");
        if (rtcConnected) {
            rtc.setAlarm1(rtc.now() + sleepTime, DS3231_A1_Date);
            Serial.println("sleeping " + String(sleepTime) + "s");
            rtc.clearAlarm(1);
            delay(500);
            Serial.println("Alarm cleared");
        }
        ESP.restart();
    };

    void syncSleep(int sleepMode, int sysID) {
        int wake0 = 1589194800;
        int wakeTime = wake0 + sysID * 30;
        int sleepCyc[3] = {2000, 1, 8};
        int sleepT = sleepCyc[sleepMode];
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("sleep", 30);
        preferences->end();
        int block = idRead;
        int nowTime = rtc.now().unixtime();
        int timeDif = (sleepT * block) - (nowTime - wakeTime) % (sleepT * block); // time to go to next block
        rtc.setAlarm1(rtc.now() + TimeSpan(timeDif), DS3231_A1_Date);             // plan time to next block + cyc

        while (true) { // try to clear alarm to turn off PCB power
            rtc.clearAlarm(1);
            delay(500);
        }
    }
};
#endif