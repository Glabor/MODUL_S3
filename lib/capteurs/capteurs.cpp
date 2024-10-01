#include "capteurs.h"

capteurs::capteurs(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr, String modele) {
    rot = new angle(&dsox, modele);
    pins = p;
    if (pins->LHR_CS_1 >= 0) {
        ldc1 = new LDC(pins,pins->LHR_CS_1,pins->LHR_SWITCH_1);
    }
    if (pins->LHR_CS_2 >= 0) {
        ldc2 = new LDC(pins,pins->LHR_CS_2,pins->LHR_SWITCH_2);
    }
    rtc = r;
    fs = &f;
    preferences = pr;
    adxl = new Adafruit_ADXL375(pins->ADXL375_SCK, pins->ADXL375_MISO,
                                pins->ADXL375_MOSI, pins->ADXL375_CS, 12345);
    /*preferences->begin("prefid", false);
    id = preferences->getUInt("id", 0);
    preferences->end();*/
}
float capteurs::measBatt() {
    float cellVolt;
    analogReadResolution(12);

    int count = 0;
    for (int battCount = 1; battCount <= 100; battCount++) {
        count++;
        float meas = analogRead(pins->battPin);
        cellVolt = (float)((float)((float)(cellVolt * (count - 1)) +
                                   (float)analogRead(pins->battPin)) /
                           (float)count); // reading pin to measure battery level
    }
    cellVolt = cellVolt / 4096 * 3.3 * 2; // convert measure from 12bytes to volts
    battSend = cellVolt * 100;
    Serial.print("batt : ");
    Serial.println(cellVolt);
    delay(1000);
    if ((cellVolt < 3.5) && (cellVolt > 0.5) && !(rtc->chg)) { // sleep for 40 days (arbitrary)
        rtc->goSleep(21600);
    }

    return cellVolt;
}
bool capteurs::lsmSetup(void) {
    /*set up accelerometer*/
    if (!dsox.begin_I2C()) {
        Serial.println("LSM not found");
        return false;
    };
    Serial.println("LSM ok");

    dsox.setAccelDataRate(LSM6DS_RATE_6_66K_HZ);
    dsox.setAccelRange(LSM6DS_ACCEL_RANGE_8_G);
    dsox.setGyroRange(LSM6DS_GYRO_RANGE_125_DPS);
    dsox.setGyroDataRate(LSM6DS_RATE_6_66K_HZ);

    return true;
}

bool capteurs::adxlSetup(void) {
    SPI.begin(pins->ADXL375_SCK, pins->ADXL375_MISO, pins->ADXL375_MOSI);
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
    pins->all_CS_high();
    digitalWrite(pins->ADXL375_CS, LOW);
    if (!adxl->begin()) {
        Serial.println("could not find ADXL");
        return false;
    }
    adxl->setDataRate(ADXL3XX_DATARATE_3200_HZ);
    SPI.endTransaction();
    digitalWrite(pins->ADXL375_CS, HIGH);
    // SPI.end();
    return true;
}
void capteurs::accBuffering(int meas) {
    // divide int to two bytes
    sdBuf[r] = highByte(meas);
    r++;
    sdBuf[r] = lowByte(meas);
    r++;
}
void lire() {
    while (Serial2.available()) {
        Serial.print((char)Serial2.read());
    }
    Serial.println();
    delay(100);
}

void capteurs::HMRsetup() {
    digitalWrite(pins->ON_SICK, HIGH);
    Serial.println("HMRSetup");
    delay(100);
    Serial2.print("*99WE\r");
    Serial2.flush();
    lire();
    delay(2000);

    Serial2.print("*99ID\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00WE\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00B\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00WE\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00R=100\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00WE\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00TN\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00WE\r");
    Serial2.flush();
    lire();
    delay(200);

    Serial2.print("*00C\r");
    Serial2.flush();
    Serial.println("HMR setup done");
}

void capteurs::mesurePicot(long senstime) {
    if (!initSens("lsm")) {
        return;
    }
    if (!initSens("sick")) {
        return;
    }
    digitalWrite(pins->ON_SICK, HIGH);
    w0 = rot->wheelRot2();
    String fn = getName("picot");
    newName = fn;
    File file = SD_MMC.open(fn, FILE_WRITE);
    unsigned long t0 = millis();
    unsigned long time0 = micros();
    unsigned long ta_micro;
    if (!file) {
        return;
    }
    while (millis() < t0 + senstime * 1000) {
        ta_micro = micros() - time0;
        for (size_t j = 0; j < 4; j++) {
            sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
            r++;
        }
        getSens("lsm");
        getSens("lsmGyro");
        for (int j = 0; j < r; j++) {
            file.write(sdBuf[j]);
        }
        r = 0;
        for (int i = 0; i < 100; i++) {
            ta_micro = micros() - time0;
            for (size_t j = 0; j < 4; j++) {
                sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
                r++;
            }
            getSens("sick");
            for (int j = 0; j < r; j++) {
                file.write(sdBuf[j]);
            }
            r = 0;
        }
    }
    file.flush();
    file.close();
    digitalWrite(pins->ON_SICK, LOW);
}

