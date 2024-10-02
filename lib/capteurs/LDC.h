#include "pinout.h"
#include <Arduino.h>

class LDC {
    public:
    LDC(pinout *p,int spics,int swi);
    void LHRSetup();
    bool mesure2f();
    long LHR_LSB1, LHR_MID1, LHR_MSB1,LHR_LSB2, LHR_MID2, LHR_MSB2, f1,f2;
    int count=0;
    long f1Max=LONG_MIN;
    long f1Min=LONG_MAX;
    long long f1sum=0;
    long f1moy;
    long f2Max=LONG_MIN;
    long f2Min=LONG_MAX;
    long long f2sum=0;
    long f2moy;
    void reset2f();
    private:
    int count1=0;
    int count2=0;
    pinout* pins=nullptr;
    long ReadLHR_Data(byte thisRegister);
    void WriteRegister(byte thisRegister, byte thisValue);
    byte READ = 0b10000000;
    long f = 16000000;       // f from clock gen
    long inductance;   
    int sens_div = 0;
    int cs;
    int sw;
};