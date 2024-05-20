#include <WiFi.h>
#include <WiFiUdp.h>

// const char* ssid = "raspi-Wifi";
// const char* password = "sensarPWD";
const char* ssid = "Livebox-7340";
const char* password = "DAAeXFXNNdb2hkdT7Y";
// String ssid = "SENSAR 4661";
// String password = "H6{3g897";

const char* host = "chg.local";  // Change this to the website you want to fetch data from
const char* udpAddress = "LAPTOP-TF0BBSC1";
// const char* udpAddress = "SENSAR";
unsigned int servUdpPort = 4210;  //  port to listen on
WiFiUDP Udp;

int lastTime = 0;
int delayTime = 1000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // // Load webpage and print content
  // Serial.println("Fetching data from webpage:");
  getDataFromWebpage();
  lastTime = millis();
}

void getDataFromWebpage() {
  WiFiClient client;

  if (!client.connect(host, 5000)) {
    Serial.println("Connection failed");
    return;
  }

  // Send HTTP request
  client.print(String("GET / HTTP/1.1\r\n") + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  // Wait for the server's response
  while (!client.available()) {
    delay(10);
  }

  // Read the response from the server
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  client.stop();
}

void sendUDP(String mess) {
  Udp.beginPacket(udpAddress, servUdpPort);
  Udp.print(mess);
  Udp.endPacket();
  // Serial.println("sent");
}

void loop() {
  if (millis() - lastTime > delayTime) {
    lastTime = millis();
    Serial.print("RRSI: ");
    String wifiRSSI = String(WiFi.RSSI());
    Serial.println(wifiRSSI);
    sendUDP(wifiRSSI);
  }
}
