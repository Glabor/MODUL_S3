#ifndef RTCCLASS_H
#define RTCCLASS_H
#include <RTClib.h>
class rtcClass {
public:
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
};
#endif