void capteurs::mesureRipper(long senstime, String sens) {
    if (!initSens(sens)) {
        return;
    }
    String fn = getName(sens);
    Serial.println(fn);
    int startMillis = millis();
    File file = SD_MMC.open(fn, FILE_WRITE);
    unsigned long time0 = micros();
    newName = fn;
    if (!file) {
        return;
    }
    unsigned long t0 = millis();
    unsigned long ta_micro;
    while (millis() < t0 + senstime * 1000) {
        ta_micro = micros() - time0;
        for (size_t j = 0; j < 4; j++) {
            sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
            r++;
        }
        if (sens == "LDC1") {
            if(ldc1->mesure2f()){
                sdBuf[r] = ldc1->LHR_MSB1;
                r++;
                sdBuf[r] = ldc1->LHR_MID1;
                r++;
                sdBuf[r] = ldc1->LHR_LSB1;
                r++;
                sdBuf[r] = ldc1->LHR_MSB2;
                r++;
                sdBuf[r] = ldc1->LHR_MID2;
                r++;
                sdBuf[r] = ldc1->LHR_LSB2;
                r++;
                }
        }
        if (sens == "LDC2") {
            if(ldc2->mesure2f()){
                sdBuf[r] = ldc2->LHR_MSB1;
                r++;
                sdBuf[r] = ldc2->LHR_MID1;
                r++;
                sdBuf[r] = ldc2->LHR_LSB1;
                r++;
                sdBuf[r] = ldc2->LHR_MSB2;
                r++;
                sdBuf[r] = ldc2->LHR_MID2;
                r++;
                sdBuf[r] = ldc2->LHR_LSB2;
                r++;
            }
        }
        for (int j = 0; j < r; j++) {
                file.write(sdBuf[j]);
            }
        r = 0;
    }
    file.flush();
    file.close();
}

bool capteurs::initSens(String sens) {
    Serial.println(sens);
    if (sens == "lsm") {
        return lsmSetup();
    } else if (sens == "adxl") {
        return adxlSetup();
    }
    if (sens == "sick") {
        digitalWrite(pins->ON_SICK, HIGH);
        return true;
    } else if (sens == "hmr") // add line for HMR
    {
        HMRsetup();
        // digitalWrite(pins->ON_SICK, HIGH);
        return true;
    } else if (sens == "LDC1") {
        if (pins->LHR_CS_1 < 0) {
            return false;
        }
        ldc1->LHRSetup();
        return true;
    } else if (sens == "LDC2") {
        if (pins->LHR_CS_2 < 0) {
            return false;
        }
        ldc2->LHRSetup();
        return true;
    }
    return false;
}

