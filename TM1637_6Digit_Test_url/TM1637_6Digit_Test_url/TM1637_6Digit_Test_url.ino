//  TM1637TinyDisplay TEST Sketch for 6-Digit Display
//  This is a test sketch for the Arduino TM1637TinyDisplay LED Display library
//
//  Author: Jason A. Cox - @jasonacox - https://github.com/jasonacox
//  Date: 29 March 2021
//

// Includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <TM1637TinyDisplay6.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>

#ifndef STASSID
#define STASSID "aterm-10815d-g"
#define STAPSK  "48a530348599a"
#define LOCALHOST "solar"
#endif

#define DEBUG false
const char* ssid = STASSID;
const char* password = STAPSK;
const char* llhost = LOCALHOST;

int keep_length = 0;
String keep_payload;

WiFiServer server(80);
ESP8266WiFiMulti WiFiMulti;

// Module connection pins (Digital Pins)
#define CLK 4 // GPIO4 (D2)
#define DIO 5 // GPIO5 (D1)

// The amount of time (in milliseconds) between tests
#define TEST_DELAY   1250

const PROGMEM char FlashString[] = "Flash Test - 1234567890";
const PROGMEM char FlashString2[] = "good";

TM1637TinyDisplay6 display(CLK, DIO);

void setup()
{
  Serial.begin(115200);

  display.setBrightness(BRIGHT_HIGH);
  display.clear();

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
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
  if (MDNS.begin(llhost))         //  "xx.local" で応答
    Serial.println("mDNS started : " + (String)llhost + ".local");
  else
    Serial.println("mDNS failed to start");

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

void loop()
{
  ArduinoOTA.handle();

  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;
    String payload;
    if (DEBUG) Serial.print("[HTTP] begin...\n");
    http.begin(client, "http://192.168.0.9:81/data/power.txt"); //HTTP
    if (DEBUG) Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (DEBUG) Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        if (DEBUG) Serial.println(payload);
      } else {
        if (DEBUG) Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
    display.setBrightness(BRIGHT_HIGH);
    if ((payload.length() > 0 ) && (keep_payload != payload)) {
      keep_payload = payload;

      int str_len = payload.length();
      int brank = 6 - str_len;

      if (keep_length != str_len) {
        keep_length = str_len;
        display.clear();
      }
      // char length (right) and brank length
      display.showString(payload.c_str(), str_len, brank);
    }
    delay(TEST_DELAY);
  }
}
