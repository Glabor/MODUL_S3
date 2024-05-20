#include "comLORA.h"
comLORA::comLORA(pinout *p) {
    pins = p;
    // rhSPI = s;
    rhSPI = new RHSoftwareSPI(RHSoftwareSPI::Frequency1MHz, RHSoftwareSPI::BitOrderMSBFirst, RHSoftwareSPI::DataMode0);
    rhSPI->setPins(pins->ADXL375_MISO, pins->ADXL375_MOSI, pins->ADXL375_SCK);
    rf95 = new RH_RF95(pins->RFM95_CS, pins->RFM95_INT, *rhSPI);
}

bool comLORA::rf95Setup(void) {
    /*
    setup rf95 module at beginning
    do only once in program since it seems to freeze if done again after
    */
    bool rfSetup = false;
    digitalWrite(pins->ADXL375_CS, HIGH);
    digitalWrite(pins->RFM95_CS, LOW);

    digitalWrite(pins->RFM95_RST, LOW);
    delay(100);
    digitalWrite(pins->RFM95_RST, HIGH);
    delay(100);
    Serial.println("try rf init");
    if (!rf95->init()) {
        Serial.println("LoRa radio init failed");
        return rfSetup;
    }
    rfSetup = true;
    Serial.println("LoRa radio init OK!");
    rf95->setFrequency(RF95_FREQ);
    rf95->setTxPower(23, false);
    // set Bandwidth (7800,10400,15600,20800,31250,41700,62500,125000,250000, 500000)
    int bwSet = 125000;
    rf95->setSignalBandwidth(bwSet);
    // set Coding Rate (5, 6, 7, 8)
    int crSet = 5;
    rf95->setCodingRate4(crSet);
    // set Spreading Factor (6->64, 7->128, 8->256, 9->512, 10->1024, 11->2048, 12->4096)
    int sfSet = 7;
    rf95->setSpreadingFactor(sfSet);
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
    return rfSetup;
}
void comLORA::pinSetup() {
    pinMode(pins->RFM95_CS, OUTPUT);
    pinMode(pins->RFM95_INT, INPUT);
    pinMode(pins->RFM95_RST, OUTPUT);
}