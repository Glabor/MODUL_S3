// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "pinout.h"
#include "rtcClass.h"

String model = "v3.1";

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(model);
rtcClass rtc;
capteurs cap(&pins, &rtc, SD_MMC, &preferences);
comLORA lora(&pins, &cap);
charger charge(&pins, &rtc, SD_MMC, &preferences, &cap, &server, &ws, &lora);

void setup() {
    Serial.begin(115200);
    Serial.println("begin");

    pins.pinSetup();
    delay(1000);
    cap.pinSetup();
    lora.pinSetup();
    // lora.rf95Setup();
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

/*TEST*/
#define CS pins.Ext_SPI_CS
byte READ = 0b10000000;
uint8_t status;
long LHR_status;
#define STATUS 0x20
#define LHR_STATUS 0x3B
#define LHR_DATA_LSB 0x38
#define LHR_DATA_MID 0x39
#define LHR_DATA_MSB 0x3A
long LHR_LSB, LHR_MID, LHR_MSB, LHR_DATA, inductance;
int sens_div = 0;
long f = 16000000; // f from clock gen

void Write(byte thisRegister, byte thisValue) {
    digitalWrite(CS, LOW);
    SPI.transfer(thisRegister);
    SPI.transfer(thisValue);
    digitalWrite(CS, HIGH);
}

long ReadLHRStatus(byte thisRegister) {
    // Serial.println("Reading LHR STATUS register");
    digitalWrite(CS, LOW);
    SPI.transfer(thisRegister | READ); // reading status
    LHR_status = SPI.transfer(0x00);
    // Serial.println(LHR_status, BIN);
    digitalWrite(CS, HIGH);
    return LHR_status;
}

long ReadLHR_Data(byte thisRegister) {
    digitalWrite(CS, LOW);
    SPI.transfer(thisRegister | READ);
    LHR_DATA = SPI.transfer(0x00);
    // Serial.println(LHR_DATA);
    digitalWrite(CS, HIGH);
    return LHR_DATA;
}

void rf95Setup() {
    Serial.println("rf setup");
    digitalWrite(pins.RFM95_CS, LOW);
    digitalWrite(pins.ADXL375_CS, HIGH);
    digitalWrite(pins.Ext_SPI_CS, HIGH);
    digitalWrite(pins.RFM95_RST, LOW);
    delay(100);
    digitalWrite(pins.RFM95_RST, HIGH);
    delay(100);

    if (!lora.rf95->init()) {
        Serial.println("LoRa radio init failed");
    }

    lora.rf95->setFrequency(RF95_FREQ);
    lora.rf95->setTxPower(23, false);
    // set Bandwidth (7800,10400,15600,20800,31250,41700,62500,125000,250000, 500000)
    int bwSet = 125000;
    lora.rf95->setSignalBandwidth(bwSet);
    // set Coding Rate (5, 6, 7, 8)
    int crSet = 5;
    lora.rf95->setCodingRate4(crSet);
    // set Spreading Factor (6->64, 7->128, 8->256, 9->512, 10->1024, 11->2048, 12->4096)
    int sfSet = 7;
    lora.rf95->setSpreadingFactor(sfSet);
    digitalWrite(pins.RFM95_CS, HIGH);
}

void rfSend(String message) {
    digitalWrite(pins.RFM95_CS, LOW);
    int bufSize = message.length() + 1;
    char Buf[bufSize];
    message.toCharArray(Buf, bufSize);
    lora.rf95->send((uint8_t *)Buf, bufSize);
    lora.rf95->waitPacketSent();
    digitalWrite(pins.RFM95_CS, HIGH);
}

void LDCsetup() {
    // Serial.begin(115200);
    SPI.begin(pins.ADXL375_SCK, pins.ADXL375_MISO, pins.ADXL375_MOSI);

    digitalWrite(pins.RFM95_CS, HIGH);
    digitalWrite(pins.ADXL375_CS, HIGH);

    pinMode(CS, OUTPUT);
    // digitalWrite(CS, HIGH);
    digitalWrite(CS, LOW);
    delay(500);
    Serial.println("setup");

    // rf95Setup();

    Write(0x0A, 0x00); //  INTB_MODE
    Write(0x01, 0x75); //  RP_SET
    Write(0x04, 0x03); //  DIG_CONF
    Write(0x05, 0x01); //  ALT_CONFIG
    Write(0x0C, 0x01); //  D_CONFIG
    Write(0x30, 0xCC); //  LHR_RCOUNT_LSB sample rate LSB
    Write(0x31, 0x77); //  LHR_RCOUNT_MSB sample rate MSB
    Write(0x34, 0x00); //  LHR_CONFIG
    Write(0x32, 0x00);
    Write(0x33, 0x00);
    Write(0x0B, 0x00); //  START_CONFIG
    delay(500);
    Serial.println("setup done");
}

void LDCloop() {
    status = ReadLHRStatus(LHR_STATUS);

    // if (status || 0b00000001 == 0b00000001) { // if bit0 of status i.e. drdy = 0, output is available
    if (status == 0) {
        Serial.print(status, BIN);
        Serial.print(",");
        // Serial.println("status loop");
        LHR_LSB = ReadLHR_Data(0x38);
        LHR_MID = ReadLHR_Data(0x39);
        LHR_MSB = ReadLHR_Data(0x3A);
        inductance = (LHR_MSB << 16) | (LHR_MID << 8) | LHR_LSB;
        String dataMessage = String(inductance) + ",";
        // Serial.print(inductance);
        // Serial.print(",");

        long fsensor = (pow(2, sens_div) * f * inductance) / (pow(2, 24));
        dataMessage = dataMessage + fsensor;
        Serial.println(dataMessage);
        // rfSend(dataMessage);
    }
    // Serial.println(fsensor);
    // delay(10); // try without delay?
}

void LDCTest() {
    Serial.println("test");
    neopixelWrite(pins.LED, 12, 0, 0);
    cap.LDC_LHRSetup();
    // LDCsetup();
    // delay(1000);
    digitalWrite(pins.RFM95_CS, HIGH);
    digitalWrite(pins.ADXL375_CS, HIGH);

    neopixelWrite(pins.LED, 12, 12, 0);
    while (true) {
        LDCloop();
        neopixelWrite(pins.LED, 12, 12, 12);
    }
}
/*TEST*/

void mainRipper() {
    // init LDC
    // mesure LDC
    // mettre resultats dans message
    // rafale message
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
    neopixelWrite(pins.LED, 0, 12, 0);
    int batt = cap.measBatt() * 100;

    int id = 12;
    byte message[30];
    message[0] = lowByte(id);
    message[1] = highByte(id);
    message[3] = lowByte(max >> 8 * 2);
    message[4] = lowByte(max >> 8 * 1);
    message[5] = lowByte(max >> 8 * 0);
    message[6] = lowByte(min >> 8 * 2);
    message[7] = lowByte(min >> 8 * 1);
    message[8] = lowByte(min >> 8 * 0);
    message[9] = lowByte(moy >> 8 * 2);
    message[10] = lowByte(moy >> 8 * 1);
    message[11] = lowByte(moy >> 8 * 0);
    message[12] = lowByte(batt);
    message[13] = highByte(batt);
    lora.rafale(message, 14, id);
    rtc.goSleep(20);
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
    charge.routinecharge(&mainRipper);
}
