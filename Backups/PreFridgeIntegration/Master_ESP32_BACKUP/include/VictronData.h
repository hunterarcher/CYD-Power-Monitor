#pragma once

#include <Arduino.h>

/**
 * Data structures matching Victron ESP32 sender
 * These will be sent via ESP-NOW
 */

// BMV-712 Battery Monitor Data
struct BMVData {
    float voltage;
    float current;
    float soc;              // State of charge %
    float consumedAh;
    float auxVoltage;
    uint16_t timeToGo;      // Minutes
    bool hasLowVoltageAlarm;
    bool hasHighVoltageAlarm;
    bool hasLowSOCAlarm;
    unsigned long timestamp;
    bool valid;
};

// SmartSolar MPPT Data
struct MPPTData {
    float batteryVoltage;
    float batteryCurrent;
    float solarPower;
    float yieldToday;       // kWh
    uint8_t state;          // 0=Off, 3=Bulk, 4=Absorption, 5=Float
    uint8_t error;
    unsigned long timestamp;
    bool valid;
};

// Blue Smart IP22 AC Charger Data
struct IP22Data {
    float batteryVoltage;
    float batteryCurrent;
    float temperature;
    float loadCurrent;      // AC input current
    float power;            // Calculated: voltage * current
    uint8_t state;          // 0=Off, 3=Bulk, 4=Absorption, 5=Float
    uint8_t error;
    unsigned long timestamp;
    bool valid;
};

// EcoFlow Delta 2 Max Data
struct EcoFlowData {
    char serialNumber[32];      // Device serial number
    uint8_t batteryPercent;     // Battery level 0-100%
    char macAddress[18];        // MAC address
    int rssi;                   // Signal strength
    unsigned long timestamp;    // Last update time
    bool valid;                 // Data is valid
};

// Combined packet sent via ESP-NOW
struct VictronPacket {
    BMVData bmv;
    MPPTData mppt;
    IP22Data ip22;
    EcoFlowData ecoflow;
    uint32_t packetId;      // Incremental packet counter
    unsigned long senderTime;
};

// State name helper
inline String getStateName(uint8_t state) {
    switch(state) {
        case 0: return "Off";
        case 1: return "Low Power";
        case 2: return "Fault";
        case 3: return "Bulk";
        case 4: return "Absorption";
        case 5: return "Float";
        case 6: return "Storage";
        case 7: return "Equalize";
        default: return "Unknown";
    }
}
