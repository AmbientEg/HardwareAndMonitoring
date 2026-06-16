#include <WiFi.h>
#include <ArduinoOTA.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include "BLEBeacon.h"

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

// iBeacon parameters
#define BEACON_UUID "E2C56DB5-DFFB-48D2-B060-D0F5A71096E0"
#define BEACON_MAJOR 2
#define BEACON_MINOR 1
#define BEACON_TX_POWER -59  // Calibrated signal strength at 1m

void setup() {
  Serial.begin(115200);
  Serial.println("Starting LodgyBeacon...");

  // --------- Connect to Wi-Fi ---------
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  // --------- Initialize Arduino OTA ---------
  ArduinoOTA.setHostname("LodgyESP32");

  ArduinoOTA.begin();
  Serial.println("OTA Ready");

  // --------- Initialize BLE iBeacon ---------
  BLEDevice::init("LodgyBeacon"); // Local name visible to scanners

  BLEBeacon beacon;
  beacon.setManufacturerId(0x004C);  // Apple's company ID
  beacon.setProximityUUID(BLEUUID(BEACON_UUID));
  beacon.setMajor(BEACON_MAJOR);
  beacon.setMinor(BEACON_MINOR);
  beacon.setSignalPower(BEACON_TX_POWER);

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  BLEAdvertisementData advertisementData;

  advertisementData.setFlags(0x04);  // BR_EDR_NOT_SUPPORTED
  advertisementData.setManufacturerData(beacon.getData());  

  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  pAdvertising->start();

  Serial.println("Beacon started:");
  Serial.print("   UUID: "); Serial.println(BEACON_UUID);
  Serial.print("   Major: "); Serial.println(BEACON_MAJOR);
  Serial.print("   Minor: "); Serial.println(BEACON_MINOR);
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();

  // Keep your beacon running
  delay(1000);
}
