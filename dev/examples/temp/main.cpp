// Import required libraries
#include <Preferences.h>

// Import modules
#include "Adafruit_ADS1X15.h"
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"

String cardModel = "v3.1";
String breakout = "ripperdoublev1";
String etrierModel = "etrier17";

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(cardModel, breakout);
rtcClass rtc(&preferences, SD_MMC);
capteurs cap(&pins, &rtc, SD_MMC, &preferences, etrierModel);
comLORA lora(&pins, &cap, &preferences);
charger charge(&pins, &rtc, SD_MMC, &preferences, &cap, &server, &ws, &lora);

#define set 4
#define CS1 5  // to set on PCB
#define CS2 18 // to set on PCB

#define ON_SICK 13
#define ADS1115 0x49

#define CH_A 15
#define CH_B 16

bool A, B;
unsigned int val = 0;
byte writeBuf[3];
byte buffer[3];
float t_micro;

const float VPS = 4.096 / 32768.0; // volts per step

int result = 0x37; // register selection where the ADC result is stored
long ADC1_result, ADC2_result;
byte drdy1, drdy2;
byte READ = 0b10000000;
int count = 0;
int pmwChannel = 0;
File dataFile; // file with logs of date, battery voltage and battery temp
int LED = 1;
float color[3] = {0., 0., 0.};
float bright = 12.0;
File file;
bool s = false;
byte dataBuffer[100];
int r = 0;
long tMeas;
String dataMessage;
unsigned long ADC1_read1, ADC1_read2, ADC2_read1, ADC2_read2;

void setup() {
    Serial.begin(115200);
    Serial.println("begin");

    pins.pinSetup();
    delay(1000);
    cap.pinSetup();
    lora.pinSetup();
    lora.rf95Setup();
    pins.sdmmcSetup();
    rtc.rtcSetup();
    cap.lsmSetup();
    // cap.adxlSetup();
    charge.initSPIFFS();
    charge.initWebSocket();
    charge.setup();
    cap.measBatt();
}

void sendRSSI() {
    pins.rainbowLoop(10);
    int milli0 = millis();
    while (millis() - milli0 < 30e3) {
        int responseCode = -2;
        if (charge.wifiConnect()) {
            int RSSI = WiFi.RSSI();
            JsonDocument info;
            info["RSSI"] = RSSI;
            info["millis"] = millis() - milli0;
            String infoPost;
            serializeJson(info, infoPost);
            Serial.println(infoPost);
            responseCode = charge.httpPostRequest(charge.host + "/rssi", infoPost);
        }
        Serial.print("response : ");
        Serial.println(responseCode);
        delay(500);
    }
}

// honeywell magnet test functions
unsigned char CheckStatus_ADC1(byte thisRegister) { // adc = CS
    /* checks status of ADC 1*/
    // SRI why global variables here since they are not used anywhere else ?
    digitalWrite(CS1, LOW); // reading
    SPI.transfer(thisRegister | READ);
    long ADC_status16 = SPI.transfer(0x00);
    long ADC_status8 = SPI.transfer(0x00);
    long ADC_status0 = SPI.transfer(0x00);
    long ADC_status24 = (ADC_status16 << 16) | (ADC_status8 << 8) | ADC_status0;
    digitalWrite(CS1, HIGH);
    ADC_status0 = ADC_status0 >> 4;
    return (ADC_status0);
}

unsigned char CheckStatus_ADC2(byte thisRegister) { // adc = CS
    /* checks status of ADC 1*/
    // SRI why global variables here since they are not used anywhere else ?
    digitalWrite(CS2, LOW); // reading
    SPI.transfer(thisRegister | READ);
    long ADC_status16 = SPI.transfer(0x00);
    long ADC_status8 = SPI.transfer(0x00);
    long ADC_status0 = SPI.transfer(0x00);
    long ADC_status24 = (ADC_status16 << 16) | (ADC_status8 << 8) | ADC_status0;
    digitalWrite(CS2, HIGH);
    ADC_status0 = ADC_status0 >> 4;
    return (ADC_status0);
}

