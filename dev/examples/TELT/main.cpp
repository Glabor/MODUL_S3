// Import required libraries
#include <Preferences.h>

// Import modules
#include "capteurs.h"
#include "charger.h"
#include "comLORA.h"
#include "espnow.h"
#include "pinout.h"
#include "rtcClass.h"

String cardModel = "v3.1";
String breakout = "HMCv2";
// String etrierModel = "hmcL17";
String etrierModel = "etrier17";

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

pinout pins(cardModel, breakout);
rtcClass rtc(&preferences, SD_MMC);
capteurs cap(&pins, &rtc, SD_MMC, &preferences, etrierModel);
comLORA lora(&pins, &cap, &preferences);
charger charge(&pins, &rtc, SD_MMC, &preferences, &cap, &server, &ws, &lora);

void accBuffering(int meas, byte *sdBuf, int r) {
    // divide int to two bytes
    sdBuf[r] = highByte(meas);
    r++;
    sdBuf[r] = lowByte(meas);
    r++;
}

bool mesureRSSI(long sensTime) {
    binFile file = binFile();
    file.header.addMetaData("date", rtc.rtc.now());
    file.header.addMetaData("id", cap.id);
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

    measurement rssi;
    rssi.addField(field("time", "microsecond", UNSIGNED_4BYTES_B, 1));
    rssi.addField(field("rssi", "number", SIGNED_2BYTES_B, 1));
    rssi.nRow = 1;
    file.header.addMeasurement(rssi);

    unsigned long t0 = millis();
    unsigned long time0 = micros();
    String fn = cap.getName("rssi");
    cap.newName = fn;
    file.writeHeader(fn);

    int r = 0;
    byte sdBuf[100];
    if (charge.wifiConnect()) {

        while (millis() < t0 + sensTime * 1000) {
            unsigned long ta_micro = micros() - time0;
            for (size_t j = 0; j < 4; j++) {
                sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
                r++;
            }

            sensors_event_t event;
            cap.dsox.getEvent(&cap.accel, &cap.gyro, &cap.temp);
            accBuffering((int)(cap.accel.acceleration.x * 100), sdBuf, r);
            r += 2;
            accBuffering((int)(cap.accel.acceleration.y * 100), sdBuf, r);
            r += 2;
            accBuffering((int)(cap.accel.acceleration.z * 100), sdBuf, r);
            r += 2;
            accBuffering((int)(cap.gyro.gyro.x * 100), sdBuf, r);
            r += 2;
            accBuffering((int)(cap.gyro.gyro.y * 100), sdBuf, r);
            r += 2;
            accBuffering((int)(cap.gyro.gyro.z * 100), sdBuf, r);
            r += 2;

            ta_micro = micros() - time0;
            for (size_t j = 0; j < 4; j++) {
                sdBuf[r] = lowByte(ta_micro >> 8 * (3 - j));
                r++;
            }

            int RSSI = WiFi.RSSI();
            for (size_t j = 0; j < 2; j++) {
                sdBuf[r] = lowByte(RSSI >> 8 * (1 - j));
                r++;
            }
            for (int j = 0; j < r; j++) {
                file.outFile.write(sdBuf[j]);
            }
            delay(20);
            r = 0;
        }
    } else {
        return false;
    }
    return true;
}

void sendRSSI(int time) {
    // pins.rainbowLoop(10);
    int milli0 = millis();
    while (millis() - milli0 < time * 1000) {
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
        delay(200);
    }
}

// struct Metadata {
//     int speed;
//     int angle;
//     int ring;
// };

capteurs::Metadata askRot() {
    capteurs::Metadata meta;
    JsonDocument info;
    // info["batt"] = cap.battSend;
    // info["etrNum"] = cap.id;
    info["batt"] = cap.battSend;
    info["etrNum"] = cap.id;
    info["temp"] = (int)(rtc.rtc.getTemperature() * 100);
    info["time"] = (int)(rtc.rtc.now().unixtime());

    String infoPost;
    serializeJson(info, infoPost);
    Serial.println(infoPost);
    int responseCode = charge.httpPostRequest(charge.host + "/rot", infoPost);

    WiFiClient client;
    HTTPClient http;
    int response = -1;
    // Your Domain name with URL path or IP address with path
    http.begin(client, charge.host + "/meta");
    // Send HTTP POST request
    http.addHeader("Content-Type", "text/plain");
    int httpResponseCode = http.POST(infoPost);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    // Free resources
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("text from post: " + String(response));
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (error) {
            Serial.print(F("Erreur JSON: "));
            Serial.println(error.f_str());
        }
        int ring = doc["ring"];
        int speed = doc["speed"];
        int angle = doc["angle"];
        int time = doc["time"];
        meta.ring = ring;
        meta.speed = speed;
        meta.initangle = angle;
        meta.timestamp = time;
    }

    http.end();
    return meta;

    Serial.println(responseCode);
    Serial.print("rotation speed (rpm) : ");
    Serial.println(((float)responseCode / 1000));
    return meta;
}

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

void mainHMC() {
    Serial.println("main");

    capteurs::Metadata meta;
    float w = cap.rot->wheelRot2();
    if (charge.wifiConnect()) {
        meta = askRot();
        if (meta.speed >= 0) {
            w = ((float)meta.speed) * 2.0 * M_PI / 60.0 / 1000.0;
        }
    }

    Serial.println(w);
    if (abs(w) > 0.5 * 2.0 * M_PI / 60.0) { // rotation <0.5rpm

        bool transmitted = false;
        // cap.type = "hmc";
        // cap.mesureLisse(180, meta);
        // if (charge.wifiConnect()) {
        //     transmitted = charge.POSTNew("hmc");
        // }
        // delay(1000);

        // sendRSSI(10);
        // WiFi.disconnect(true);
        // WiFi.mode(WIFI_OFF);

        // mesureRSSI(20);
        // if (charge.wifiConnect()) {
        //     transmitted = charge.POSTNew("rssi");
        // }
        // delay(1000);

        cap.type = "sick";
        cap.saveSens(cap.type, 20);
        if (charge.wifiConnect()) {
            transmitted = charge.POSTNew("sick");
        }
        delay(1000);
        // if (!transmitted) {
        //     sendFile(cap.newName);
        // }

        cap.type = "adxl";
        cap.saveSens(cap.type, 20);
        if (charge.wifiConnect()) {
            transmitted = charge.POSTNew("adxl");
            // charge.sendSens(cap.type);
        }
        delay(1000);
    }
}

void loop() {
    // LDCTest();
    charge.routinecharge(&mainHMC);
}