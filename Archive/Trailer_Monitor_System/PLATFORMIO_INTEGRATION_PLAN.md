# Victron Instant Readout - Standalone ESPHome Integration
*Created: 2025-09-29 - Based on proven HACS integration method*

## 🎯 **PROJECT INTEGRATION PLAN**

### **Back to Your Original PlatformIO Structure:**
```
Trailer_Monitor_System/
├── src/
│   ├── main.cpp                    ← Your existing main
│   ├── config.h                    ← Your existing config  
│   ├── victron_ble.cpp/.h         ← Your existing (failed bindkey approach)
│   ├── victron_instant.cpp/.h     ← NEW: Working instant readout
│   └── [other components]          ← Room for expansion
├── platformio.ini                  ← Your existing PlatformIO config
└── [ESPHome configs]              ← Keep as reference/backup
```

## 🚀 **IMPLEMENTATION STRATEGY**

### **Option A: Pure PlatformIO C++ Implementation** (RECOMMENDED)
- Port the `victron_ble` library logic to C++
- Integrate with your existing `src/` structure
- Add BLE scanning for manufacturer ID 0x02E1
- Parse Instant Readout advertisements directly
- Create native ESP32 sensors

### **Option B: ESPHome Custom Component**
- Create ESPHome custom component
- Use external library support
- Keep compatibility with ESPHome ecosystem
- Easy web interface integration

### **Option C: Hybrid Approach**
- Start with PlatformIO base (your existing structure)
- Add ESPHome-style YAML for configuration
- Best of both worlds

## 📊 **TECHNICAL IMPLEMENTATION**

### **Core BLE Parsing Logic** (From HACS Integration):
```cpp
// victron_instant.cpp - Based on working HACS method

class VictronInstantReadout {
private:
    // Victron manufacturer ID and data format
    static constexpr uint16_t VICTRON_MFR_ID = 0x02E1;
    static constexpr uint8_t INSTANT_READOUT_MARKER = 0x10;
    
    // Your confirmed device MACs
    std::vector<std::string> device_macs = {
        "C0:3B:98:39:E6:FE",  // BMV-712 Smart (Battery Monitor)
        "E8:86:01:5D:79:38",  // SmartSolar MPPT (Solar Controller)  
        "C7:A2:C2:61:9F:C4"   // Blue Smart IP22 (AC Charger)
    };

public:
    void setup() {
        // Initialize BLE scanning for Victron devices
        BLEDevice::init("Trailer Monitor");
        scan = BLEDevice::getScan();
        scan->setAdvertisedDeviceCallbacks(new VictronScanCallbacks());
        scan->setActiveScan(true);
        scan->start(0); // Continuous scan
    }
    
    void parseVictronData(BLEAdvertisedDevice* device) {
        // Check for Victron manufacturer data
        if (device->haveManufacturerData()) {
            std::string mfr_data = device->getManufacturerData();
            uint16_t mfr_id = *(uint16_t*)mfr_data.data();
            
            if (mfr_id == VICTRON_MFR_ID && mfr_data[2] == INSTANT_READOUT_MARKER) {
                // Parse based on device type (BMV-712, MPPT, IP22)
                parseDeviceSpecificData(device->getAddress().toString(), mfr_data);
            }
        }
    }
};
```

### **Device-Specific Parsers:**
```cpp
void parseDeviceSpecificData(std::string mac_address, std::string data) {
    if (mac_address == "C0:3B:98:39:E6:FE") {
        // BMV-712 Battery Monitor
        parseBatteryMonitor(data);
    } 
    else if (mac_address == "E8:86:01:5D:79:38") {
        // SmartSolar MPPT  
        parseSolarController(data);
    }
    else if (mac_address == "C7:A2:C2:61:9F:C4") {
        // Blue Smart IP22 AC Charger
        parseACCharger(data);
    }
}

void parseBatteryMonitor(std::string data) {
    // Extract: voltage, current, SOC, power, consumed_energy
    // Based on BatteryMonitorData parsing logic
    battery_voltage = extractVoltage(data);
    battery_current = extractCurrent(data);  
    battery_soc = extractSOC(data);
    // Update your existing sensor variables
}
```

## 🎯 **INTEGRATION WITH YOUR EXISTING PROJECT**

### **Step 1: Update `config.h`**
```cpp
// config.h - Add Victron configuration
#ifndef CONFIG_H
#define CONFIG_H

// Your existing WiFi config
#define WIFI_SSID "Rocket"
#define WIFI_PASSWORD "Ed1nburgh2015!"

// NEW: Victron Device Configuration
#define VICTRON_BMV712_MAC "C0:3B:98:39:E6:FE"
#define VICTRON_MPPT_MAC "E8:86:01:5D:79:38" 
#define VICTRON_IP22_MAC "C7:A2:C2:61:9F:C4"

// Sensor update intervals
#define VICTRON_UPDATE_INTERVAL 30000  // 30 seconds
#define BLE_SCAN_TIME 10  // 10 seconds

// Your existing sensor configs
// ... other defines

#endif
```

### **Step 2: Update `main.cpp`**
```cpp
// main.cpp - Integrate with your existing structure
#include "config.h"
#include "victron_instant.h"  // NEW
// #include "victron_ble.h"   // OLD - remove or comment out

VictronInstantReadout victron;  // NEW

void setup() {
    Serial.begin(115200);
    
    // Your existing WiFi setup
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // NEW: Initialize Victron Instant Readout
    victron.setup();
    
    // Your existing sensor initializations
    // ... other setup code
}

void loop() {
    // Your existing loop code
    
    // NEW: Process Victron data (non-blocking)
    victron.update();
    
    // Your existing sensor readings and web server
    // ... other loop code
    
    delay(1000);
}
```

### **Step 3: Web Interface Integration**
```cpp
// Add to your existing web server in main.cpp
void handleVictronData() {
    String json = "{";
    json += "\"battery_voltage\":" + String(victron.getBatteryVoltage()) + ",";
    json += "\"battery_current\":" + String(victron.getBatteryCurrent()) + ","; 
    json += "\"battery_soc\":" + String(victron.getBatterySOC()) + ",";
    json += "\"solar_power\":" + String(victron.getSolarPower()) + ",";
    json += "\"ac_output_power\":" + String(victron.getACOutputPower());
    json += "}";
    
    server.send(200, "application/json", json);
}
```

## ✅ **ADVANTAGES OF THIS APPROACH**

1. **Builds on Your Working Foundation** ✅
2. **Uses Proven BLE Communication** ✅  
3. **No Memory Crashes** (lightweight C++ vs heavy ESPHome)
4. **Room for Expansion** (other trailer sensors)
5. **Native Performance** (direct ESP32 code)
6. **Keep Your Existing Structure** ✅

## 🚀 **READY TO IMPLEMENT?**

I can create the complete integration files:
- `victron_instant.h` - Header with class definition
- `victron_instant.cpp` - Implementation with BLE parsing
- Updated `main.cpp` - Integration with your existing code  
- Updated `config.h` - Device configuration

This will give you a **rock-solid, standalone Victron monitoring system** integrated into your existing PlatformIO project!

**Should I create these files now?**

---
**Status**: Ready to implement proven Victron Instant Readout in your existing PlatformIO structure ✅