void ReadResult_ADC1(byte thisRegister) { // adc = CS
    /* reads result of ADC */
    digitalWrite(CS1, LOW); // reading
    SPI.transfer(thisRegister | READ);
    unsigned long ADC_hi = SPI.transfer(0x00);
    unsigned long ADC_mid = SPI.transfer(0x00);
    unsigned long ADC_lo = SPI.transfer(0x00);
    digitalWrite(CS1, HIGH);

    if (s == false) {
        ADC1_read1 = (ADC_hi << 16) | (ADC_mid << 8) | ADC_lo;
        dataMessage = dataMessage + String(ADC1_read1) + ",";
        // dataMessage = dataMessage + "adc11" + ",";

        dataBuffer[r] = ADC_hi;
        r++;
        dataBuffer[r] = ADC_mid;
        r++;
        dataBuffer[r] = ADC_lo;
        r++;

    } else {
        ADC1_read2 = (ADC_hi << 16) | (ADC_mid << 8) | ADC_lo;
        ADC1_result = (long)ADC1_read1 - (long)ADC1_read2;

        dataMessage = dataMessage + String(ADC1_read2) + ",";
        // dataMessage = dataMessage + "adc12" + ",";

        dataBuffer[r] = ADC_hi;
        r++;
        dataBuffer[r] = ADC_mid;
        r++;
        dataBuffer[r] = ADC_lo;
        r++;

        // Serial.println(dataMessage);
        // dataMessage = "";
    }
}

void ReadResult_ADC2(byte thisRegister) { // adc = CS
    /* reads result of ADC */
    digitalWrite(CS2, LOW); // reading
    SPI.transfer(thisRegister | READ);
    unsigned long ADC_hi = SPI.transfer(0x00);
    unsigned long ADC_mid = SPI.transfer(0x00);
    unsigned long ADC_lo = SPI.transfer(0x00);
    digitalWrite(CS2, HIGH);

    if (s == false) {
        ADC2_read1 = (ADC_hi << 16) | (ADC_mid << 8) | ADC_lo;
        dataMessage = dataMessage + String(ADC2_read1) + ",";
        // dataMessage = dataMessage + "adc21" + ",";
        dataBuffer[r] = ADC_hi;
        r++;
        dataBuffer[r] = ADC_mid;
        r++;
        dataBuffer[r] = ADC_lo;
        r++;

    } else {
        ADC2_read2 = (ADC_hi << 16) | (ADC_mid << 8) | ADC_lo;
        ADC2_result = (long)ADC2_read1 - (long)ADC2_read2;

        dataMessage = dataMessage + String(ADC2_read2) + ",";
        // dataMessage = dataMessage + "adc22" + ",";

        dataBuffer[r] = ADC_hi;
        r++;
        dataBuffer[r] = ADC_mid;
        r++;
        dataBuffer[r] = ADC_lo;
        r++;

        A = digitalRead(CH_A);
        B = digitalRead(CH_B);
        dataMessage = dataMessage + String(A) + "," + String(B);
        Serial.println(dataMessage);
        dataMessage = "";

        dataBuffer[r] = (A << 1 | B);
        r++;
    }
}

void ADC_conv() {
    /* during conversion, gets the accelero data */
    /* checks for drdy pin and reads the result */
    drdy1 = CheckStatus_ADC1(0x38); // function to check status register
    drdy2 = CheckStatus_ADC2(0x38); // function to check status register

    int time3 = millis();
    bool timeOut = false;

    while ((drdy1 != 1 || drdy2 != 1) & (millis() - time3 < 20)) {
        drdy1 = CheckStatus_ADC1(0x38); // function to check status register
        drdy2 = CheckStatus_ADC2(0x38); // function to check status register
    }

    // Serial.printf("%d,%d\n", drdy1, drdy2);

    ReadResult_ADC1(result);
    ReadResult_ADC2(result);
}

void Write(byte thisRegister, byte thisValue) {
    /* SPI write function */
    digitalWrite(CS1, LOW);
    digitalWrite(CS2, LOW);
    SPI.transfer(thisRegister);
    SPI.transfer(thisValue);
    digitalWrite(CS1, HIGH);
    digitalWrite(CS2, HIGH);
}

