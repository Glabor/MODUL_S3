#ifndef RTCCLASS_H
#define RTCCLASS_H
#include <Preferences.h>
#include <RTClib.h>

class rtcClass {
public:
    Preferences *preferences;
    rtcClass(Preferences *pr) {
        preferences = pr;
    };
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