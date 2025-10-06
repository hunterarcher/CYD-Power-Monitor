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

// Fridge Data (Flex Adventure Camping Fridge)
struct FridgeData {
    // Temperature readings
    int8_t left_actual;         // LEFT actual temperature (°C)
    int8_t left_setpoint;       // LEFT setpoint (°C)
    int8_t right_actual;        // RIGHT actual temperature (°C)
    int8_t right_setpoint;      // RIGHT setpoint (°C)

    // Settings
    bool eco_mode;              // ECO mode enabled
    uint8_t battery_protection; // 0=L(8.5V), 1=M(10.1V), 2=H(11.1V)
    bool lock;                  // Lock enabled
    bool celsius;               // true=Celsius, false=Fahrenheit

    // Status
    bool connected;             // BLE connected
    int rssi;                   // Signal strength
    unsigned long last_seen;    // Last update time
    bool valid;                 // Data is valid
};

// Command packet (Master → Victron)
struct ControlCommand {
    uint32_t commandId;         // Unique command ID for tracking
    uint8_t device;             // Target device (1=Fridge)
    uint8_t command;            // Command type
    int16_t value1;             // Parameter 1
    int16_t value2;             // Parameter 2
    unsigned long timestamp;    // When command was sent
};

// ACK packet (Victron → Master)
struct CommandAck {
    uint32_t commandId;         // Which command this ACKs
    bool received;              // Command received and queued
    bool executed;              // Command executed successfully
    uint8_t errorCode;          // 0=success, other=error
    unsigned long timestamp;
};

// Status packet (Victron → Master) - signals when ready/scanning
struct StatusMessage {
    uint8_t type;               // 0=SCANNING, 1=READY
    unsigned long timestamp;
};

#define STATUS_SCANNING 0
#define STATUS_READY 1

// Combined packet sent via ESP-NOW (Victron → Master)
struct VictronPacket {
    BMVData bmv;
    MPPTData mppt;
    IP22Data ip22;
    EcoFlowData ecoflow;
    FridgeData fridge;
    uint32_t packetId;          // Incremental packet counter
    unsigned long senderTime;
    bool readyForCommand;       // True when Victron can receive commands
};

// Fridge control commands
#define CMD_FRIDGE_SET_LEFT_TEMP 1
#define CMD_FRIDGE_SET_RIGHT_TEMP 2
#define CMD_FRIDGE_SET_ECO 3
#define CMD_FRIDGE_SET_BATTERY 4

// Legacy alias (kept for compatibility)
#define CMD_FRIDGE_SET_TEMP 1

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
