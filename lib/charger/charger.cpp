#include "charger.h"
// test
charger::charger(pinout *p, rtcClass *r, fs::FS &f, Preferences *pr, capteurs *c, AsyncWebServer *s, AsyncWebSocket *w, comLORA *l) {
    pins = p;
    rtc = r;
    fs = &f;
    preferences = pr;
    cap = c;
    server = s;
    ws = w;
    lora = l;
    // server=new AsyncWebServer(80);
}
void charger::handleFileList(AsyncWebServerRequest *request, String folderPath) {
    String html = "<html><body><h1>File List - " + folderPath + "</h1><ul>";

    File root = SD_MMC.open(folderPath);
    if (!root) {
        Serial.println("Failed to open folder");
        request->send(500, "text/plain", "Internal Server Error");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            String folderName = file.name();
            html += "<li><a style='color:red;' href='/sd?folder=" + folderPath + "/" + folderName + "'>" + folderName + "/</a>      " +
                    "<a href='/removeFolder?filename=" + String(file.path()) + "'>delete</ a> </li> ";
        } else {
            html += "<li><a href='/download?filename=" + String(file.path()) + "'>" + String(file.path()) + "</a>     " +
                    "<a href='/removeFile?filename=" + String(file.path()) + "'>delete</ a> </li> ";
        }
        file.close();
        file = root.openNextFile();
    }
    html += "</ul><p><a href='/'><button class='button rtc obj'>HOME</button></a></p></body></html>";
    request->send(200, "text/html", html);
}
void charger::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}
String charger::printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return String();
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    String date = String(timeinfo.tm_year + 1900) + "/" +
                  String(timeinfo.tm_mon + 1) + "/" +
                  String(timeinfo.tm_mday) + " " +
                  String(timeinfo.tm_hour) + ":" +
                  String(timeinfo.tm_min) + ":" +
                  String(timeinfo.tm_sec);
    Serial.println(date);
    char unixTime[15];
    strftime(unixTime, 15, "%s", &timeinfo);
    Serial.println(unixTime);
    Serial.println();
    return date;
}
void charger::initWebSocket() {
    ws->onEvent([&](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) { return onEvent(server, client, type, arg, data, len); });
    server->addHandler(ws);
}
void charger::syncRTC() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();

    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        delay(100);
    }
    if (!rtc->rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        return;
    }

    DateTime now_esp = DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    rtc->rtc.adjust(now_esp);
}
bool charger::initSPIFFS() {
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        neopixelWrite(pins->LED, pins->bright, 0, 0); // R

        return false;
    }
    return true;
}
String charger::processor(const String &var) {
    if (var == "TIMESTAMP") {
        Serial.print(var);
        Serial.print(" : ");
        Serial.println(rtc->rtc.now().unixtime());
        DateTime dt = rtc->rtc.now();
        String date = String(dt.year()) + "/" +
                      String(dt.month()) + "/" +
                      String(dt.day()) + " " +
                      String(dt.hour()) + ":" +
                      String(dt.minute()) + ":" +
                      String(dt.second());

        date += " - " + String(rtc->rtc.now().unixtime());
        return (date);
    }
    if (var == "ID") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("id", 0);
        preferences->end();

        return (String(idRead));
    }
    if (var == "BLINK") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("blink", 5);
        preferences->end();

        return (String(idRead));
    }
    if (var == "SLEEP") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("sleep", 33);
        preferences->end();

        return (String(idRead));
    }
    if (var == "RADIUS") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("radius", 0);
        preferences->end();

        return (String(idRead));
    }
    if (var == "SLEEPMEAS") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("sleepMeas", 0); // sleep after measuring (h)
        preferences->end();

        return (String(idRead));
    }
    if (var == "SLEEPNOMEAS") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("sleepNoMeas", 0); // sleep after not measuring (min)
        preferences->end();

        return (String(idRead));
    }
    if (var == "MEASTIME") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("measTime", 0); // minute of the hour to wake up
        preferences->end();

        return (String(idRead));
    }
    if (var == "TRANSTIME") {
        preferences->begin("prefid", false);
        int idRead = preferences->getUInt("transTime", 0); // minute of the hour to transmit
        preferences->end();

        return (String(idRead));
    }
    if (var == "TOOLNUM") {
        // if(rtc->setForceWakeupDate(ssid)){}
        /*preferences->begin("prefid", false);
        String preftoolNum = preferences->getString("toolNum", ssid);
        preferences->end();*/

        return (rtc->getForceWakeupDate());
    }
    if (var == "SSID") {
        preferences->begin("prefid", false);
        String prefSSID = preferences->getString("SSID", ssid);
        preferences->end();

        return (String(prefSSID));
    }
    if (var == "PWD") {
        preferences->begin("prefid", false);
        String prefPWD = preferences->getString("PWD", password);
        preferences->end();

        return (String(prefPWD));
    }
    if (var == "HOST") {
        preferences->begin("prefid", false);
        String prefHost = preferences->getString("host", host);
        preferences->end();

        return (String(prefHost));
    }
    if (var == "GENERAL") {
        return (String(cap->genVar)); // utilisee pour acctime
    }
    if (var == "BATTERY") {
        return (String(cap->measBatt()) + "V   --   " + String(rtc->rtc.getTemperature()) + "&degC");
    }
    return String();
}
void charger::callbaque(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false, [this](const String &var) -> String { return processor(var); });
}
void charger::serverRoutes() {
    // Route for root / web page
    // server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) { callbaque(request); });
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("HOME");
        request->send(SPIFFS, "/index.html", String(), false, [this](const String &var) -> String { return processor(var); });
        // request->send(SPIFFS, "/index.html", String(), false, this->processor(var));
        // request->send(200, "text/html", "test");
    });

    // Route to load style.css file
    server->on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/style.css", "text/css");
    });
    server->on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/script.js", "text/javascript");
    });

    // Route to list all files and folders
    server->on("/sd", HTTP_GET, [](AsyncWebServerRequest *request) {
        String folder = request->arg("folder");
        if (folder == "") {
            folder = "/";
        }
        handleFileList(request, folder);
    });

    // Route to download file
    server->on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        int paramsNr = request->params();
        Serial.println(paramsNr);
        String downFile = "/1/test1.txt";

        for (int i = 0; i < paramsNr; i++) {
            const AsyncWebParameter *p = request->getParam(i);
            Serial.print("Param name: ");
            Serial.println(p->name());
            Serial.print("Param value: ");
            Serial.println(p->value());
            Serial.println("------");
            downFile = p->value();
        }
        request->send(SD_MMC, downFile, "text/plain", true);
    });

    // Route to remove file
    server->on("/removeFile", HTTP_GET, [&](AsyncWebServerRequest *request) {
        int paramsNr = request->params();
        Serial.println(paramsNr);
        String remFile = "/1/test1.txt";

        for (int i = 0; i < paramsNr; i++) {
            const AsyncWebParameter *p = request->getParam(i);
            Serial.print("Param name: ");
            Serial.println(p->name());
            Serial.print("Param value: ");
            Serial.println(p->value());
            Serial.println("------");
            remFile = p->value();
        }
        if (SD_MMC.remove(remFile)) {
            neopixelWrite(pins->LED, 0, pins->bright, 0); // G
            delay(50);
        } else {
            neopixelWrite(pins->LED, pins->bright, 0, 0); // R
            delay(50);
        };
        request->redirect("/sd");
    });

    // Route to remove folder
    server->on("/removeFolder", HTTP_GET, [&](AsyncWebServerRequest *request) {
        int paramsNr = request->params();
        Serial.println(paramsNr);
        String remFile = "/1/test1.txt";

        for (int i = 0; i < paramsNr; i++) {
            const AsyncWebParameter *p = request->getParam(i);
            Serial.print("Param name: ");
            Serial.println(p->name());
            Serial.print("Param value: ");
            Serial.println(p->value());
            Serial.println("------");
            remFile = p->value();
        }
        // if (SD_MMC.rmdir(remFile)) {
        if (dirClear(remFile)) {
            neopixelWrite(pins->LED, 0, pins->bright, 0); // G
            delay(50);
        } else {
            neopixelWrite(pins->LED, pins->bright, 0, 0); // R
            delay(50);
        };
        request->redirect("/sd");
    });

    server->on("/new", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Serial.println("accessing new");
        // Serial.println("/" + cap->type + ".bin");
        Serial.println(cap->newName);
        request->send(SD_MMC, cap->newName, "text/plain", false);
    });
    Serial.println("server ok");
}
bool charger::dirClear(String path) {
    if (SD_MMC.rmdir(path)) {
        Serial.println("folder " + path + " deleted");
        return true;
    }
    Serial.println("openning folder " + path);
    File root = SD_MMC.open(path);
    File file = root.openNextFile();
    while (file) {
        String folderName = file.name();
        if (file.isDirectory()) {
            if (!dirClear(path + "/" + folderName)) {
                return false;
            };
        } else {
            if (!SD_MMC.remove(path + "/" + folderName)) {
                Serial.println("cannot delete file " + folderName);
                return false;
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    if (!SD_MMC.rmdir(path)) {
        Serial.println("cannot delete folder " + path);
        return false;
    }
    Serial.println("folder " + path + " deleted");
    return true;
}
int charger::httpPostRequest(String serverName, String postText) {
    WiFiClient client;
    HTTPClient http;
    int response = -1;
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverName);
    // Send HTTP POST request
    http.addHeader("Content-Type", "text/plain");
    int httpResponseCode = http.POST(postText);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    // Free resources
    if (httpResponseCode > 0) {
        response = http.getString().toInt();
        Serial.println("text from post: " + String(response));
    }
    http.end();
    return response;
}
int charger::sendFlask() {
    JsonDocument info;
    info["batt"] = cap->battSend;
    info["bWifi"] = bWifi;
    info["etrNum"] = cap->id;

    String infoPost;
    serializeJson(info, infoPost);
    int responseCode = httpPostRequest(host + "/batt", infoPost);
    Serial.println(responseCode);
    if (responseCode > 0) {
        // connected flask on
        bWifi = true;
        return responseCode;
    } else if (responseCode == 0) {
        // connected flask on / no wifi
        bWifi = false;
        if (rtc->rtcConnected && rtc->chg) {
            rtc->rtc.setAlarm1(rtc->rtc.now() + 90, DS3231_A1_Date);
            rtc->rtc.clearAlarm(1);
            neopixelWrite(pins->LED, 0, 0, 0); // 0
            esp_deep_sleep(60e6);
        }
        return responseCode;
    }
    // connected flask off
    return responseCode;
}

int charger::sendSens(String type) {
    JsonDocument info;
    bool bWifi = true;
    int timestamp = rtc->rtc.now().unixtime();
    float rtcTemp = rtc->rtc.getTemperature();

    info["batt"] = cap->measBatt();
    info["bWifi"] = bWifi;
    info["etrNum"] = cap->id;
    info["timestamp"] = timestamp;
    info["rtcTemp"] = rtcTemp;
    info["type"] = type;

    String infoPost;
    serializeJson(info, infoPost);
    int responseCode = httpPostRequest(host + "/sens", infoPost);
    Serial.println(responseCode);
    if (responseCode > 0) {
        // connected flask on
        bWifi = true;
        return responseCode;
    } else if (responseCode == 0) {
        // connected flask on / no wifi
        bWifi = false;
        return responseCode;
    }
    // connected flask off
    return responseCode;
}

bool charger::manageCOM() {
    bool local = false; // chg ? listen : sleep; and try again later
    bool wConnect = false;
    wConnect = wifiConnect();
    if (wConnect) {
        int respFlask = sendFlask(); // check if server running (can send to deep sleep)
        if (respFlask > 0) {
            // flask on + wifi on
            local = false;
        } else {
            // flask on or off / local true
            local = true;
        }
    } else {
        // not connected / local true
        local = true;
    }
    return local;
}

bool charger::wifiConnect() {
    // Connect to Wi-Fi
    int count1 = 0;
    int count2 = 0;

    preferences->begin("prefid", false);
    int idRead = preferences->getUInt("id", 0);
    preferences->end();

    if (ssid == "SENSAR_OSLO") {
        Serial.println("Static Local IP : 192.168.216." + String(idRead));
        IPAddress local_IP(192, 168, 216, idRead); // Set the desired IP
        IPAddress gateway(192, 168, 216, 77);      // Set the gateway IP
        IPAddress subnet(255, 255, 255, 0);        // Set the subnet mask
        // Configure the ESP32 to use a static IP
        if (!WiFi.config(local_IP, gateway, subnet)) {
            Serial.println("Static IP configuration failed.");
        }
    }
    Serial.print("Connecting Wifi : ");
    Serial.println(ssid);
    WiFi.mode(WIFI_MODE_APSTA);
    if (WiFi.status() != WL_CONNECTED) {
        while (count1 < 3 && (WiFi.status() != WL_CONNECTED)) {
            count1++;
            WiFi.begin(ssid, password);
            // WiFi.begin(ssid);
            while (count2 < 10 && (WiFi.status() != WL_CONNECTED)) {
                count2++;
                delay(200);
                neopixelWrite(pins->LED, pins->bright, 0, 0); // R
                delay(200);
                neopixelWrite(pins->LED, 0, 0, 0); // 0
                delay(200);
                Serial.println("Connecting to WiFi..");
            }
            count2 = 0;
            if (WiFi.status() == WL_CONNECTED) {
                neopixelWrite(pins->LED, 0, pins->bright, 0); // G
                Serial.println(WiFi.localIP());
            } else {
                WiFi.disconnect(true);
            }
        }
    }
    WiFi.softAP(soft_ap_ssid, "password");

    if (!MDNS.begin("esp32")) {
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("mDNS responder started");
    }

    server->begin();

    return (WiFi.status() == WL_CONNECTED);
}
void charger::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char *)data;
        prints = JsonDocument();
        prints["print"] = message;

        if (message == "on") {
            pins->color[0] = pins->bright;
            pins->color[1] = 0.;
            pins->color[2] = pins->bright / 2;
            neopixelWrite(pins->LED, pins->color[0], pins->color[1], pins->color[2]); // rose
            testlora = true;
        } else if (message == "off") {
            pins->color[0] = 0.;
            pins->color[1] = pins->bright;
            pins->color[2] = pins->bright;
            neopixelWrite(pins->LED, pins->color[0], pins->color[1], pins->color[2]); // rose
            testlora = false;
            // neopixelWrite(LED, 0, bright, bright); // cyan
        } else if (message == "alarm") {
            rtc->goSleep(cap->genVar);
        } else if (message == "restart") {
            ESP.restart();
        } else if (message == "sync") {
            syncRTC();
        } else if (message == "sick") {
            cap->bSick = !cap->bSick;
            digitalWrite(pins->ON_SICK, cap->bSick);
            Serial.println("sick");
        } else if (message == "wifi") {
            Serial.println("wifi");
        } else if (message == "angle") {
            bAng = !bAng;
            printInt = 0;
        } else if (message == "RSSI") {
            bRSSI = !bRSSI;
            printInt = 0;
        } else if (message == "ldc") {
            bLDC = !bLDC;
            printInt = 0;
        } else if (message == "s_ldc") {
            bS_LDC = true;
        } else if (message == "hmc") {
            bHMC = !bHMC;
            printInt = 0;
        } else if (message == "lsm") {
            bLSM = !bLSM;
            printInt = 0;
        } else if (message == "s_lsm") {
            bS_LSM = true;
        } else if (message == "s_adxl") {
            bS_ADXL = true;
        } else if (message == "s_sick") {
            bS_SICK = true;
        } else if (message == "adxl") {
            cap->bADXL = !cap->bADXL;
            cap->adxl->printSensorDetails();
            Serial.println("");
        } else {
            // JSONVar myObject = JSON.parse((char *)data);
            JsonDocument myObject;
            deserializeJson(myObject, data);
            // from https://github.com/arduino-libraries/Arduino_JSON/blob/master/examples/JSONObject/JSONObject.ino
            if (myObject.containsKey("id")) {
                prints["print"] = String((const char *)myObject["id"]).toInt();

                cap->id = String((const char *)myObject["id"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("id", cap->id);
                preferences->end();
            }
            if (myObject.containsKey("blink")) {
                prints["print"] = String((const char *)myObject["blink"]).toInt();

                blink = String((const char *)myObject["blink"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("blink", blink);
                preferences->end();
            }
            if (myObject.containsKey("sleep")) {
                prints["print"] = String((const char *)myObject["sleep"]).toInt();

                int sleep = String((const char *)myObject["sleep"]).toInt();
                cap->genVar = sleep; // measure duration pour le savesens
                preferences->begin("prefid", false);
                preferences->putUInt("sleep", sleep);
                preferences->end();
            }
            if (myObject.containsKey("radius")) {
                prints["print"] = String((const char *)myObject["radius"]).toInt();

                int radius = String((const char *)myObject["radius"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("radius", radius);
                preferences->end();
            }
            if (myObject.containsKey("sleepMeas")) {
                prints["print"] = String((const char *)myObject["sleepMeas"]).toInt();

                int sleepMeas = String((const char *)myObject["sleepMeas"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("sleepMeas", sleepMeas);
                preferences->end();
            }
            if (myObject.containsKey("sleepNoMeas")) {
                prints["print"] = String((const char *)myObject["sleepNoMeas"]).toInt();

                int sleepNoMeas = String((const char *)myObject["sleepNoMeas"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("sleepNoMeas", sleepNoMeas);
                preferences->end();
            }
            if (myObject.containsKey("measTime")) {
                prints["print"] = String((const char *)myObject["measTime"]).toInt();

                int measTime = String((const char *)myObject["measTime"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("measTime", measTime);
                preferences->end();
            }
            if (myObject.containsKey("transTime")) {
                prints["print"] = String((const char *)myObject["transTime"]).toInt();

                int transTime = String((const char *)myObject["transTime"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("transTime", transTime);
                preferences->end();
            }
            if (myObject.containsKey("toolNum")) {
                /*prints["print"] = String((const char *)myObject["toolNum"]);

                String toolNum = (const char *)myObject["toolNum"];

                preferences->begin("prefid", false);
                preferences->putString("toolNum", toolNum);
                preferences->end();*/
                String toolNum = (const char *)myObject["toolNum"];
                if (rtc->setForceWakeupDate(toolNum)) {
                    prints["print"] = String((const char *)myObject["toolNum"]);
                } else {
                    prints["print"] = "error";
                }
            }
            if (myObject.containsKey("ssid")) {
                prints["print"] = String((const char *)myObject["ssid"]);

                ssid = (const char *)myObject["ssid"];

                preferences->begin("prefid", false);
                preferences->putString("SSID", ssid);
                preferences->end();
            }
            if (myObject.containsKey("pwd")) {
                prints["print"] = String((const char *)myObject["pwd"]);

                password = (const char *)myObject["pwd"];

                preferences->begin("prefid", false);
                preferences->putString("PWD", password);
                preferences->end();
            }
            if (myObject.containsKey("host")) {
                prints["print"] = String((const char *)myObject["host"]);

                host = (const char *)myObject["host"];

                preferences->begin("prefid", false);
                preferences->putString("host", host);
                preferences->end();
            }
            if (myObject.containsKey("gen")) {
                prints["print"] = String((const char *)myObject["gen"]).toInt();
                cap->genVar = String((const char *)myObject["gen"]).toInt();
            }
        }

        String stringPrints;
        serializeJson(prints, stringPrints);
        ws->textAll(stringPrints);
    }
}
void charger::setup() {
    pinMode(pins->BOOT0, INPUT_PULLUP);
    preferences->begin("prefid", false);
    cap->id = preferences->getUInt("id", 0);
    blink = preferences->getUInt("blink", 5);
    ssid = preferences->getString("SSID", ssid);
    password = preferences->getString("PWD", password);
    host = preferences->getString("host", host);
    preferences->end();
    serverRoutes();
    ElegantOTA.begin(server); // Start ElegantOTA
}
void charger::loopWS() {
    // sending with websockets
    bool bBoot0Change = (digitalRead(pins->BOOT0) != bBoot0);
    bBoot0 = bBoot0Change ? !bBoot0 : bBoot0;
    prints["BOOT0"] = bBoot0 ? "ON" : "OFF";
    if (testlora) {
        if (lora->rf95Setup()) {
            byte buf[250];
            int sendSize = 250;
            buf[0] = highByte(2);
            buf[1] = lowByte(2);
            lora->rf95->send((uint8_t *)buf, sendSize);
            lora->rf95->waitPacketSent();
        }
    }
    if (cap->bSick) {
        float sickMeas = analogRead(pins->SICK1);
        Serial.println(sickMeas);
        prints["sick"] = String(sickMeas);
    }
    if (bAng) {
        sensors_event_t event;
        cap->dsox.getEvent(&cap->accel, &cap->gyro, &cap->temp);
        cap->rot->initangle(cap->accel.acceleration.x, cap->accel.acceleration.y, cap->accel.acceleration.z, cap->accel.gyro.x, cap->accel.gyro.y, cap->accel.gyro.z, micros());
        // Serial.println(String((int)cap->rot->anglef));
        prints["angle"] = String((int)cap->rot->anglef);
    }
    if (bRSSI) {
        prints["RSSI"] = String(WiFi.RSSI());
    }
    if (bHMC) {
        SPI.end();
        cap->pinSetup();
        cap->HW->ADCsetup();

        cap->HW->r = 0;
        digitalWrite(cap->HW->set, HIGH);
        delayMicroseconds(50);
        cap->HW->Write(0x01, 0x70); // writes to CONV_START - result stored in DATA7
        cap->HW->ADC_conv();

        digitalWrite(cap->HW->set, LOW);
        delayMicroseconds(50);
        cap->HW->Write(0x01, 0x70); // writes to CONV_START - result stored in DATA7
        byte *dataPrint;
        dataPrint = cap->HW->ADC_conv();

        long x1, y1, z1, x2, y2, z2;
        int i = 0;
        x1 = (long)dataPrint[0 + i * 3] << 16 |
             (long)dataPrint[1 + i * 3] << 8 |
             (long)dataPrint[2 + i * 3];
        i++;
        y1 = (long)dataPrint[0 + i * 3] << 16 |
             (long)dataPrint[1 + i * 3] << 8 |
             (long)dataPrint[2 + i * 3];
        i++;
        z1 = (long)dataPrint[0 + i * 3] << 16 |
             (long)dataPrint[1 + i * 3] << 8 |
             (long)dataPrint[2 + i * 3];
        i++;
        x1 = (long)dataPrint[0 + i * 3] << 16 |
             (long)dataPrint[1 + i * 3] << 8 |
             (long)dataPrint[2 + i * 3];
        i++;
        y2 = (long)dataPrint[0 + i * 3] << 16 |
             (long)dataPrint[1 + i * 3] << 8 |
             (long)dataPrint[2 + i * 3];
        i++;
        z2 = (long)dataPrint[0 + i * 3] << 16 |
             (long)dataPrint[1 + i * 3] << 8 |
             (long)dataPrint[2 + i * 3];
        prints["hmc"] = String(x1 - x2) + "," + String(y1 - y2) + "," + String(z1 - z2);
    }
    if (bLDC) {
        SPI.end();
        cap->pinSetup();
        if (pins->LHR_CS_1 >= 0) {
            cap->ldc1->LHRSetup();
        }
        if (pins->LHR_CS_2 >= 0) {
            cap->ldc2->LHRSetup();
        }
        String result = "";
        // prints["ldc"]=',';
        if (pins->LHR_CS_1 >= 0) {
            // cap->initSens("LDC1");
            cap->ldc1->mesure2f();
            result += String((int)cap->ldc1->f1 / 1000) + ',' + String((int)cap->ldc1->f2 / 1000);
        }
        if (pins->LHR_CS_2 >= 0) {
            // cap->initSens("LDC2");
            cap->ldc2->mesure2f();
            result += ',' + String((int)cap->ldc2->f1 / 1000) + ',' + String((int)cap->ldc2->f2 / 1000);
        }
        prints["ldc"] = result;
    }
    if (bS_LDC) {
        cap->saveSens("LDC1", cap->genVar);
        bS_LDC = false;
        neopixelWrite(pins->LED, pins->bright, pins->bright / 2, 0); // Orange
    }
    if (bLSM) {
        cap->dsox.getEvent(&cap->accel, &cap->gyro, &cap->temp);
        prints["lsm"] = String(cap->accel.acceleration.x) + ',' +
                        String(cap->accel.acceleration.y) + ',' +
                        String(cap->accel.acceleration.z);
    }
    if (bS_LSM) {
        cap->saveSens("lsm", cap->genVar);
        bS_LSM = false;
        neopixelWrite(pins->LED, pins->bright, pins->bright / 2, 0); // Orange
    }
    if (bS_ADXL) {
        cap->saveSens("adxl", cap->genVar);
        bS_ADXL = false;
        neopixelWrite(pins->LED, pins->bright, pins->bright / 2, 0); // Orange
    }
    if (bS_SICK) {
        cap->saveSens("sick", cap->genVar);
        bS_SICK = false;
        neopixelWrite(pins->LED, pins->bright, pins->bright / 2, 0); // Orange
    }
    if (cap->bADXL) {
        cap->adxlSetup();
        sensors_event_t event;
        cap->adxl->getEvent(&event);
        Serial.println(event.acceleration.x);
        prints["adxl"] = String(event.acceleration.x) + ',' +
                         String(event.acceleration.y) + ',' +
                         String(event.acceleration.z);
    }
    if (bBoot0Change) {
        if (lora->rf95Setup()) {
            byte buf[2];
            int sendSize = 2;
            buf[0] = highByte(2);
            buf[1] = lowByte(2);
            lora->rf95->send((uint8_t *)buf, sendSize);
            lora->rf95->waitPacketSent();
        }
        Serial.println("boot0");
    }

    if (cap->bADXL || bLSM || bBoot0Change || cap->bSick || bAng || bLDC || bHMC || bRSSI) {
        String stringPrints;
        serializeJson(prints, stringPrints);
        ws->textAll(stringPrints);
    }
    ws->cleanupClients();
}
int charger::manageLoop() {
    /*
    define timeouts until next try to communicate
    define color of LED
    */
    int comTO = 30;
    bool local = manageCOM();
    if (!local) {
        // flask on
        comTO = 10;
        /*pins->color[0] = pins->bright;
        pins->color[1] = pins->bright / 2;
        pins->color[2] = 0;*/
    } else {
        if (!rtc->chg) {
            // sleep
            preferences->begin("prefid", false);
            int idRead = preferences->getUInt("sleepNoMeas", 33);
            preferences->end();
            rtc->goSleep(idRead * 60);
        } else {
            // listen local
            comTO = 60;
            pins->color[0] = pins->bright / 2;
            pins->color[1] = 0;
            pins->color[2] = pins->bright / 2;
        }
    }
    return comTO;
}
void charger::routinecharge(void (*task)()) {
    if (!rtc->chg && !taskDone) {
        task();
        taskDone = true;
        Serial.println("task done");
    }
    if (millis() > loopTO) {
        loopTO = manageLoop() * 1000 + millis();
        Serial.print("manage loop : ");
        Serial.println(loopTO);
    }
    if (millis() > wsTO) {
        loopWS();
        wsTO = millis() + wsDelay;
    }
    if (millis() > blinkTO) {
        pins->loopBlink(bBlink);
        bBlink = !bBlink;
        blinkTO = millis() + 500;
        if (rtc->rtcConnected && rtc->chg) {
            rtc->rtc.setAlarm1(rtc->rtc.now() + 10, DS3231_A1_Date);
            rtc->rtc.clearAlarm(1);
        }
    }
}

void charger::normalTask() {
    for (size_t i = 0; i < blink; i++) {
        pins->rainbowLoop(10);
    }
}
