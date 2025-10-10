#include <WiFi.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEBeacon.h>
#include <BLEAdvertising.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ===== CONFIG =====
const char* ssid = "kokokaka";
const char* password = "Bvcxsdfgtrew2345";
IPAddress local_IP(192, 168, 1, 50);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const char* serverUrl = "http://192.168.1.122:3000/api/scan";

// ===== ESP32 Beacon info =====
#define BEACON_UUID "E2C56DB5-DFFB-48D2-B060-D0F5A71096E0"
#define BEACON_MAJOR 1
#define BEACON_MINOR 1
#define BEACON_TX_POWER -59
#define BEACON_NAME "ESP32_Beacon"

// ===== Timing =====
const int SCAN_SECONDS = 4;
const int POST_DELAY_MS = 200;

// ===== Storage =====
const int MAX_FOUND = 80;

struct Found {
  String mac;
  String name;
  int rssi;
  String advHex;
  String protocol;
  String uuid;
  String tx_beacon_name;
  bool valid;
};

Found foundArr[MAX_FOUND];
BLEScan* pBLEScan;
String ownMAC;

// ===== UTILITIES =====
String toHex(const String &s) {
  String out = "";
  for (unsigned int i = 0; i < s.length(); ++i) {
    uint8_t b = (uint8_t)s[i];
    if (b < 0x10) out += "0";
    out += String(b, HEX);
  }
  out.toUpperCase();
  return out;
}

// ===== WiFi =====
void connectWiFi() {
  WiFi.config(local_IP, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi");
  int tries=0;
  while(WiFi.status()!=WL_CONNECTED && tries<40){ delay(500); Serial.print("."); tries++; }
  Serial.println();
  Serial.println("✅ WiFi IP: " + WiFi.localIP().toString());
  ownMAC = WiFi.macAddress();
}

// ===== Beacon Advertise =====
BLEAdvertising* pAdvGlobal;
void setupBeaconAdvert() {
  BLEDevice::init(BEACON_NAME);

  BLEBeacon beacon;
  beacon.setManufacturerId(0x004C);
  beacon.setProximityUUID(BLEUUID(BEACON_UUID));
  beacon.setMajor(BEACON_MAJOR);
  beacon.setMinor(BEACON_MINOR);
  beacon.setSignalPower(BEACON_TX_POWER);

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x04);
  String serviceData = "";
  serviceData += (char)26;
  serviceData += (char)0xFF;
  serviceData += beacon.getData();
  advertisementData.addData(serviceData);
  advertisementData.setName(BEACON_NAME);

  pAdvGlobal = BLEDevice::getAdvertising();
  pAdvGlobal->setAdvertisementData(advertisementData);
  pAdvGlobal->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  pAdvGlobal->start(); // ✅ Continuous advertising
}

// ===== Found Buffer =====
void resetFound() { for(int i=0;i<MAX_FOUND;i++) foundArr[i].valid=false; }
void putFound(const Found &f) {
  for(int i=0;i<MAX_FOUND;i++){
    if(!foundArr[i].valid){ foundArr[i]=f; foundArr[i].valid=true; return; }
    if(foundArr[i].mac.equalsIgnoreCase(f.mac) && f.rssi>foundArr[i].rssi){ foundArr[i].valid=true; foundArr[i]=f; foundArr[i]=f; return; }
  }
}

// ===== Build JSON Payload =====
String buildPayload() {
  String s = "[";
  bool first = true;
  unsigned long ts = millis();
  for(int i=0;i<MAX_FOUND;i++){
    if(!foundArr[i].valid) continue;
    if(!first) s += ",";
    first=false;
    Found &f = foundArr[i];
    s += "{";
    s += "\"mac\":\""+f.mac+"\","; 
    s += "\"name\":\""+f.name+"\","; 
    s += "\"type\":\"BLE\","; 
    s += "\"protocol\":\""+f.protocol+"\","; 
    s += "\"uuid\":null,"; 
    s += "\"major\":"+String(BEACON_MAJOR)+",";
    s += "\"minor\":"+String(BEACON_MINOR)+",";
    s += "\"tx_power\":"+String(BEACON_TX_POWER)+",";
    s += "\"tx_beacon_name\":\""+String(BEACON_NAME)+"\","; 
    s += "\"rssi\":"+String(f.rssi)+",";
    s += "\"adv_data\":\""+f.advHex+"\","; 
    s += "\"ts\":"+String((unsigned long long)ts);
    s += "}";
  }
  s += "]";
  return s;
}

// ===== POST JSON =====
bool postJson(const String &payload){
  if(WiFi.status()!=WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type","application/json");
  int code = http.POST(payload);
  if(code>0){
    String resp = http.getString();
    if(resp.length()) Serial.println("Resp: "+resp);
  }
  http.end();
  return (code>=200 && code<300);
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("\n=== ESP32: iBeacon Tx + BLE Scanner ===");
  connectWiFi();
  setupBeaconAdvert(); // Start advertising once
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  resetFound();
}

// ===== Loop =====
void loop() {
  if(WiFi.status()!=WL_CONNECTED){ WiFi.reconnect(); delay(2000); }

  // Scan
  BLEScanResults* results = pBLEScan->start(SCAN_SECONDS,false);
  if(!results){ delay(100); return; }
  int cnt = results->getCount();

  for(int i=0;i<cnt;i++){
    BLEAdvertisedDevice dev = results->getDevice(i);
    String mac = dev.getAddress().toString().c_str();
    if(mac.equalsIgnoreCase(ownMAC)) continue;

    Found f;
    f.mac = mac;
    f.name = dev.getName().c_str()?String(dev.getName().c_str()):"Unknown";
    f.rssi = dev.getRSSI();
    f.advHex = toHex(dev.getManufacturerData());
    f.protocol = "BLE";
    f.tx_beacon_name = BEACON_NAME; // Transmitter beacon
    putFound(f);
  }

  pBLEScan->clearResults();

  // Send payload if any
  bool any=false; 
  for(int i=0;i<MAX_FOUND;i++) if(foundArr[i].valid){ any=true; break; }

  if(any){ 
    String payload = buildPayload(); 
    Serial.println("Payload:"); 
    Serial.println(payload); 
    postJson(payload); 
  }

  delay(POST_DELAY_MS);
}
