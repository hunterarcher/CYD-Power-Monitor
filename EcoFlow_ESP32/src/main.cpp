#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertisedDevice.h>
#include <esp_now.h>
#include <WiFi.h>
#include "EcoFlowData.h"

// Master ESP32 MAC Address - CHANGE THIS to your Master ESP32 MAC!
uint8_t masterMAC[] = {0x7C, 0x87, 0xCE, 0x31, 0xFE, 0x50};

// EcoFlow data
EcoFlowData ecoflowData;
EcoFlowPacket packet;
uint32_t packetCounter = 0;

bool deviceFound = false;
BLEScan* pBLEScan;

// Parse EcoFlow beacon data
// Format: C5-C5-13 [DATA...]
bool parseEcoFlowBeacon(uint8_t* data, size_t length, EcoFlowData* output) {
    Serial.printf("\n[DEBUG] Parsing beacon, length: %d\n", length);
    Serial.print("[DEBUG] Raw data: ");
    for (int i = 0; i < length; i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();

    // Look for EcoFlow manufacturer ID: 0xC5C5
    // Format appears to be: C5 C5 13 [byte] [ASCII serial chars...]
    if (length >= 3 && data[0] == 0xC5 && data[1] == 0xC5 && data[2] == 0x13) {
        Serial.println("[DEBUG] ✓ Found EcoFlow Delta beacon (0xC5C5 0x13)");

        output->valid = true;
        output->timestamp = millis();

        // Extract serial number from ASCII bytes
        // Appears to be at positions 4 onwards
        if (length > 4) {
            int serialLen = min((int)(length - 4), 30);
            for (int i = 0; i < serialLen; i++) {
                output->serialNumber[i] = (char)data[4 + i];
            }
            output->serialNumber[serialLen] = '\0';
            Serial.printf("[DEBUG] Serial number: %s\n", output->serialNumber);
        }

        // Battery percentage at position 3
        // Encoding: raw_value - 43 = battery %
        // Example: 0x8D (141) - 43 = 98%
        uint8_t rawBattery = data[3];

        if (rawBattery >= 43) {
            output->batteryPercent = rawBattery - 43;
        } else {
            output->batteryPercent = 0;
        }

        Serial.printf("[DEBUG] Battery raw: 0x%02X (%d) = %d%%\n",
                     rawBattery, rawBattery, output->batteryPercent);

        return true;
    }

    return false;
}

// BLE Scan callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String mac = advertisedDevice.getAddress().toString().c_str();
        String name = advertisedDevice.getName().c_str();

        // Look for Espressif MAC (34:b4:xx:xx:xx:xx) or EcoFlow in name
        bool isEspressif = mac.substring(0, 5).equalsIgnoreCase("34:b4");
        bool hasEcoFlowName = name.indexOf("EF-") >= 0 || name.indexOf("R351") >= 0;

        if (isEspressif || hasEcoFlowName || advertisedDevice.haveManufacturerData()) {
            Serial.println("\n========================================");
            Serial.printf("Device: %s\n", name.c_str());
            Serial.printf("  MAC: %s\n", mac.c_str());
            Serial.printf("  RSSI: %d dBm\n", advertisedDevice.getRSSI());

            if (advertisedDevice.haveManufacturerData()) {
                String mfgData = advertisedDevice.getManufacturerData().c_str();
                Serial.printf("  Manufacturer Data Length: %d\n", mfgData.length());

                if (mfgData.length() > 0) {
                    Serial.print("  Raw Manufacturer Data: ");
                    for (int i = 0; i < mfgData.length(); i++) {
                        Serial.printf("%02X ", (uint8_t)mfgData[i]);
                    }
                    Serial.println();

                    // Try to parse EcoFlow beacon
                    const uint8_t* data = (const uint8_t*)mfgData.c_str();
                    if (parseEcoFlowBeacon((uint8_t*)data, mfgData.length(), &ecoflowData)) {
                        Serial.println("  ✓ EcoFlow beacon parsed!");
                        ecoflowData.rssi = advertisedDevice.getRSSI();
                        strncpy(ecoflowData.cpuId, mac.c_str(), sizeof(ecoflowData.cpuId) - 1);
                        deviceFound = true;
                    }
                }
            }

            // Also check payload data
            if (advertisedDevice.haveServiceData()) {
                Serial.println("  Has service data");
            }

            Serial.println("========================================");
        }
    }
};

// ESP-NOW send callback
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.printf("[ESP-NOW] Send status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "✓ Success" : "✗ Failed");
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("   EcoFlow Delta 2 Max BLE Scanner");
    Serial.println("========================================\n");

    // Initialize BLE
    BLEDevice::init("EcoFlow_Scanner");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    Serial.println("✓ BLE initialized\n");

    // Initialize WiFi for ESP-NOW
    WiFi.mode(WIFI_STA);
    Serial.printf("ESP32 MAC: %s\n\n", WiFi.macAddress().c_str());

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ ESP-NOW init failed!");
        return;
    }
    Serial.println("✓ ESP-NOW initialized");

    // Register send callback
    esp_now_register_send_cb(onDataSent);

    // Add Master ESP32 as peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("✗ Failed to add Master as peer!");
        return;
    }
    Serial.printf("✓ Added Master ESP32 as peer: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                 masterMAC[0], masterMAC[1], masterMAC[2], masterMAC[3], masterMAC[4], masterMAC[5]);

    Serial.println("Starting BLE scan for EcoFlow devices...\n");
    Serial.println("Tip: Looking for devices with MAC starting 34:B4:xx (Espressif)");
    Serial.println("     or devices with 0xB5B5 in manufacturer data\n");
}

void loop() {
    // Reset found flag
    deviceFound = false;
    memset(&ecoflowData, 0, sizeof(ecoflowData));

    // Scan for 10 seconds
    Serial.println("[BLE] Starting 10 second scan...");
    BLEScanResults* foundDevices = pBLEScan->start(10, false);
    Serial.printf("[BLE] Scan complete. Found %d devices total\n", foundDevices->getCount());
    pBLEScan->clearResults();

    // Print status
    Serial.println("\n========== ECOFLOW STATUS ==========");
    if (deviceFound && ecoflowData.valid) {
        Serial.println("Status: ONLINE ✓");
        Serial.printf("MAC/CPU ID:       %s\n", ecoflowData.cpuId);
        Serial.printf("Serial Number:    %s\n", ecoflowData.serialNumber);
        Serial.printf("Battery Level:    %d%%\n", ecoflowData.batteryPercent);
        Serial.printf("Signal Strength:  %d dBm\n", ecoflowData.rssi);
    } else {
        Serial.println("Status: OFFLINE ✗");
        Serial.println("No EcoFlow Delta 2 Max found");
    }
    Serial.println("====================================\n");

    // Prepare and send ESP-NOW packet
    memset(&packet, 0, sizeof(packet));
    memcpy(&packet.ecoflow, &ecoflowData, sizeof(EcoFlowData));
    packet.packetId = packetCounter++;
    packet.senderTime = millis();

    Serial.printf("[ESP-NOW] Sending packet #%d...\n", packet.packetId);
    esp_err_t result = esp_now_send(masterMAC, (uint8_t*)&packet, sizeof(packet));

    if (result == ESP_OK) {
        Serial.println("[ESP-NOW] Packet queued for transmission");
    } else {
        Serial.printf("[ESP-NOW] ✗ Send failed! Error: %d\n", result);
    }

    // Wait 30 seconds before next scan
    Serial.println("\nWaiting 30 seconds before next scan...\n");
    delay(30000);
}
