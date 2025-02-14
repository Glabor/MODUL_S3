#ifndef CAPTEURS_H
#define CAPTEURS_H

#include "LDC.h"
#include "angle.h"
#include "hw.h"
#include "pinout.h"
#include "rtcClass.h"
#include <Adafruit_ADXL375.h>
#include <Adafruit_LSM6DSO32.h>
#include <Preferences.h>
#include <RTClib.h>
#include <SD_MMC.h>
class capteurs {
public:
    capteurs(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr, String etrier);
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
    void mesurePicot(long senstime);
    void mesureRipper(long senstime, String sens);
    float w0; // rotation debut mesure
    float wf; // rotation fin mesure
    int genVar = 10;
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
    angle *rot = nullptr;
    LDC *ldc1 = nullptr;
    LDC *ldc2 = nullptr;
    hw *HW = nullptr;
    String getName(String sens);

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