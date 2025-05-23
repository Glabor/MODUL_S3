#include "comLORA.h"

comLORA::comLORA(pinout *p, capteurs *c,Preferences *pr) {
    preferences = pr;
    pins = p;
    cap = c;
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
    pinMode(pins->ADXL375_SCK, OUTPUT);
    pinMode(pins->ADXL375_MOSI, OUTPUT);
    pinMode(pins->ADXL375_MISO, INPUT_PULLDOWN);

    rhSPI->begin();
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

void comLORA::rfSend(String message) {
    preferences->begin("prefid", false);
    int id=preferences->getUInt("id", 99);
    String mess2Send = "TT";//message texte
    preferences->end();
    mess2Send += message;
    // send a message using radio module
    rf95Setup();
    // rf95Setup();
    int bufSize = mess2Send.length() + 1;
    char Buf[bufSize];
    mess2Send.toCharArray(Buf, bufSize);
    Buf[0]=id;
    rf95->send((uint8_t *)Buf, bufSize);
    rf95->waitPacketSent();
}

void comLORA::rafale(byte *message, int length, int id) {
    Serial.println("rafale");
    int transmilli0 = millis();
    int transmitTime = 60;
    int sentTime;

    int prevAng;
    int turnNumber = 0;
    int angle;
    bool stopBool = false;
    rf95Setup();
    cap->dsox.getEvent(&cap->accel, &cap->gyro, &cap->temp);
    cap->rot->initangle(cap->accel.acceleration.x, cap->accel.acceleration.y, cap->accel.acceleration.z, cap->gyro.gyro.x, cap->gyro.gyro.y, cap->gyro.gyro.z,micros());
    int ns=0;
    while (((millis() - transmilli0) < transmitTime * 1000) && !stopBool) {
        cap->dsox.getEvent(&cap->accel, &cap->gyro, &cap->temp);
        /*if (cap->accel.acceleration.x != 0) {
            alpha = atan2(cap->accel.acceleration.y, cap->accel.acceleration.x * sqrt(sq(cap->accel.acceleration.x) + sq(cap->accel.acceleration.z)) / abs(cap->accel.acceleration.x));
        } else // add a small amount to zero acceleration to not divide by 0
        {
            alpha = atan2(cap->accel.acceleration.y, cap->accel.acceleration.x * sqrt(sq(cap->accel.acceleration.x) + sq(cap->accel.acceleration.z)) / abs(cap->accel.acceleration.x + 0.01));
        }*/
        float alpha_deg =cap->rot->correctionangle(0.1,cap->accel.acceleration.x, cap->accel.acceleration.y, cap->accel.acceleration.z, cap->gyro.gyro.x, cap->gyro.gyro.y, cap->gyro.gyro.z,micros());
        if ((alpha_deg - prevAng) > 300) { // decrement turn counter if too great difference with previous angle
            turnNumber--;
        }
        if ((prevAng - alpha_deg) > 300) { // increment turn Counter
            turnNumber++;
        }
        prevAng = alpha_deg;
        angle = (int)(alpha_deg * 100);
        /*Serial.print(angle);
        Serial.print(",");
        Serial.println(turnNumber);*/

        if (rf95->available())
        // receive to check if confirmation is sent
        {
            Serial.println("received");
            uint8_t buf[50];
            uint8_t len = sizeof(buf);
            if (rf95->recv(buf, &len)) {
                // String recMessage = String((char *)buf);
                int count = buf[0];
                // int count = recMessage.toInt();
                if ((count == id) && (len < 5)) {
                    stopBool = true; // confirmation that base received the message, and tells to stop rafale
                }
                Serial.print(count);
                Serial.print(",");
                Serial.println(len);
            }
        }

        if ((millis() - sentTime > 50)) {
            int index = length;
            message[index++] = lowByte(angle);
            message[index++] = highByte(angle);
            message[index++] = lowByte(turnNumber);
            rf95->send(message, length + 3);
            ns++;
            rf95->waitPacketSent();
            //Serial.println("sent");
            sentTime = millis();
        }
    }
    Serial.println("rafale ended: "+String(ns)+" messages sent");
}