void SR_pwm() {
    /* set and reset pulses */
    /* each pulse calls for a conversion by the ADC */
    digitalWrite(set, HIGH);
    delayMicroseconds(50);
    s = false;         // if i = false , then ADC conversion during SET
    Write(0x01, 0x70); // writes to CONV_START - result stored in DATA7

    // dataMessage += String(tMeas) + ",";
    if (file) {
        // Serial.println("file is open to print");
        file.write(dataBuffer, r);
        r = 0;
    }

    for (size_t i = 0; i < 4; i++) { // time in dataBuffer
        dataBuffer[r] = lowByte(tMeas >> 8 * (3 - i));
        r++;
    }

    ADC_conv(); // read drdy & read
    digitalWrite(set, LOW);
    delayMicroseconds(50);
    s = true;          // if i = true , then ADC conversion during RESET
    Write(0x01, 0x70); // writes to CONV_START - result stored in DATA7
    ADC_conv();
}

void ADCsetup() {
    Write(0x00, 0x00); // PD - register normal mode
    Write(0x03, 0x00); // CAL_START - PGA gain calibration
    Write(0x08, 0x38); // FILTER - set to SINC4 highest data rate SINGLE CONV 240Hz
    // Write(0x08, 0x36); // FILTER - set to SINC4 highest data rate CONT CONV 240Hz
    Write(0x09, 0x46); // CTRL
    Write(0x0A, 0x00); // SOURCE
    Write(0x0B, 0x23); // MUX_CTRL0
    Write(0x0C, 0xFF); // MUX_CTRL1
    Write(0x0D, 0x00); // MUX_CTRL2
    Write(0x0E, 0x20); // PGA
    // Write(0x01, 0x71); // writes to CONV_START - result stored in DATA7
    // Serial.println("setup of ADCs complete");
}

