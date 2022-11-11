#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "aterm-10815d-g"
#define STAPSK  "48a530348599a"
#define LOCALHOST "wifi"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const char* llhost = LOCALHOST;

// Set AP credentials
#define AP_SSID "ESP8266-wifi"
#define AP_PASS "00000000"
#define AP_IP "192.168.4.1"

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
IPAddress ip( 192, 168, 4, 1 );
IPAddress subnet( 255, 255, 0, 0 );

//ソートデータ
#define NUM_DATA    20
typedef struct SOMEDATA {
  int val;
  String sssid;
  int channel;
  int encryptionType;
};
SOMEDATA data[NUM_DATA];
int num_data;
int temptureCount;

//クイックソート
int pivot(int i, int j) {
  int k = i + 1;
  while (k <= j && data[i].val == data[k].val) k++;
  if (k > j) return -1;
  if (data[i].val >= data[k].val) return i;
  return k;
}
int partition(int i, int j, int x) {
  int l = i, r = j;
  while (l <= r) {
    //昇順・降順は x 前の等号を入れ替える
    while (l <= j && data[l].val >= x)  l++;
    while (r >= i && data[r].val < x) r--;
    if (l > r) break;
    String t = data[l].sssid;
    int val = data[l].val;
    int channel = data[l].channel;
    int encryptionType =  data[l].encryptionType;
    data[l].sssid = data[r].sssid;
    data[l].val = data[r].val;
    data[l].channel = data[r].channel;
    data[l].encryptionType = data[r].encryptionType;
    data[r].sssid = t;
    data[r].val = val;
    data[r].channel = channel;
    data[r].encryptionType = encryptionType;
    l++; r--;
  }
  return l;
}
void quickSort(int i, int j) {
  if (i == j) return;
  int p = pivot(i, j);
  if (p != -1) {
    int k = partition(i, j, data[p].val);
    quickSort(i, k - 1);
    quickSort(k, j);
  }
}
void sort() {
  quickSort(0, num_data - 1);
}
char *changeStringToChar( String tString) {
  char cBuf[256];
  char __result[sizeof(tString)];
  tString.toCharArray(__result, sizeof(__result));
  return __result;
}
void setup() {
  Serial.begin(115200);

  // PIN D4
  // prepare LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  // Connect to WiFi network
  Serial.println("Booting");
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP(AP_SSID, AP_PASS);
    
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
  Serial.println(AP_IP);
  
  
  // Set local DNS
  //  mDNS の呼びかけに応答できるように仕込む
  Serial.println("Setting up mDNS...");
  if (MDNS.begin(llhost))         //  "xx.local" で応答
    Serial.println("mDNS started : " + (String)llhost + ".local");
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
  Serial.print("AP  IP address: ");
  Serial.println(AP_IP);
  Serial.print("STA IP address: ");
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
  String sMsg = "<br><br>";
  char cMsg[256];
  if (req.indexOf(F("/gpio/0")) != -1) {
    val = 0;
  } else if (req.indexOf(F("/gpio/1")) != -1) {
    val = 1;
  } else if (req.indexOf(F("/wifi")) != -1) {

    Serial.println("scan start");
    int n = num_data = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
      sMsg += "no networks found";
    } else {
      // sort start
      // data clear
      for (int i = 0; i < NUM_DATA; i++) {
        data[i].val = 0;
        data[i].sssid = "";
        data[i].channel = 0;
        data[i].encryptionType = 0;
      }
      // data set
      for (int i = 0; i < num_data; i++) {
        data[i].val = WiFi.RSSI(i);
        data[i].sssid = WiFi.SSID(i);
        data[i].channel = WiFi.channel(i);
        data[i].encryptionType = WiFi.encryptionType(i);
        char setMsg[64];
        memset(setMsg, '\0', sizeof(setMsg));

        // 表示フォーマット
        sprintf(setMsg, "%2d : %32s : ch %2d (%3ddbm)%d<br>", i, data[i].sssid.c_str(), data[i].channel, data[i].val, data[i].encryptionType);
        Serial.println(setMsg);
      }

      Serial.println("Sorting...");
      sort();

      for (int i = 0; i < num_data; i++) {
                char sortMsg[64];
        memset(sortMsg, '\0', sizeof(sortMsg));

        // 表示フォーマット
        sprintf(sortMsg, "%2d : %32s : ch %2d (%3ddbm)%d<br>", i, data[i].sssid.c_str(), data[i].channel, data[i].val, data[i].encryptionType);
        Serial.println(sortMsg);

      }
      //sort end

      Serial.print(n);
      sMsg += n;
      Serial.println(" networks found");
      sMsg += " networks found<br>";
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        // String to Char
        char cMsg[64];
        memset(cMsg, '\0', sizeof(cMsg));
        
        String enc = (data[i].encryptionType == ENC_TYPE_NONE) ? " " : "*";

        // 表示フォーマット
        sprintf(cMsg, "%2d : %32s : ch %2d (%3d dbm)%s<br>", i + 1, data[i].sssid.c_str(), data[i].channel, data[i].val, enc.c_str());
        Serial.println(cMsg);

        sMsg += (String)cMsg;
        delay(10);
      }
    }
    Serial.println("");
  } else {
    // Serial.println(F("invalid request"));
    val = digitalRead(LED_BUILTIN);
  }

  // Set LED according to the request
  digitalWrite(LED_BUILTIN, val);

  // read/ignore the rest of the request
  // do not client.flush(): it is for output only, see below
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  // Send the response to the client
  // it is OK for multiple small client.print/write,
  // because nagle algorithm will group them into one single packet
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO-0(D4) is now "));
  client.print((val) ? F("high") : F("low"));
  client.print(sMsg);
  client.print(F("<br><br>AP&nbsp;&nbsp;IP Address : "));
  client.print(AP_IP);
  client.print(F("<br>STA&nbsp;IP Address : "));
  client.print(WiFi.localIP());
  client.print(F("<br><br>Click <a href='http://"));
  client.print(AP_IP);
  client.print(F("/gpio/1'>here</a> to switch LED GPIO on<br> or <a href='http://"));
  client.print(AP_IP);
  client.print(F("/gpio/0'>here</a> to switch LED GPIO off<br>or <a href='http://"));
  client.print(AP_IP);
  client.print(F("/wifi'>here</a> to Search Wifi.</html>"));

  // The client will actually be *flushed* then disconnected
  // when the function returns and 'client' object is destroyed (out-of-scope)
  // flush = ensure written data are received by the other side
  Serial.println(F("Disconnecting from client"));
}
