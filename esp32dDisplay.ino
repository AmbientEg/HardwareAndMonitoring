#include <WiFi.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_wifi.h>

// WiFi Configuration
const char* ssid = "kokokaka";
const char* password = "Bvcxsdfgtrew2345";
IPAddress local_IP(192, 168, 1, 50);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// BLE Configuration
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BEACON_NAME         "ESP32_IPS_BEACON"

// Web Server
WebServer server(80);

// BLE Objects
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLEScan* pBLEScan = NULL;
bool deviceConnected = false;

// Data Storage Structures
struct BLEDeviceData {
  String address;
  String name;
  int rssi;
  float distance;
  unsigned long lastSeen;
};

struct WiFiDeviceData {
  String mac;
  String ssid;
  int rssi;
  int channel;
  int authMode;
  unsigned long lastSeen;
};

std::vector<BLEDeviceData> bleDevices;
std::vector<WiFiDeviceData> wifiDevices;

// Scanning flags
bool scanningBLE = false;
unsigned long lastBLEScan = 0;
unsigned long lastWiFiScan = 0;
const unsigned long BLE_SCAN_INTERVAL = 5000;  // 5 seconds
const unsigned long WIFI_SCAN_INTERVAL = 10000; // 10 seconds

// Calculate distance from RSSI using path loss model (Forward declaration)
float calculateDistance(int rssi, int txPower);

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Client Disconnected");
      BLEDevice::startAdvertising();
    }
};

// BLE Scan Callbacks
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      // Calculate distance from RSSI (rough estimation)
      int rssi = advertisedDevice.getRSSI();
      float distance = calculateDistance(rssi, -59); // -59 is typical RSSI at 1m
      
      BLEDeviceData device;
      device.address = advertisedDevice.getAddress().toString().c_str();
      device.name = advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "Unknown";
      device.rssi = rssi;
      device.distance = distance;
      device.lastSeen = millis();
      
      // Update or add device
      bool found = false;
      for (auto& dev : bleDevices) {
        if (dev.address == device.address) {
          dev = device;
          found = true;
          break;
        }
      }
      if (!found && bleDevices.size() < 50) { // Limit to 50 devices
        bleDevices.push_back(device);
      }
    }
};

// Calculate distance from RSSI using path loss model
float calculateDistance(int rssi, int txPower) {
  if (rssi == 0) {
    return -1.0;
  }
  
  float ratio = rssi * 1.0 / txPower;
  if (ratio < 1.0) {
    return pow(ratio, 10);
  } else {
    float distance = (0.89976) * pow(ratio, 7.7095) + 0.111;
    return distance;
  }
}

// Initialize BLE Server
void initBLEServer() {
  BLEDevice::init(BEACON_NAME);
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("ESP32 IPS Beacon Ready");
  
  // Start service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Server started and advertising");
}

