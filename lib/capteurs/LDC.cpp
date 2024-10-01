#include "LDC.h"
void LDC::WriteRegister(byte thisRegister, byte thisValue) {
    digitalWrite(cs, LOW);
    SPI.transfer(thisRegister);
    SPI.transfer(thisValue);
    digitalWrite(cs, HIGH);
}
void LDC::reset2f()
{
    count=0;
    count1=0;
    count2=0;
    f1Max=LONG_MAX;
    f1Min=LONG_MIN;
    f1sum=0;
    f1moy;
    f2Max=LONG_MAX;
    f2Min=LONG_MIN;
    f2sum=0;
    f2moy;
}
long LDC::ReadLHR_Data(byte thisRegister)
{
    digitalWrite(cs, LOW);
    SPI.transfer(thisRegister | 0b10000000);
    long LHR_DATA = SPI.transfer(0x00);
    // Serial.println(LHR_DATA);
    digitalWrite(cs, HIGH);
    return LHR_DATA;
}
LDC::LDC(pinout *p,int spics,int swi)
{
    cs=spics;
    sw=swi;
    pins=p;
}
void LDC::LHRSetup()
{
    pins->all_CS_high();
    //SPI.begin(pins->ADXL375_SCK, pins->ADXL375_MISO, pins->ADXL375_MOSI);
    //SPI.beginTransaction(SPISettings(100000, LSBFIRST, SPI_MODE3));
    if(cs>=0){
        // SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
        
        //pinMode(pins->LHR_CS_1, OUTPUT);
        //pinMode(pins->LHR_SWITCH_1, OUTPUT);
        // digitalWrite(CS, HIGH);
        delay(100);
        Serial.println("setup");

        WriteRegister(0x0A, 0x001); //  INTB_MODE
        WriteRegister(0x01, 0x75); //  RP_SET
        WriteRegister(0x04, 0x03); //  DIG_CONF
        WriteRegister(0x05, 0x01); //  ALT_CONFIG
        WriteRegister(0x0C, 0x01); //  D_CONFIG
        WriteRegister(0x30, 0xCC); //  LHR_RCOUNT_LSB sample rate LSB
        WriteRegister(0x31, 0x77); //  LHR_RCOUNT_MSB sample rate MSB
        WriteRegister(0x34, 0x00); //  LHR_CONFIG
        WriteRegister(0x32, 0x00);
        WriteRegister(0x33, 0x00);
        WriteRegister(0x0B, 0x00); //  START_CONFIG
        delay(500);
    }
    mesure2f();
    reset2f();
}
bool LDC::mesure2f()
{
    pins->all_CS_high();
    bool m1=false;
    bool m2=false;
    digitalWrite(sw,HIGH);
    delay(100);
    digitalWrite(cs, LOW);
    SPI.transfer(0x3B | READ); // reading status
    uint8_t LHR_status=1;
    LHR_status = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    if (LHR_status == 0) {
        //neopixelWrite(pins->LED, 12, 12, 12);
        // Serial.println("status loop");
        LHR_LSB1 = ReadLHR_Data(0x38);
        LHR_MID1 = ReadLHR_Data(0x39);
        LHR_MSB1 = ReadLHR_Data(0x3A);
        inductance = (LHR_MSB1 << 16) | (LHR_MID1 << 8) | LHR_LSB1;
        f1 = (pow(2, sens_div) * f * inductance) / (pow(2, 24));
        Serial.println(f1);
        m1=true;
        count1++;
        f1Max=max(f1,f1Max);
        f1Min=min(f1,f1Min);
        f1sum+=f1;
        f1moy=f1sum/count1;
    }
    digitalWrite(sw,LOW);
    delay(100);
    digitalWrite(cs, LOW);
    SPI.transfer(0x3B | READ); // reading status
    LHR_status =1;
    LHR_status = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    if (LHR_status == 0) {
        //neopixelWrite(pins->LED, 12, 12, 12);
        // Serial.println("status loop");
        LHR_LSB2 = ReadLHR_Data(0x38);
        LHR_MID2 = ReadLHR_Data(0x39);
        LHR_MSB2 = ReadLHR_Data(0x3A);
        inductance = (LHR_MSB2 << 16) | (LHR_MID2 << 8) | LHR_LSB2;
        f2 = (pow(2, sens_div) * f * inductance) / (pow(2, 24));
        m2=true;
        count2++;
         f2Max=max(f2,f2Max);
        f2Min=min(f2,f2Min);
        f2sum+=f2;
        f2moy=f2sum/count2;
    }
    count=count1+count2;
    Serial.print("f1 = ");
    Serial.print(f1);
    Serial.print(", f2 = ");
    Serial.println(f2);
    Serial.println("setup done");
    Serial.print(count);
    Serial.println(" mesures LDC");
    
   
    return m1&m2;
}
/*void LDC::LDC_LHRMesure(long *f1Moy,long *f2Moy, int duration,int cs,int sw) {
    if(cs<0){
        return;
    }
    long LHR_status, LHR_LSB, LHR_MID, LHR_MSB, inductance, f1,f2;
    long long sum1 = 0;
    long long sum2 = 0;
    int count = 0;
    int sens_div = 0;
    long f = 16000000; // f from clock gen
    byte READ = 0b10000000;

    int startTime = 123456789;
    if (rtc->rtcConnected) {
        DateTime startDate = rtc->rtc.now();
        startTime = startDate.unixtime();
    }
    String fileDate = String(startTime);
    String beginStr = fileDate.substring(0, 5);
    String endStr = fileDate.substring(5, 10);
    String sens = "LHR";
    String name = "/" + sens + "/" + beginStr + "/" + endStr + "/" + sens + ".bin";
    int index = 0;
    while (name.indexOf("/", index) >= 0) {
        int start = name.indexOf("/", index) + 1;
        index = name.indexOf("/", index) + 1;
        int end = name.indexOf("/", index);
        if (end >= 0) {
            String dirCreate = SD_MMC.mkdir(name.substring(0, end)) ? "dir " + name.substring(0, end) + " created " : " dir not created ";
            Serial.println(dirCreate);
        } else {
            Serial.println("file : /" + name.substring(start));
        }
    }
    File file = SD_MMC.open(name, FILE_WRITE);
    neopixelWrite(pins->LED, 0, 0, 0);
    delay(1000);
    int start_time = millis();

    while (millis() < start_time + duration) {
        digitalWrite(sw,HIGH);
        delay(100);
        digitalWrite(cs, LOW);
        SPI.transfer(0x3B | READ); // reading status
        uint8_t LHR_status = SPI.transfer(0x00);
        digitalWrite(cs, HIGH);

        if (LHR_status == 0) {
            neopixelWrite(pins->LED, 12, 12, 12);
            // Serial.println("status loop");
            LHR_LSB = ReadLHR_Data(0x38,cs);
            LHR_MID = ReadLHR_Data(0x39,cs);
            LHR_MSB = ReadLHR_Data(0x3A,cs);
            inductance = (LHR_MSB << 16) | (LHR_MID << 8) | LHR_LSB;
            f1 = (pow(2, sens_div) * f * inductance) / (pow(2, 24));
        }
        file.write(LHR_LSB);
        file.write(LHR_MID);
        file.write(LHR_MSB);
        digitalWrite(sw,HIGH);
        delay(100);
        digitalWrite(cs, LOW);
        SPI.transfer(0x3B | READ); // reading status
        uint8_t LHR_status = SPI.transfer(0x00);
        digitalWrite(cs, HIGH);

        if (LHR_status == 0) {
            neopixelWrite(pins->LED, 12, 12, 12);
            // Serial.println("status loop");
            LHR_LSB = ReadLHR_Data(0x38,cs);
            LHR_MID = ReadLHR_Data(0x39,cs);
            LHR_MSB = ReadLHR_Data(0x3A,cs);
            inductance = (LHR_MSB << 16) | (LHR_MID << 8) | LHR_LSB;
            f2 = (pow(2, sens_div) * f * inductance) / (pow(2, 24));
        }
        file.write(LHR_LSB);
        file.write(LHR_MID);
        file.write(LHR_MSB);
        sum1 += f1;
        sum2 += f2;
        count++;
    }    
    // sum = sum / count;
    // Moy = &sum;
    long moy = sum1 / count;
    *f1Moy = moy;
    moy = sum2 / count;
    *f2Moy = moy;
    digitalWrite(cs, HIGH);
    file.flush();
    file.close();
    // SPI.endTransaction();
    SPI.end();
}*/