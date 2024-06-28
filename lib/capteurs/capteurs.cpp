#include "capteurs.h"

capteurs::capteurs(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr) {
    pins = p;
    rtc = r;
    fs = &f;
    preferences = pr;
    adxl = new Adafruit_ADXL375(pins->ADXL375_SCK, pins->ADXL375_MISO,
                                pins->ADXL375_MOSI, pins->ADXL375_CS, 12345);
    preferences->begin("prefid", false);
    id = preferences->getUInt("id", 0);
    preferences->end();
}
float capteurs::measBatt() {
    float cellVolt;
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
    SPI.begin(pins->ADXL375_SCK, pins->ADXL375_MISO, pins->ADXL375_MOSI,
              pins->ADXL375_CS);
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
    digitalWrite(pins->RFM95_CS, HIGH);
    digitalWrite(pins->ADXL375_CS, LOW);
    if (!adxl->begin()) {
        Serial.println("could not find ADXL");
        return false;
    }
    adxl->setDataRate(ADXL3XX_DATARATE_3200_HZ);
    SPI.endTransaction();
    digitalWrite(pins->ADXL375_CS, HIGH);
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
    } else if (sens == "LDC") {
        LDC_LHRSetup();
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
    } else if (sens == "adxl") {
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
    }
    return;
}

void capteurs::saveSens(String sens, int sensTime) {
    if (!initSens(sens)) {
        return;
    }
    // if (!sdmmcSetup()) {
    //     Serial.println("SD_MMC setup failed");
    //     return;
    // }
    // create folder for file
    int startTime = 123456789;
    if (rtc->rtcConnected) {
        DateTime startDate = rtc->rtc.now();
        startTime = startDate.unixtime();
    }
    // create folder to save chunk of data
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

    // int accTime = genVar;
    // String fn = "/" + sens + ".bin";
    String fn = name;
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
void capteurs::LDC_LHRSetup() {
    SPI.begin(pins->ADXL375_SCK, pins->ADXL375_MISO, pins->ADXL375_MOSI);
    digitalWrite(pins->Ext_SPI_CS, LOW);
    SPI.transfer(0x0A);
    SPI.transfer(0x00); //  INTB_MODE
    SPI.transfer(0x01);
    SPI.transfer(0x75); //  RP_SET
    SPI.transfer(0x04);
    SPI.transfer(0x03); //  DIG_CONF
    SPI.transfer(0x05);
    SPI.transfer(0x01); //  ALT_CONFIG
    SPI.transfer(0x0C);
    SPI.transfer(0x01); //  D_CONFIG
    SPI.transfer(0x30);
    SPI.transfer(0xCC); //  LHR_RCOUNT_LSB sample rate LSB
    SPI.transfer(0x31);
    SPI.transfer(0x77); //  LHR_RCOUNT_MSB sample rate MSB
    SPI.transfer(0x34);
    SPI.transfer(0x00); //  LHR_CONFIG
    SPI.transfer(0x32);
    SPI.transfer(0x00);
    SPI.transfer(0x33);
    SPI.transfer(0x00);
    SPI.transfer(0x0B);
    SPI.transfer(0x00); //  START_CONFIG);
    digitalWrite(pins->Ext_SPI_CS, HIGH);
}
void capteurs::LDC_LHRMesure(long *Max, long *Min, long *Moy, int duration) {
    digitalWrite(pins->Ext_SPI_CS, LOW);
    long LHR_status, LHR_LSB, LHR_MID, LHR_MSB, inductance, fsensor;
    long sum = 0;
    int count = 0;
    Max = 0;
    long test = pow(2, 24);
    Min = &test;
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
    File file = SD_MMC.open(name, FILE_WRITE);
    int start_time = millis();
    while (millis() < start_time + duration) {
        SPI.transfer(0x3B | READ); // reading status
        long LHR_status = SPI.transfer(0x00);
        if (LHR_status == 0) {
            // Serial.println("status loop");
            SPI.transfer(0x38 | READ);
            LHR_LSB = SPI.transfer(0x00);
            SPI.transfer(0x39 | READ);
            LHR_MID = SPI.transfer(0x00);
            SPI.transfer(0x3A | READ);
            LHR_MSB = SPI.transfer(0x00);
            inductance = (LHR_MSB << 16) | (LHR_MID << 8) | LHR_LSB;
            fsensor = (pow(2, sens_div) * f * inductance) / (pow(2, 24));
            if (fsensor < *Min) {
                Min = &fsensor;
            }
            if (fsensor > *Max) {
                Max = &fsensor;
            }
            if (sum < pow(2, 13) - fsensor) {
                sum += fsensor;
                count++;
            }
            file.write(LHR_MSB);
            file.write(LHR_MID);
            file.write(LHR_LSB);
        }
    }
    sum = sum / count;
    Moy = &sum;
    digitalWrite(pins->Ext_SPI_CS, HIGH);
    file.flush();
    file.close();
}
void capteurs::pinSetup() {
    pinMode(pins->ADXL375_SCK, OUTPUT);
    pinMode(pins->ADXL375_MOSI, OUTPUT);
    pinMode(pins->ADXL375_MISO, INPUT_PULLDOWN);
    pinMode(pins->ADXL375_CS, OUTPUT);
    pinMode(pins->ON_SICK, OUTPUT);
    pinMode(pins->SICK1, INPUT_PULLDOWN);
    pinMode(pins->battPin, INPUT_PULLDOWN);

    digitalWrite(pins->ADXL375_CS, HIGH);
    digitalWrite(pins->RFM95_CS, HIGH);
    digitalWrite(pins->ON_SICK, bSick);
    analogReadResolution(12);
    pinMode(pins->Ext_SPI_CS, OUTPUT);
    digitalWrite(pins->Ext_SPI_CS, HIGH);
}