// Initialize BLE Scanner
void initBLEScanner() {
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

// Scan for WiFi networks and extract device information
void scanWiFiNetworks() {
  int n = WiFi.scanNetworks(false, false, false, 300);
  
  if (n > 0) {
    wifiDevices.clear();
    for (int i = 0; i < n; i++) {
      WiFiDeviceData device;
      device.mac = WiFi.BSSIDstr(i);
      device.ssid = WiFi.SSID(i);
      device.rssi = WiFi.RSSI(i);
      device.channel = WiFi.channel(i);
      device.authMode = WiFi.encryptionType(i);
      device.lastSeen = millis();
      
      if (wifiDevices.size() < 50) { // Limit to 50 networks
        wifiDevices.push_back(device);
      }
    }
    Serial.printf("WiFi scan complete: %d networks found\n", n);
  }
  WiFi.scanDelete();
}

// Web server handlers
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 IPS Beacon</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
  html += "h1 { color: #333; }";
  html += "h2 { color: #666; margin-top: 30px; }";
  html += ".container { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  html += "table { width: 100%; border-collapse: collapse; margin-top: 10px; }";
  html += "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }";
  html += "th { background-color: #4CAF50; color: white; }";
  html += "tr:hover { background-color: #f5f5f5; }";
  html += ".info { background: #e3f2fd; padding: 10px; border-radius: 4px; margin: 10px 0; }";
  html += ".status { display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 0.9em; }";
  html += ".status.online { background: #4CAF50; color: white; }";
  html += ".refresh { background: #2196F3; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 10px 0; }";
  html += ".refresh:hover { background: #0b7dda; }";
  html += "</style>";
  html += "<script>";
  html += "function autoRefresh() { setTimeout(function(){ location.reload(); }, 10000); }";
  html += "</script>";
  html += "</head><body onload='autoRefresh()'>";
  html += "<div class='container'>";
  html += "<h1>🎯 ESP32 IPS Beacon Dashboard</h1>";
  
  // Beacon Info
  html += "<div class='info'>";
  html += "<strong>Beacon Status:</strong> <span class='status online'>ONLINE</span><br>";
  html += "<strong>IP Address:</strong> " + local_IP.toString() + "<br>";
  html += "<strong>BLE Name:</strong> " + String(BEACON_NAME) + "<br>";
  html += "<strong>Uptime:</strong> " + String(millis() / 1000) + " seconds<br>";
  html += "<strong>WiFi RSSI:</strong> " + String(WiFi.RSSI()) + " dBm";
  html += "</div>";
  
  // BLE Devices Table
  html += "<h2>📱 Detected BLE Devices (" + String(bleDevices.size()) + ")</h2>";
  html += "<table><tr><th>Address</th><th>Name</th><th>RSSI (dBm)</th><th>Distance (m)</th><th>Last Seen</th></tr>";
  
  for (const auto& dev : bleDevices) {
    unsigned long timeSince = (millis() - dev.lastSeen) / 1000;
    html += "<tr>";
    html += "<td>" + dev.address + "</td>";
    html += "<td>" + dev.name + "</td>";
    html += "<td>" + String(dev.rssi) + "</td>";
    html += "<td>" + String(dev.distance, 2) + "</td>";
    html += "<td>" + String(timeSince) + "s ago</td>";
    html += "</tr>";
  }
  html += "</table>";
  
  // WiFi Networks Table
  html += "<h2>📡 Detected WiFi Networks (" + String(wifiDevices.size()) + ")</h2>";
  html += "<table><tr><th>MAC Address</th><th>SSID</th><th>RSSI (dBm)</th><th>Channel</th><th>Security</th></tr>";
  
  for (const auto& dev : wifiDevices) {
    String authType;
    switch (dev.authMode) {
      case WIFI_AUTH_OPEN: authType = "Open"; break;
      case WIFI_AUTH_WEP: authType = "WEP"; break;
      case WIFI_AUTH_WPA_PSK: authType = "WPA"; break;
      case WIFI_AUTH_WPA2_PSK: authType = "WPA2"; break;
      case WIFI_AUTH_WPA_WPA2_PSK: authType = "WPA/WPA2"; break;
      case WIFI_AUTH_WPA2_ENTERPRISE: authType = "WPA2-E"; break;
      default: authType = "Unknown"; break;
    }
    
    html += "<tr>";
    html += "<td>" + dev.mac + "</td>";
    html += "<td>" + dev.ssid + "</td>";
    html += "<td>" + String(dev.rssi) + "</td>";
    html += "<td>" + String(dev.channel) + "</td>";
    html += "<td>" + authType + "</td>";
    html += "</tr>";
  }
  html += "</table>";
  
  html += "<button class='refresh' onclick='location.reload()'>🔄 Refresh Now</button>";
  html += "<p style='color: #999; font-size: 0.9em;'>Page auto-refreshes every 10 seconds</p>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleBLEData() {
  String json = "{\"ble_devices\":[";
  for (size_t i = 0; i < bleDevices.size(); i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"address\":\"" + bleDevices[i].address + "\",";
    json += "\"name\":\"" + bleDevices[i].name + "\",";
    json += "\"rssi\":" + String(bleDevices[i].rssi) + ",";
    json += "\"distance\":" + String(bleDevices[i].distance, 2) + ",";
    json += "\"last_seen\":" + String(bleDevices[i].lastSeen);
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleWiFiData() {
  String json = "{\"wifi_devices\":[";
  for (size_t i = 0; i < wifiDevices.size(); i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"mac\":\"" + wifiDevices[i].mac + "\",";
    json += "\"ssid\":\"" + wifiDevices[i].ssid + "\",";
    json += "\"rssi\":" + String(wifiDevices[i].rssi) + ",";
    json += "\"channel\":" + String(wifiDevices[i].channel) + ",";
    json += "\"auth_mode\":" + String(wifiDevices[i].authMode);
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 IPS Beacon Starting ===");
  
  // Configure static IP
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Failed to configure static IP");
  }
  
  // Connect to WiFi
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("WiFi RSSI: ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println("\nWiFi Connection Failed!");
  }
  
  // Initialize BLE
  Serial.println("Initializing BLE...");
  initBLEServer();
  initBLEScanner();
  
  // Setup Web Server
  server.on("/", handleRoot);
  server.on("/api/ble", handleBLEData);
  server.on("/api/wifi", handleWiFiData);
  server.begin();
  Serial.println("Web Server started");
  
  // Initial WiFi scan
  Serial.println("Performing initial WiFi scan...");
  scanWiFiNetworks();
  
  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  // Handle web server
  server.handleClient();
  
  // Periodic BLE scan
  if (millis() - lastBLEScan > BLE_SCAN_INTERVAL) {
    Serial.println("Starting BLE scan...");
    BLEScanResults* foundDevices = pBLEScan->start(3, false); // Scan for 3 seconds
    Serial.printf("BLE scan complete: %d devices found\n", foundDevices->getCount());
    pBLEScan->clearResults();
    lastBLEScan = millis();
  }
  
  // Periodic WiFi scan
  if (millis() - lastWiFiScan > WIFI_SCAN_INTERVAL) {
    Serial.println("Starting WiFi scan...");
    scanWiFiNetworks();
    lastWiFiScan = millis();
  }
  
  // Broadcast beacon status via BLE characteristic
  if (deviceConnected) {
    String status = "Beacon Active | BLE: " + String(bleDevices.size()) + 
                   " | WiFi: " + String(wifiDevices.size());
    pCharacteristic->setValue(status.c_str());
    pCharacteristic->notify();
  }
  
  delay(100);
}