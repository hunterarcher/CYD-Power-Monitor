#pragma once

#include <Arduino.h>

/**
 * EcoFlow Delta 2 Max BLE Beacon Data
 * Data extracted from BLE advertisement beacons (0xB5B5 manufacturer data)
 */

struct EcoFlowData {
    char serialNumber[32];      // Device serial number
    uint8_t batteryPercent;     // Battery level 0-100%
    char cpuId[32];             // CPU ID
    int rssi;                   // Signal strength
    unsigned long timestamp;    // Last update time
    bool valid;                 // Data is valid
};

// ESP-NOW packet to send to Master ESP32
struct EcoFlowPacket {
    EcoFlowData ecoflow;
    uint32_t packetId;
    unsigned long senderTime;
};
