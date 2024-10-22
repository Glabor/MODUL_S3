#include "pinout.h"
#include <Arduino.h>

class hw {
public:
    hw(pinout *p);
    void measureHMR(int measTime, String sensName);

private:
    pinout *pins = nullptr;
    int adc1 = -1, adc2 = -1, adc3 = -1, set = -1;
    int result = 0x37; // register selection where the ADC result is stored
    long ADC1_result;
    long ADC2_result;
    long ADC3_result;
    long max_x = 0, max_y = 0, max_z = 0;
    long min_x = 16777215, min_y = 16777215, min_z = 16777215;
    long avg_x = 0, avg_y = 0, avg_z = 0;
    long sum_x = 0, sum_y = 0, sum_z = 0;
    byte drdy1, drdy2, drdy3;
    byte READ = 0b10000000;
    bool s = false;
    int count_x = 0, count_y = 0, count_z = 0;
    int r = 0;
    long tMeas;

    File file;     // file with ADC data
    File dataFile; // file with logs of date, battery voltage and battery temp
    String dataMessage;
    byte dataBuffer[100];
    ////////////LED///////////////
    int LED = 1;
    float color[3] = {0., 0., 0.};
    float bright = 12.0;

    void ADCsetup();
    unsigned char CheckStatus_ADC(byte thisRegister, int adc);
    void ReadResult_ADC(byte thisRegister, int adc);
    void ADC_conv();
    void Write(byte thisRegister, byte thisValue);
    void SR_pwm();
};