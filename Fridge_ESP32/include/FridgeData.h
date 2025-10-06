#pragma once

#include <Arduino.h>

/**
 * Flex Adventure Fridge Data Structures
 * Phase 1: Passive scanning only - no connection
 */

// Known BLE identifiers
#define FRIDGE_TARGET_MAC "ff:ff:11:c6:29:50"
#define FRIDGE_SERVICE_UUID "00001234-0000-1000-8000-00805f9b34fb"

// Fridge data structure
struct FridgeData {
    // Temperature (Phase 3+)
    int8_t left_setpoint;       // °C
    int8_t right_setpoint;      // °C
    int8_t left_actual;         // °C
    int8_t right_actual;        // °C

    // Status (Phase 3+)
    bool connected;
    bool battery_mode;          // vs AC mode
    bool eco_mode;
    bool compressor_running;
    uint8_t error_code;

    // Raw decoded values from status frame
    uint8_t last_zone;          // Zone/compartment from last status (0x01=left, 0x02=right)
    uint8_t status_byte1;       // Status flags byte 1
    uint8_t status_byte2;       // Status flags byte 2
    uint8_t raw_setpoint;       // Raw setpoint value
    uint8_t raw_temp1;          // Raw temperature field 1
    uint8_t raw_temp2;          // Raw temperature field 2
    uint8_t battery_percent;    // Battery percentage (if available)

    // Metadata (Phase 1+)
    unsigned long last_seen;
    unsigned long last_status_frame;  // When we last decoded a status frame
    int rssi;
    bool detected;              // Found via BLE scan
    String device_name;
    String mac_address;
};

// Phase tracking
enum FridgePhase {
    PHASE_1_PASSIVE_SCAN = 1,   // Current: Advertisement monitoring only
    PHASE_2_PASSIVE_CONN = 2,   // Future: Connect + observe only
    PHASE_3_KEEPALIVE = 3,      // Future: Keep-alive only
    PHASE_4_CONTROL = 4         // Future: Full control
};

// BLE Characteristic UUIDs (for Phase 2+)
#define FRIDGE_NOTIFY_CHAR_UUID "00001236-0000-1000-8000-00805f9b34fb"
#define FRIDGE_WRITE_CHAR_UUID  "00001235-0000-1000-8000-00805f9b34fb"

// Connection state tracking
enum ConnectionState {
    STATE_DISCONNECTED,
    STATE_SCANNING,
    STATE_CONNECTING,
    STATE_DISCOVERING_SERVICES,
    STATE_GETTING_CHARACTERISTICS,
    STATE_ENABLING_NOTIFICATIONS,
    STATE_CONNECTED_OBSERVING,
    STATE_ERROR
};

// Keep-alive frame format
#define KEEPALIVE_FRAME_SIZE 6
const uint8_t KEEPALIVE_FRAME[KEEPALIVE_FRAME_SIZE] = {0xFE, 0xFE, 0x03, 0x01, 0x02, 0x00};

// Current implementation phase
#define CURRENT_PHASE PHASE_3_KEEPALIVE  // Phase 2.5 (jumping to 3 for keep-alive)