uint32_t neopixelColor(uint8_t red, uint8_t green, uint8_t blue) {
    return (uint32_t(red) << 16) | (uint32_t(green) << 8) | blue;
}
uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return neopixelColor(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return neopixelColor(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return neopixelColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbowLoop(int wait) {
    int LED = 1;

    for (int j = 0; j < 256; j++) {
        uint32_t color = Wheel(((byte)j * 3) & 255); // Incrementing by 3 for a single RGB LED
        uint8_t red = (color >> 16) & 0xFF;
        uint8_t green = (color >> 8) & 0xFF;
        uint8_t blue = color & 0xFF;
        neopixelWrite(LED, red / 20, green / 20, blue / 20);
        delay(wait);
    }
}
// honeywell test functions

void mainHW() {

    pinMode(CS1, OUTPUT); // chip selects for ADC
    pinMode(CS2, OUTPUT); // chip selects for ADC

    pinMode(set, OUTPUT); // Vset
    pinMode(CH_A, INPUT);
    pinMode(CH_B, INPUT);

    digitalWrite(CS1, HIGH);
    digitalWrite(CS2, HIGH);
    digitalWrite(set, LOW);
    ADCsetup();
    digitalWrite(2, HIGH);
    digitalWrite(12, HIGH);
    neopixelWrite(LED, 0, 12, 0);
    long m0 = millis();
    file = SD_MMC.open("/testHW.bin", FILE_WRITE);

    pins.initBlink();
    while (millis() < m0 + 240000) { // 120 second measurement
        tMeas = micros();
        SR_pwm();
        // Serial.println(millis() - tMeas);
    }
    file.close();
    rtc.goSleep(5000);
}

// 10V sick ADC functions

void SICK_meas() {
    if (dataFile) {
        // Serial.println("file is open to print");
        dataFile.write(dataBuffer, r);
        r = 0;
    }

    for (size_t i = 0; i < 4; i++) { // time in dataBuffer
        dataBuffer[r] = lowByte(tMeas >> 8 * (3 - i));
        r++;
    }

    Wire.beginTransmission(ADS1115);
    Wire.write(0);
    Wire.endTransmission();

    Wire.requestFrom(ADS1115, 2);
    dataBuffer[r] = Wire.read();
    r++;

    dataBuffer[r] = Wire.read();
    r++;
    Wire.endTransmission();
}

void mainADCsick() {
    delay(1000);
    Wire.begin(14, 21);
    delay(1000);

    pinMode(ON_SICK, OUTPUT);
    digitalWrite(ON_SICK, HIGH);

    delay(1000);
    Serial.begin(115200);
    Serial.println("Hello!");

    Serial.println("Getting differential reading from AIN0 (P) and AIN1 (N)");
    Serial.println("for an input range of +/-4.096V");

    // measuring
    neopixelWrite(LED, 0, 12, 0);
    long m0 = millis();
    dataFile = SD_MMC.open("/testSICK_adc.bin", FILE_WRITE);

    writeBuf[0] = 1; // Config register
    writeBuf[1] = 0b10000100;
    writeBuf[2] = 0b11100011;

    Wire.beginTransmission(ADS1115);
    Wire.write(writeBuf[0]);
    Wire.write(writeBuf[1]);
    Wire.write(writeBuf[2]);
    Wire.endTransmission();

    pins.initBlink();

    while (millis() < m0 + 20000) { // 20 seconds
        tMeas = micros();
        SICK_meas();
        // Serial.println(millis() - tMeas);
    }
    dataFile.close();
    digitalWrite(ON_SICK, LOW);
    rtc.goSleep(5000);
}

void mainRipper() {
    // check wheel rot speed
    //  init LDC
    //  mesure LDC
    //  mettre resultats dans message
    //  rafale message

    int beginRotW = cap.wheelRot(2000) / M_PI * 30;
    Serial.println(beginRotW);
    int id = cap.id;

    if (beginRotW > 100) {
        Serial.println("turning");

        long max;
        long min;
        long moy;
        max = 30;
        min = 10;
        moy = 20;
        digitalWrite(pins.RFM95_CS, HIGH);
        digitalWrite(pins.ADXL375_CS, HIGH);
        cap.LDC_LHRSetup();
        cap.LDC_LHRMesure(&max, &min, &moy, 3000);
        neopixelWrite(pins.LED, 12, 12, 0);
        delay(50);

        byte message[30];
        int batt = cap.measBatt() * 100;
        int rtcTemp = rtc.rtc.getTemperature() * 10;
        long tsMeas = rtc.rtc.now().unixtime();
        int bufIndex = 0;
        for (size_t i = 0; i < 4; i++) {
            message[i] = lowByte(tsMeas >> 8 * (3 - i));
            bufIndex++;
        }
        message[bufIndex++] = lowByte(id);
        message[bufIndex++] = lowByte(batt);
        message[bufIndex++] = highByte(batt);
        message[bufIndex++] = lowByte(rtcTemp);
        message[bufIndex++] = highByte(rtcTemp);
        message[bufIndex++] = lowByte(beginRotW);
        message[bufIndex++] = highByte(beginRotW);
        message[bufIndex++] = lowByte(min >> 8 * 2);
        message[bufIndex++] = lowByte(min >> 8 * 1);
        message[bufIndex++] = lowByte(min >> 8 * 0);
        message[bufIndex++] = lowByte(max >> 8 * 2);
        message[bufIndex++] = lowByte(max >> 8 * 1);
        message[bufIndex++] = lowByte(max >> 8 * 0);
        message[bufIndex++] = lowByte(moy >> 8 * 2);
        message[bufIndex++] = lowByte(moy >> 8 * 1);
        message[bufIndex++] = lowByte(moy >> 8 * 0);
        lora.rafale(message, bufIndex, id);
        // message[0] = lowByte(7);
        // message[1] = highByte(7);
        // message[2] = lowByte(beginRotW);
        // message[3] = highByte(beginRotW);
        // lora.rafale(message, 4, 7);
        lora.rfSend("sleep turning");
        rtc.syncSleep(2, id);
    } else {
        Serial.println("still");
        lora.rfSend("sleeping still");
        rtc.syncSleep(1, id);
    }
}

void mainTask() {
    Serial.println("main");
    // digitalWrite(pins.ON_SICK, HIGH);
    // pins.rainbowLoop(10);
    cap.type = "sick";
    cap.saveSens(cap.type, 10);
    if (charge.wifiConnect()) {
        charge.sendSens(cap.type);
    }
    delay(2000);
    WiFi.disconnect(true);

    cap.type = "adxl";
    cap.saveSens(cap.type, 10);
    if (charge.wifiConnect()) {
        charge.sendSens(cap.type);
    }
    delay(2000);
    WiFi.disconnect(true);
}

void loop() {
    // LDCTest();
    // charge.routinecharge(&mainRipper);
    charge.routinecharge(&mainHW);      // for honeywell test COMMENT IF NOT USED
    charge.routinecharge(&mainADCsick); // for 10V sick ADC test COMMENT IF NOT USED
}