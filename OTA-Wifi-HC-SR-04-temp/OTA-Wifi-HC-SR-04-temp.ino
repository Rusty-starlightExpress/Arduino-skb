#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

/*
    This sketch demonstrates how to set up a simple HTTP-like server.
    The server will set a GPIO pin depending on the request
      http://server_ip/gpio/0 will set the GPIO2 low,
      http://server_ip/gpio/1 will set the GPIO2 high
    server_ip is the IP address of the ESP8266 module, will be
    printed to Serial when the module is connected.
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#ifndef STASSID
#define STASSID "aterm-10815d-g"
#define STAPSK  "48a530348599a"
#define LOCALHOST "wifi"
#endif
#define echoPin 12 // Echo Pin
#define trigPin 13 // Trigger Pin

double Duration = 0; //受信した間隔
double Distance = 0; //距離
float Tempture = 25.0;//デフォルト 25度
int   Humidity = 50;
int   Lumen    = 50;
int   TemptureCount = 0;//気温取得回数
int   TemptureCountMax = 30; //気温取得最大回数

const char* ssid = STASSID;
const char* password = STAPSK;
const char* localhost = LOCALHOST;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
String Https_Get_access() {
  BearSSL::WiFiClientSecure client;
  client.setTimeout(500);
  client.setInsecure();

  const int httpPort = 443;
  const String httpHost = "uapi1.sremo.net";
  const String httpUrl = "/user_api/awn3bfqub2b/get_thl";
  const char* host2 = httpHost.c_str();
  if (!client.connect(host2, httpPort)) {
    Serial.println("connection failed");
    return "non";
  }
  client.print(String("GET ") + httpUrl + " HTTP/1.1\r\n" +
               "Host: " + httpHost + "\r\n" +
               "User-Agent: ESP8266/1.0\r\n" +
               "Authorization: Bearer Cop8s5NWnuGiImY1HQdhONGHbFA6nxghrYoxloEh\r\n\r\n");
  unsigned long timeout = micros();

  while (client.available() == 0) {
    if ( micros() - timeout  > 5000000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return "non";
    }
  }
  String line = "";
  while (client.available()) {
    line = client.readStringUntil('\r');
    //Serial.print(line);
  }
  return line;
}

void getTempture() {
  //Temp-get
  String result2 = Https_Get_access();
  Serial.println(result2);

  char json[50];
  strcpy(json, result2.c_str());

  const size_t capacity = 50;
  DynamicJsonDocument doc(capacity);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  int te = doc["t"];
  int he = doc["h"];
  int lu = doc["l"];

  Tempture = (float)te;
  Humidity = he;
  Lumen    = lu;
  // Print values.
  Serial.println("Tempture : " + (String)Tempture);
  Serial.println("Humidity : " + (String)Humidity);
  Serial.println("Lumen    : " + (String)Lumen);
}
void getDuration(){
  double tmpDuration = 0;
  while (tmpDuration == 0 || tmpDuration > 26000) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite( trigPin, HIGH ); //超音波を出力
    delayMicroseconds( 10 ); //
    digitalWrite( trigPin, LOW );
    tmpDuration = pulseIn( echoPin, HIGH ); //センサからの入力
  }
  Duration = tmpDuration;
}
void setup() {
  Serial.begin(115200);

  //　使用可能PIN
  //  D0 - GPIO16
  //  D1 - GPIO5
  //  D5 - GPIO14
  //  D6 - GPIO12
  //  D7 - GPIO13

  pinMode( echoPin, INPUT );
  pinMode( trigPin, OUTPUT );


  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.println("Booting");
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    //delay(5000);
    ESP.restart();
  }
  Serial.println();
  Serial.println(F("WiFi connected"));

  // Start the server
  server.begin();
  Serial.println(F("Server started"));

  // Print the IP address
  Serial.println(WiFi.localIP());

  // Set local DNS
  //  mDNS の呼びかけに応答できるように仕込む
  Serial.println("Setting up mDNS...");
  if (MDNS.begin(localhost))         //  "xx.local" で応答
    Serial.println("mDNS started : " + (String)localhost + ".local");
  else
    Serial.println("mDNS failed to start");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println(F("new client"));

  client.setTimeout(5000); // default is 1000

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  // Match the request
  int val;
  int retType;
  // retType 0:html  1:json

  String sMsg = "";
  if (req.indexOf(F("/json")) != -1) {
    retType = 1;
    getDuration();

    Serial.println("Senser : " + (String)Duration);
    if (TemptureCount > TemptureCountMax) {
      TemptureCount = 0;
      getTempture();
    }
    float v = (331.5 + 0.6 * Tempture) / 10000; // 音速
    float distance = v * (Duration / 2); // 距離
    Serial.print("distance:");
    Serial.print(distance);
    Serial.println(" cm");
    sMsg += "{\"distance\":";
    sMsg += distance;
    sMsg += "}";

  } else if (req.indexOf(F("/html")) != -1) {
    retType = 0;
    getDuration();
    //      Duration = Duration / 2; //往復距離を半分にする
    //      Distance = Duration * 340 * 100 / 1000000; // 音速を340m/sに設定
    float v = (331.5 + 0.6 * Tempture) / 10000; // 音速
    float distance = v * (Duration / 2); // 距離
    int tmpM = distance / 100;
    int tmpC = (int)distance % 100;
    int tmpMM = ( distance - (int)distance ) * 100;
    Serial.println("Duration: " + (String)Duration);
    Serial.print("distance: ");
    if (tmpM > 0) Serial.print(tmpM + " m ");
    Serial.print(tmpC + " cm ");
    Serial.println(tmpMM + " mm");
    sMsg += "distance: ";
    sMsg += Duration;
    sMsg += "<br>";
    if (tmpM > 0) sMsg += tmpM;
    if (tmpM > 0) sMsg += " m ";
    sMsg += tmpC;
    sMsg += " cm ";
    sMsg += tmpMM;
    sMsg += " mm<br>";

  } else if (req.indexOf(F("/thl")) != -1) {
    retType = 1;
    sMsg += "{\"Tempture\":";
    sMsg += Tempture;
    sMsg += ",\"Humidity\":";
    sMsg += Humidity;
    sMsg += ",\"Lumen\":";
    sMsg += Lumen;
    sMsg += "}";

  } else {
    //Serial.println(F("invalid request"));
    //    val = digitalRead(LED_BUILTIN);
  }

  // Set LED according to the request
  // digitalWrite(LED_BUILTIN, val);

  // read/ignore the rest of the request
  // do not client.flush(): it is for output only, see below
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }
  if (retType == 0) {
    // Send the response to the client
    // it is OK for multiple small client.print/write,
    // because nagle algorithm will group them into one single packet
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n"));
    client.print(sMsg);
    client.print(F("<br><br>Click <a href='http://"));
    client.print(WiFi.localIP());
    client.print(F("/json'>here</a> to JSON<br>"));
    client.print(F("Click <a href='http://"));
    client.print(WiFi.localIP());
    client.print(F("/html'>here</a> to HTML<br>"));
    client.print(F("Click <a href='http://"));
    client.print(WiFi.localIP());
    client.print(F("/thl'>here</a> to THL</html>"));
  } else {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: application/json charset=utf-8\r\n\r\n"));
    client.print(sMsg);
  }
  TemptureCount++;
}
