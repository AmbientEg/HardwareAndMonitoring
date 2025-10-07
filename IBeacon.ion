/*
 * ESP32 BLE iBeacon Example
 * Equivalent to the SwiftUI beacon app (UUID, major, minor, name)
 * Based on the work of Neil Kolban and the Arduino ESP32 BLE library
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLEBeacon.h"

#define BEACON_UUID "E2C56DB5-DFFB-48D2-B060-D0F5A71096E0"
#define BEACON_MAJOR 1
#define BEACON_MINOR 1
#define BEACON_TX_POWER -59  // Calibrated signal strength at 1m

void setup() {
  Serial.begin(115200);
  Serial.println("🔵 Starting RodynaBeacon...");

  // Initialize BLE
  BLEDevice::init("RodynaBeacon"); // Local name visible to scanners

  BLEServer *pServer = BLEDevice::createServer();

  // Create beacon data
  BLEBeacon beacon;
  beacon.setManufacturerId(0x004C);  // Apple's company ID
  beacon.setProximityUUID(BLEUUID(BEACON_UUID));
  beacon.setMajor(BEACON_MAJOR);
  beacon.setMinor(BEACON_MINOR);
  beacon.setSignalPower(BEACON_TX_POWER);

  BLEAdvertisementData advertisementData;
  BLEAdvertisementData scanResponseData;

  // Add beacon payload to advertisement
  advertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED
  String serviceData;  // ✅ Use Arduino String instead of std::string
  serviceData += (char)26; // Length of beacon data
  serviceData += (char)0xFF; // Manufacturer specific data
  serviceData += beacon.getData();
  advertisementData.addData(serviceData);

  // Optional: add local name
  scanResponseData.setName("RodynaBeacon");

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->setScanResponseData(scanResponseData);
  pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND); // Non-connectable beacon
  BLEDevice::startAdvertising();

  Serial.println("✅ Beacon started:");
  Serial.print("   UUID: "); Serial.println(BEACON_UUID);
  Serial.print("   Major: "); Serial.println(BEACON_MAJOR);
  Serial.print("   Minor: "); Serial.println(BEACON_MINOR);
}

void loop() {
  delay(1000);
}
