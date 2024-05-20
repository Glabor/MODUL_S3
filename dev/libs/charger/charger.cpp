#include "charger.h"

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
            AsyncWebParameter *p = request->getParam(i);
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
            AsyncWebParameter *p = request->getParam(i);
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
            AsyncWebParameter *p = request->getParam(i);
            Serial.print("Param name: ");
            Serial.println(p->name());
            Serial.print("Param value: ");
            Serial.println(p->value());
            Serial.println("------");
            remFile = p->value();
        }
        if (SD_MMC.rmdir(remFile)) {
            neopixelWrite(pins->LED, 0, pins->bright, 0); // G
            delay(50);
        } else {
            neopixelWrite(pins->LED, pins->bright, 0, 0); // R
            delay(50);
        };
        request->redirect("/sd");
    });

    Serial.println("server ok");
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
    String infoPost;
    serializeJson(info, infoPost);
    int responseCode = httpPostRequest("http://LAPTOP-TF0BBSC1:5000/batt", infoPost);
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
        int respFlask = sendFlask();
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
    WiFi.mode(WIFI_MODE_APSTA);
    if (WiFi.status() != WL_CONNECTED) {
        while (count1 < 3 && (WiFi.status() != WL_CONNECTED)) {
            count1++;
            WiFi.begin(ssid, password);
            // WiFi.begin(ssid);
            while (count2 < 3 && (WiFi.status() != WL_CONNECTED)) {
                count2++;
                delay(100);
                neopixelWrite(pins->LED, pins->bright, 0, 0); // R
                delay(100);
                neopixelWrite(pins->LED, 0, 0, 0); // 0
                delay(100);
                Serial.println("Connecting to WiFi..");
            }
            if (WiFi.status() == WL_CONNECTED) {
                neopixelWrite(pins->LED, 0, pins->bright, 0); // G
                Serial.println(WiFi.localIP());
            } else {
                WiFi.disconnect(true);
            }
        }
    }
    WiFi.softAP(soft_ap_ssid, soft_ap_password);

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

        } else if (message == "off") {
            pins->color[0] = 0.;
            pins->color[1] = pins->bright;
            pins->color[2] = pins->bright;
            neopixelWrite(pins->LED, pins->color[0], pins->color[1], pins->color[2]); // rose
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

                id = String((const char *)myObject["id"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("id", id);
                preferences->end();
            }
            if (myObject.containsKey("blink")) {
                prints["print"] = String((const char *)myObject["blink"]).toInt();

                blink = String((const char *)myObject["blink"]).toInt();

                preferences->begin("prefid", false);
                preferences->putUInt("blink", blink);
                preferences->end();
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
    preferences->begin("prefid", false);
    id = preferences->getUInt("id", 0);
    blink = preferences->getUInt("blink", 5);
    preferences->end();
    serverRoutes();
    preferences->begin("prefid", false);
    ssid = preferences->getString("SSID", ssid);
    password = preferences->getString("PWD", password);
    preferences->end();
}
void charger::loopWS() {
    // sending with websockets
    bool bBoot0Change = (digitalRead(pins->BOOT0) != bBoot0);
    bBoot0 = bBoot0Change ? !bBoot0 : bBoot0;
    prints["BOOT0"] = bBoot0 ? "ON" : "OFF";

    if (cap->bSick) {
        float sickMeas = analogRead(pins->SICK1);
        Serial.println(sickMeas);
        prints["sick"] = String(sickMeas);
    }
    if (bLSM) {
        cap->dsox.getEvent(&cap->accel, &cap->gyro, &cap->temp);
        prints["lsm"] = String(cap->accel.acceleration.x) + ',' +
                        String(cap->accel.acceleration.y) + ',' +
                        String(cap->accel.acceleration.z);
    }
    if (bS_LSM) {
        cap->saveSens("lsm");
        bS_LSM = false;
        neopixelWrite(pins->LED, pins->bright, pins->bright / 2, 0); // Orange
    }
    if (bS_ADXL) {
        cap->saveSens("adxl");
        bS_ADXL = false;
        neopixelWrite(pins->LED, pins->bright, pins->bright / 2, 0); // Orange
    }
    if (bS_SICK) {
        cap->saveSens("sick");
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

    if (cap->bADXL || bLSM || bBoot0Change || cap->bSick) {
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
        pins->color[0] = pins->bright;
        pins->color[1] = pins->bright / 2;
        pins->color[2] = 0;
    } else {
        if (!rtc->chg) {
            // sleep
            rtc->goSleep(30);
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
void charger::routinecharge() {
    if (!rtc->chg && !taskDone) {
        normalTask();
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
        if (rtc->chg) {
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
