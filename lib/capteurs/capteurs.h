#ifndef CAPTEURS_H
#define CAPTEURS_H

#include "LDC.h"
#include "angle.h"
#include "binfile.h"
#include "hw.h"
#include "pinout.h"
#include "rtcClass.h"
#include <Adafruit_ADS1X15.h>
#include <Adafruit_ADXL375.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LSM6DSO32.h>
#include <Preferences.h>
#include <RTClib.h>
class capteurs {
public:
    capteurs(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr, String etrier);
    float measBatt();
    int battSend;
    void pinSetup();
    bool lsmSetup();
    bool bmeSetup();
    bool adsSetup();
    bool adxlSetup();
    void saveSens(String sens, int sensTime);
    void accBuffering(int meas);
    bool initSens(String sens);
    bool initHeader(String sens);
    void getSens(String sens);
    void HMRsetup();
    void mesurePicot(long senstime);
    void mesureRipper(long senstime, String sens);
    void mesureRSSI(long sensTime);
    struct Metadata {
        int speed = -1;
        int initangle = -1;
        int ring = -1;
        int timestamp = -1;
    };
    void mesureLisse(long senstime);
    void mesureLisse(long senstime, Metadata meta);
    float w0; // rotation debut mesure
    float wf; // rotation fin mesure
    int genVar = 10;
    int id = 0;
    String type = "";
    String newName = "";
    bool bADXL = false;
    bool bSick = false;
    Adafruit_ADXL375 *adxl = nullptr;
    Adafruit_BME280 bme;
    Adafruit_ADS1015 ads1015;
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    Adafruit_LSM6DSOX dsox; // accelerometer
    angle *rot = nullptr;
    LDC *ldc1 = nullptr;
    LDC *ldc2 = nullptr;
    hw *HW = nullptr;
    String getName(String sens);
    uint32_t v = 0; // sick

private:
    uint32_t t_micro = 0;
    binFile file;
    pinout *pins;
    rtcClass *rtc;
    fs::FS *fs;
    Preferences *preferences;

    // Adafruit_ADXL375 adxl = Adafruit_ADXL375(ADXL375_CS, &SPI, 12345);

    byte sdBuf[100];
    int r = 0;
};

#endif