void capteurs::getSens(String sens) {
    if (sens == "lsm") {
        sensors_event_t event;
        dsox.getEvent(&accel, &gyro, &temp);
        accBuffering((int)(accel.acceleration.x * 100));
        accBuffering((int)(accel.acceleration.y * 100));
        accBuffering((int)(accel.acceleration.z * 100));
        return;
    }
    else if (sens == "lsmGyro") {
        sensors_event_t event;
        dsox.getEvent(&accel, &gyro, &temp);
        accBuffering((int)(gyro.gyro.x * 100));
        accBuffering((int)(gyro.gyro.y * 100));
        accBuffering((int)(gyro.gyro.z * 100));
        return;
    }
    else if (sens == "adxl") {
        sensors_event_t event;
        adxl->getEvent(&event);
        accBuffering((int)(event.acceleration.x * 100));
        accBuffering((int)(event.acceleration.y * 100));
        accBuffering((int)(event.acceleration.z * 100));
        return;
    }
    if (sens == "sick") {
        int micros1 = micros();
        int count1 = 0;
        float val1 = 0;
        while ((micros() - micros1) < 1000) {
            count1++;
            val1 = (float)((val1 * (count1 - 1) + (float)analogRead(pins->SICK1)) /
                           (float)count1); // read adc
        }
        int val = (int)val1;
        accBuffering(val);
        return;
    } else if (sens == "hmr") // add variables for HMR measure
    {
        int time1;
        byte c = 0;
        int high = 0;
        int low = 0;
        int hmr_val = 0;
        time1 = micros();

        while (c != 0x0D) {
            c = Serial2.read();
        }
        c = 0;

        while (Serial2.available() < 6) {
        }

        for (int i = 0; i < 3; i++) {
            high = Serial2.read();
            low = Serial2.read();
            hmr_val = high << 8 | low;
            accBuffering(hmr_val);
            Serial.print(hmr_val);
            Serial.print(",");
        }
        Serial.println();
        // digitalWrite(pins->ON_SICK, bSick);
    } else if (sens == "angle") {
        sensors_event_t event;
        dsox.getEvent(&accel, &gyro, &temp);
        rot->initangle(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, accel.gyro.x, accel.gyro.y, accel.gyro.z, micros());
        accBuffering((int)rot->anglef);
    } else if (sens == "LDC1") {
        if (pins->LHR_CS_1 < 0) {
            return;
        }
        ldc1->mesure2f();
        accBuffering((int)ldc1->f1 / 1000);
        accBuffering((int)ldc1->f2 / 1000);
    } else if (sens == "LDC2") {
        if (pins->LHR_CS_2 < 0) {
            return;
        }
        ldc2->mesure2f();
        accBuffering((int)ldc2->f1 / 1000);
        accBuffering((int)ldc2->f2 / 1000);
    }
    return;
}
String capteurs::getName(String sens) {
    int startTime = 123456789;
    if (rtc->rtcConnected) {
        DateTime startDate = rtc->rtc.now();
        startTime = startDate.unixtime();
    }
    String fileDate = String(startTime);
    String beginStr = fileDate.substring(0, 5);
    String endStr = fileDate.substring(5, 10);
    String name =
        "/" + sens + "/" + beginStr + "/" + endStr + "/" + sens + ".bin";
    int index = 0;
    Serial.println("SUBSTRINGS OF " + name);
    while (name.indexOf("/", index) >= 0) {
        int start = name.indexOf("/", index) + 1;
        index = name.indexOf("/", index) + 1;
        int end = name.indexOf("/", index);
        if (end >= 0) {
            String dirCreate = SD_MMC.mkdir(name.substring(0, end))
                                   ? "dir " + name.substring(0, end) + " created "
                                   : " dir not created ";
            Serial.println(dirCreate);
        } else {
            Serial.println("file : /" + name.substring(start));
        }
    }
    return name;
}
void capteurs::saveSens(String sens, int sensTime) {
    if (!initSens(sens)) {
        return;
    }
    String fn = getName(sens);
    Serial.println(fn);
    int startMillis = millis();
    File file = SD_MMC.open(fn, FILE_WRITE);
    unsigned long time0 = micros();
    newName = fn;

    if (file) {
        while ((millis() - startMillis) < sensTime * 1000) {
            // change LED color
            float prog =
                ((float)(millis() - startMillis)) / ((float)(sensTime * 1000));
            neopixelWrite(pins->LED, pins->bright * (1.0 - prog), 0,
                          pins->bright * prog); // r->b

            // get data;
            r = 0;
            // accBuffering((int)(millis() - startMillis));
            unsigned long ta_micro = micros() - time0;
            for (size_t i = 0; i < 4; i++) {
                sdBuf[r] = lowByte(ta_micro >> 8 * (3 - i));
                r++;
            }

            getSens(sens);

            // write data
            for (int j = 0; j < r; j++) {
                file.write(sdBuf[j]);
            }
        }
    }
    file.flush();
    file.close();
    digitalWrite(pins->ON_SICK, bSick);
}
void capteurs::pinSetup() {
    // pinMode(pins->ADXL375_SCK, OUTPUT);
    // pinMode(pins->ADXL375_MOSI, OUTPUT);
    // pinMode(pins->ADXL375_MISO, INPUT_PULLDOWN);
    pinMode(pins->ADXL375_CS, OUTPUT);
    pinMode(pins->ON_SICK, OUTPUT);
    pinMode(pins->SICK1, INPUT_PULLDOWN);
    pinMode(pins->battPin, INPUT_PULLDOWN);

    digitalWrite(pins->ADXL375_CS, HIGH);
    digitalWrite(pins->RFM95_CS, HIGH);
    digitalWrite(pins->ON_SICK, bSick);
    analogReadResolution(12);
    if (pins->LHR_CS_1 >= 0) {
        pinMode(pins->LHR_CS_1, OUTPUT);
        digitalWrite(pins->LHR_CS_1, HIGH);
        pinMode(pins->LHR_SWITCH_1, OUTPUT);
        digitalWrite(pins->LHR_SWITCH_1, LOW);
    }
    if (pins->LHR_CS_2 >= 0) {
        pinMode(pins->LHR_CS_2, OUTPUT);
        digitalWrite(pins->LHR_CS_2, HIGH);
        pinMode(pins->LHR_SWITCH_2, OUTPUT);
        digitalWrite(pins->LHR_SWITCH_2, LOW);
    }
    pins->all_CS_high();
    SPI.begin(pins->ADXL375_SCK, pins->ADXL375_MISO, pins->ADXL375_MOSI);

}