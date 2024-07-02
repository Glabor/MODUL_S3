#ifndef CAPTEURS_H
#define CAPTEURS_H

#include "pinout.h"
#include "rtcClass.h"
#include <Adafruit_ADXL375.h>
#include <Adafruit_LSM6DSO32.h>
#include <Preferences.h>
#include <RTClib.h>
#include <SD_MMC.h>

class capteurs {
public:
    capteurs(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr);
    float measBatt();
    int battSend;
    void pinSetup();
    bool lsmSetup();
    bool adxlSetup();
    void saveSens(String sens, int sensTime);
    void accBuffering(int meas);
    bool initSens(String sens);
    void getSens(String sens);
    void HMRsetup();
    void Write(byte thisRegister, byte thisValue);
    void LDC_LHRSetup();
    void LDC_LHRMesure(long *Max, long *Min, long *Moy, int duration);
    int genVar = 5;
    int id = 0;
    String type = "";
    String newName = "";
    bool bADXL = false;
    bool bSick = false;
    Adafruit_ADXL375 *adxl = nullptr;
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    Adafruit_LSM6DSOX dsox; // accelerometer
private:
    pinout *pins;
    rtcClass *rtc;
    fs::FS *fs;
    Preferences *preferences;

    // Adafruit_ADXL375 adxl = Adafruit_ADXL375(ADXL375_CS, &SPI, 12345);

    byte sdBuf[200];
    int r = 0;
};

#endif