#include "capteurs.h"

capteurs::capteurs(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr, String modele) {
    rot = new angle(&dsox, modele);
    pins = p;
    if (pins->LHR_CS_1 >= 0) {
        ldc1 = new LDC(pins, pins->LHR_CS_1, pins->LHR_SWITCH_1);
    }
    if (pins->LHR_CS_2 >= 0) {
        ldc2 = new LDC(pins, pins->LHR_CS_2, pins->LHR_SWITCH_2);
    }
    if (pins->HMCX_CS >= 0) {
        HW = new hw(pins);
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
        rtc->goSleep40Days();
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
    sensors_event_t event;
    delay(200);
    dsox.getEvent(&accel, &gyro, &temp);
    rot->initangle(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, gyro.gyro.x, gyro.gyro.y, gyro.gyro.z, micros());
    Serial.println("init angle: w = " + String(rot->w) + " w_raw = " + String(rot->w_raw) + " gz = " + String(gyro.gyro.z));
    return true;
}
bool capteurs::bmeSetup() {
    if (!bme.begin(0x76)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        return false;
    }
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X16, // temperature
                    Adafruit_BME280::SAMPLING_X16, // pressure
                    Adafruit_BME280::SAMPLING_X16, // humidity
                    Adafruit_BME280::FILTER_OFF);
    bool btake = bme.takeForcedMeasurement();
    float t = bme.readTemperature();
    float p = bme.readPressure() / 100000.0;
    float u = bme.readHumidity();
    Serial.println("temperature " + String(t) + "*C");
    Serial.println("pression " + String(p) + "Bar");
    Serial.println("humidity " + String(u) + "%");
    return true;
}
bool capteurs::adsSetup() {
    Serial.println("Setup ADS1015");
    if (!ads1015.begin(0x49)) {
        Serial.println("Couldn't find ADS1015 ADC on address 0x49");
        return false;
    };
    ads1015.setGain(GAIN_TWO);
    Serial.print("Gain is : ");
    Serial.println(ads1015.getGain());
    int16_t adc0, adc1, adc2, adc3;
    adc0 = ads1015.readADC_SingleEnded(0);
    Serial.print("AIN0: ");
    Serial.println(adc0);
    adc1 = ads1015.readADC_SingleEnded(1);
    Serial.print("AIN1: ");
    Serial.println(adc1);
    adc2 = ads1015.readADC_SingleEnded(2);
    Serial.print("AIN2: ");
    Serial.println(adc2);
    adc3 = ads1015.readADC_SingleEnded(3);
    Serial.print("AIN3: ");
    Serial.println(adc3);
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

void capteurs::mesureLisse(long senstime, capteurs::Metadata meta) {
    Serial.println("#############");
    Serial.print("Meta speed = ");
    Serial.println(meta.speed);
    Serial.print("Meta angle = ");
    Serial.println(meta.initangle);
    Serial.print("Meta ring = ");
    Serial.println(meta.ring);

    file = binFile();
    file.header.addMetaData("date", rtc->rtc.now());
    unsigned long time0 = micros();
    file.header.addMetaData("id", id);
    file.header.addMetaData("speed", meta.speed);
    file.header.addMetaData("initangle", meta.initangle);
    file.header.addMetaData("ring", meta.ring);
    file.header.addMetaData("timestamp", meta.timestamp);
    measurement lsm;
    lsm.addField(field("time", "microsecond", UNSIGNED_4BYTES_B, 1));
    lsm.addField(field("acc_x", "m/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("acc_y", "m/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("acc_z", "m/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("gyro_x", "rad/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("gyro_y", "rad/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("gyro_z", "rad/s^-2", SIGNED_2BYTES_B, 100));
    lsm.nRow = 1; // async
    file.header.addMeasurement(lsm);
    measurement hmc;
    hmc.addField(field("time_set", "microsecond", UNSIGNED_4BYTES_B, 1));
    hmc.addField(field("x_set", "number", UNSIGNED_3BYTES_B, 1));
    hmc.addField(field("y_set", "number", UNSIGNED_3BYTES_B, 1));
    hmc.addField(field("z_set", "number", UNSIGNED_3BYTES_B, 1));
    hmc.addField(field("time_reset", "microsecond", UNSIGNED_4BYTES_B, 1));
    hmc.addField(field("x_reset", "number", UNSIGNED_3BYTES_B, 1));
    hmc.addField(field("y_reset", "number", UNSIGNED_3BYTES_B, 1));
    hmc.addField(field("z_reset", "number", UNSIGNED_3BYTES_B, 1));
    hmc.nRow = 10;
    file.header.addMeasurement(hmc);
    unsigned long t0 = millis();
    unsigned long ta_micro;
    String fn = getName("lisse");
    newName = fn;
    file.writeHeader(fn);
    HW->ADCsetup();
    bool s = false;
    Serial.println("begin lisse");
    r = 0;
    while (millis() < t0 + senstime * 1000) {
        ta_micro = micros() - time0;
        for (size_t j = 0; j < 4; j++) {
            sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
            r++;
        }
        getSens("lsm");
        getSens("lsmGyro");
        for (int j = 0; j < r; j++) {
            file.outFile.write(sdBuf[j]);
        }
        r = 0;
        for (int i = 0; i < 9; i++) {
            HW->SR_pwm(file.outFile, false);
        }
        HW->SR_pwm(file.outFile, true);
    }
    file.close();
    Serial.println("end lisse");
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
    delay(10);
    digitalWrite(pins->ON_SICK, HIGH);
    w0 = rot->wheelRot2();
    String fn = getName("picot");
    newName = fn;
    preferences->begin("struct", false);
    int rm = preferences->getString("RAYONMOLETTE", "229").toInt();
    int R = preferences->getString("RAD", "4500").toInt();
    preferences->end();
    // File file = SD_MMC.open(fn, FILE_WRITE);
    file = binFile();
    file.header.addMetaData("date", rtc->rtc.now());
    file.header.addMetaData("id", id);
    file.header.addMetaData("r_molette", rm);
    file.header.addMetaData("r_profil", R);
    measurement lsm;
    lsm.addField(field("time", "microsecond", UNSIGNED_4BYTES_B, 1));
    lsm.addField(field("acc_x", "m/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("acc_y", "m/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("acc_z", "m/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("gyro_x", "rad/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("gyro_y", "rad/s^-2", SIGNED_2BYTES_B, 100));
    lsm.addField(field("gyro_z", "rad/s^-2", SIGNED_2BYTES_B, 100));
    lsm.nRow = 1;
    file.header.addMeasurement(lsm);
    measurement sick;
    sick.addField(field("time", "microsecond", UNSIGNED_4BYTES_B, 1));
    sick.addField(field("sick", "number", UNSIGNED_2BYTES_B, 1));
    sick.nRow = 100;
    file.header.addMeasurement(sick);
    unsigned long t0 = millis();
    unsigned long time0 = micros();
    unsigned long ta_micro;
    file.writeHeader(fn);
    file.header.print();
    if (!file.outFile) {
        return;
    }
    int nc = 0;
    while (millis() < t0 + senstime * 1000) {
        nc++;
        ta_micro = micros() - time0;
        for (size_t j = 0; j < 4; j++) {
            sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
            r++;
        }
        getSens("lsm");
        getSens("lsmGyro");
        for (int j = 0; j < r; j++) {
            file.outFile.write(sdBuf[j]);
        }
        r = 0;
        for (int i = 0; i < 100; i++) {
            ta_micro = micros() - time0;
            for (size_t j = 0; j < 4; j++) {
                sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
                r++;
            }
            getSens("sick");
            for (size_t j = 0; j < r; j++) {

                file.outFile.write(sdBuf[j]);
            }
            r = 0;
        }
    }
    wf = rot->w;
    file.close();
    digitalWrite(pins->ON_SICK, LOW);
    Serial.print("measurement done: nb de cycles ");
    Serial.println(nc);
}

void capteurs::mesureRipper(long senstime, String sens) {

    String fn = getName(sens);
    Serial.println(fn);
    int startMillis = millis();
    // File file = SD_MMC.open(fn, FILE_WRITE);
    file = binFile();
    file.header.addMetaData("date", rtc->rtc.now());
    file.header.addMetaData("id", id);
    unsigned long time0 = micros();
    newName = fn;

    SPI.end();
    pinSetup();
    if (!initSens(sens)) {
        return;
    }
    measurement ldc;
    ldc.addField(field("time", "microsecond", UNSIGNED_4BYTES_B, 1));
    ldc.addField(field("f1", "Hz", UNSIGNED_3BYTES_B, 1));
    ldc.addField(field("f2", "Hz", UNSIGNED_3BYTES_B, 1));
    ldc.nRow = 1;
    file.header.addMeasurement(ldc);
    file.writeHeader(fn);
    if (!file.outFile) {
        return;
    }
    unsigned long t0 = millis();
    unsigned long ta_micro;
    while (millis() < t0 + senstime * 1000) {
        ta_micro = micros() - time0;
        r = 0;
        if (sens == "LDC1") {
            if (ldc1->mesure2f()) {
                for (size_t j = 0; j < 4; j++) {
                    sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
                    r++;
                }
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
            if (ldc2->mesure2f()) {
                for (size_t j = 0; j < 4; j++) {
                    sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
                    r++;
                }
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
            file.outFile.write(sdBuf[j]);
        }
        r = 0;
    }
    file.close();
}
bool capteurs::initHeader(String sens) {
    file = binFile(); // reset header
    file.header.addMetaData("date", rtc->rtc.now());
    file.header.addMetaData("id", id);
    if (sens == "lsm") {
        measurement lsm;
        lsm.addField(field("time", "microsecond", UNSIGNED_4BYTES_L, 1));
        lsm.addField(field("acc_x", "m/s^-2", FLOAT, 1));
        lsm.addField(field("acc_y", "m/s^-2", FLOAT, 1));
        lsm.addField(field("acc_z", "m/s^-2", FLOAT, 1));
        lsm.addField(field("gyro_x", "rad/s^-2", FLOAT, 1));
        lsm.addField(field("gyro_y", "rad/s^-2", FLOAT, 1));
        lsm.addField(field("gyro_z", "rad/s^-2", FLOAT, 1));
        lsm.nRow = 1;
        file.header.addMeasurement(lsm);
        file.bind("time", &t_micro);
        uint32_t v = 0;
        file.bind("sick", &v);
        file.bind("acc_x", &accel.acceleration.x);
        file.bind("acc_y", &accel.acceleration.y);
        file.bind("acc_z", &accel.acceleration.z);
        file.bind("gyro_x", &gyro.gyro.x);
        file.bind("gyro_y", &gyro.gyro.y);
        file.bind("gyro_z", &gyro.gyro.z);
    } else if (sens == "adxl") {
        measurement adxl;
        adxl.addField(field("time", "microsecond", UNSIGNED_4BYTES_L, 1));
        adxl.addField(field("acc_x", "m/s^-2", FLOAT, 1));
        adxl.addField(field("acc_y", "m/s^-2", FLOAT, 1));
        adxl.addField(field("acc_z", "m/s^-2", FLOAT, 1));
        adxl.nRow = 1;
        file.header.addMeasurement(adxl);
        file.bind("time", &t_micro);
        uint32_t v = 0;
        file.bind("sick", &v);
        file.bind("acc_x", &accel.acceleration.x);
        file.bind("acc_y", &accel.acceleration.y);
        file.bind("acc_z", &accel.acceleration.z);
    }
    if (sens == "sick") {
        measurement sick;
        sick.addField(field("time", "microsecond", UNSIGNED_4BYTES_L, 1));
        sick.addField(field("sick", "number", UNSIGNED_2BYTES_L, 1));
        sick.nRow = 1;
        file.header.addMeasurement(sick);
        file.bind("time", &t_micro);
        file.bind("sick", &v);
    } else if (sens == "LDC1") {
        if (pins->LHR_CS_1 < 0) {
            return false;
        }
        measurement ldc;
        ldc.addField(field("time", "microsecond", UNSIGNED_4BYTES_L, 1));
        ldc.addField(field("f1", "Hz", UNSIGNED_3BYTES_L, 1));
        ldc.addField(field("f2", "Hz", UNSIGNED_3BYTES_L, 1));
        ldc.nRow = 1;
        file.header.addMeasurement(ldc);
        file.bind("time", &t_micro);
        file.bind("f1", (uint32_t *)&ldc1->f1);
        file.bind("f2", (uint32_t *)&ldc1->f2);
    }
    return true;
}
bool capteurs::initSens(String sens) {
    Serial.println(sens);
    if (sens == "bme") {
        return bmeSetup();
    }
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
        return ldc1->LHRSetup();
    } else if (sens == "LDC2") {
        if (pins->LHR_CS_2 < 0) {
            return false;
        }
        return ldc2->LHRSetup();
    }
    return false;
}

void capteurs::getSens(String sens) {
    if (sens == "bme") {
        bme.takeForcedMeasurement();
        accBuffering((int)(bme.readTemperature() * 10));
        accBuffering((int)(bme.readPressure()));
        accBuffering((int)(bme.readHumidity() * 10));
    }
    if (sens == "ads") {
        int16_t adc0, adc1, adc2, adc3;
        // adc0 = ads1015.readADC_SingleEnded(0);
        // Serial.print("AIN0: ");
        // Serial.println(adc0);
        // adc1 = ads1015.readADC_SingleEnded(1);
        // Serial.print("AIN1: ");
        // Serial.println(adc1);
        adc2 = ads1015.readADC_SingleEnded(2);
        genVar = adc2;
        Serial.print("AIN2: ");
        Serial.println(adc2);
        // adc3 = ads1015.readADC_SingleEnded(3);
        // Serial.print("AIN3: ");
        // Serial.println(adc3);
    }
    if (sens == "lsm") {
        sensors_event_t event;
        dsox.getEvent(&accel, &gyro, &temp);
        accBuffering((int)(accel.acceleration.x * 100));
        accBuffering((int)(accel.acceleration.y * 100));
        accBuffering((int)(accel.acceleration.z * 100));
        /*accBuffering((int)(gyro.gyro.x * 100));
        accBuffering((int)(gyro.gyro.y * 100));
        accBuffering((int)(gyro.gyro.z * 100));
        rot->correctionangle(0.1, accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, gyro.gyro.x, gyro.gyro.y, gyro.gyro.z, micros());
        accBuffering((int)(rot->ax_raw * 100));
        accBuffering((int)(rot->ay_raw * 100));
        accBuffering((int)(rot->w_raw * 100));
        accBuffering((int)(rot->acfx * 100));
        accBuffering((int)(rot->acfy * 100));
        accBuffering((int)(rot->w * 100));
        accBuffering((int)(rot->anglef * 10));*/
        return;
    } else if (sens == "lsmGyro") {
        sensors_event_t event;
        dsox.getEvent(&accel, &gyro, &temp);
        accBuffering((int)(gyro.gyro.x * 100));
        accBuffering((int)(gyro.gyro.y * 100));
        accBuffering((int)(gyro.gyro.z * 100));
        return;
    } else if (sens == "adxl") {
        // sensors_event_t event;
        adxl->getEvent(&accel);
        accBuffering((int)(accel.acceleration.x * 100));
        accBuffering((int)(accel.acceleration.y * 100));
        accBuffering((int)(accel.acceleration.z * 100));
        return;
    }
    if (sens == "sick") {
        int micros1 = micros();
        int count1 = 0;
        float val1 = 0;
        while ((micros() - micros1) < 1000) {
            count1++;
            val1 = (float)((val1 * (count1 - 1) + (float)analogRead(pins->SICK1)) / (float)count1); // read adc
        }
        v = (uint32_t)val1;
        accBuffering(v);
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
        rot->initangle(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z, gyro.gyro.x, gyro.gyro.y, gyro.gyro.z, micros());
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
    long startTime = 123456789;
    if (rtc->rtcConnected) {
        DateTime startDate = rtc->rtc.now();
        startTime = startDate.unixtime();
    }
    String fileDate = String(startTime);
    String beginStr = fileDate.substring(0, 5);
    String endStr = fileDate.substring(5, 10);
    String name =
        "/" + sens + "/" + beginStr + "/" + endStr + "/" + sens + ".sbf";
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
    if (!initHeader(sens)) {
        return;
    }
    String fn = getName(sens);
    Serial.println(fn);
    int startMillis = millis();
    // File file = SD_MMC.open(fn, FILE_WRITE);
    file.writeHeader(fn);
    unsigned long time0 = micros();
    newName = fn;
    preferences->begin("struct", false);
    int duration = preferences->getString("DUR", "10").toInt();
    ;
    preferences->end();
    if (file.outFile) {
        while ((millis() - startMillis) < sensTime * 1000) {
            // change LED color
            float prog =
                ((float)(millis() - startMillis)) / ((float)(sensTime * 1000));
            neopixelWrite(pins->LED, pins->bright * (1.0 - prog), 0,
                          pins->bright * prog); // r->b

            // get data;
            r = 0;
            // accBuffering((int)(millis() - startMillis));
            /*unsigned long ta_micro = micros() - time0;
            for (size_t i = 0; i < 4; i++) {
                sdBuf[r] = lowByte(ta_micro >> 8 * (3 - i));
                r++;
            }*/

            getSens(sens);
            r = 0;
            t_micro = micros() - time0;
            file.writeMeasurement(0);

            // write data
            /*for (int j = 0; j < r; j++) {
                file.write(sdBuf[j]);
            }*/
        }
    }
    // file.flush();
    Serial.println("recording  finished");
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
    if (pins->HMCX_CS >= 0) {
        pinMode(pins->HMCX_CS, OUTPUT);
        digitalWrite(pins->HMCX_CS, HIGH);
        pinMode(pins->HMCY_CS, OUTPUT);
        digitalWrite(pins->HMCY_CS, HIGH);
        pinMode(pins->HMCZ_CS, OUTPUT);
        digitalWrite(pins->HMCZ_CS, HIGH);
        pinMode(pins->SR, OUTPUT);
        digitalWrite(pins->SR, LOW);
    }
    pins->all_CS_high();
    SPI.begin(pins->ADXL375_SCK, pins->ADXL375_MISO, pins->ADXL375_MOSI);
}