# Trailer Monitor System - Project Context

## Project Overview
This is an ESP32-based comprehensive monitoring system for a trailer/RV that monitors:
1. **Victron battery/solar systems** via BLE
2. **EcoFlow power station** via BLE or MQTT
3. **Water tanks** (2x plastic tanks) using contactless sensors
4. **Gas bottle** level monitoring (future enhancement)
5. **Fridge/Freezer temperatures** (Flex Adventure fridge - in development)

## Hardware
- **Development Board:** MH-ET LIVE ESP32 DevKit with CP2102 USB chip
- **Target Display Device:** BDD ESP32 LVGL 2.8" LCD (CYD equivalent) - future migration
- **Final Target:** CYD (Cheap Yellow Display) with potential for larger display later

## Project Structure
```
Trailer_Monitor_System/
├── src/
│   ├── main.cpp              # Main entry point & loop
│   ├── config.h              # Module enable/disable flags & settings
│   ├── victron_ble.h         # Victron BLE header
│   ├── victron_ble.cpp       # Victron BLE implementation
│   ├── ecoflow_ble.h         # (Future) EcoFlow module header
│   ├── ecoflow_ble.cpp       # (Future) EcoFlow module implementation
│   ├── tank_monitor.h        # (Future) Water tank monitoring header
│   ├── tank_monitor.cpp      # (Future) Water tank monitoring implementation
│   ├── gas_monitor.h         # (Future) Gas bottle monitoring header
│   ├── gas_monitor.cpp       # (Future) Gas bottle monitoring implementation
│   ├── fridge_monitor.h      # (Future) Fridge/freezer monitoring header
│   └── fridge_monitor.cpp    # (Future) Fridge/freezer monitoring implementation
├── platformio.ini            # PlatformIO configuration
├── PROJECT_CONTEXT.md        # This file
└── README.md                 # Quick start guide
```

## Current Development Status
- ✅ Victron BLE module: In testing phase
- ⏳ EcoFlow module: Planning phase (BLE preferred over MQTT)
- ⏳ Fridge module: Reverse engineering phase (BLE protocol)
- ⏳ Tank monitoring: Planning phase (XKC-Y23A-V contactless sensors)
- ⏳ Gas monitoring: Planning phase (ultrasonic/magnetic sensors for metal cylinder)


## Development Philosophy
- **Modular design:** Each sensor/device has its own .h/.cpp files
- **Enable/disable via flags:** Use config.h to turn modules on/off during testing
- **One module at a time:** Test each module independently before integration
- **No breaking changes:** New modules shouldn't break existing working code
- **Graceful degradation:** System continues working even if one module fails

## Module Enable Flags (config.h)
```cpp
#define ENABLE_VICTRON_BLE 1  // Currently testing
#define ENABLE_ECOFLOW 0      // Not yet implemented
#define ENABLE_TANK_MONITOR 0 // Not yet implemented
#define ENABLE_GAS_MONITOR 0  // Not yet implemented
#define ENABLE_FRIDGE 0       // In development
```

## Hardware Components

### Power Monitoring
- **Victron devices:** Battery monitors, MPPT solar chargers (BLE connection)
- **EcoFlow power station:** Battery level, AC/DC status, power consumption (BLE/MQTT)

### Tank Monitoring
- **Sensor type:** XKC-Y23A-V Mini Liquid Level Sensors
- **Configuration:** 2 sensors per tank (50% and 10% levels)
- **Tank count:** 2 plastic water tanks (long and thin design)
- **Features:** Non-contact detection, IP67 rated, ±3mm accuracy
- **Future enhancement:** Inline flow sensors to detect active tank

### Gas Monitoring
- **Bottle type:** Metal gas cylinder (underneath trailer mount)
- **Sensor options:** 
  - Ultrasonic sensors mounted underneath
  - Magnetic mount level sensors
  - Weight-based sensors
- **Status:** Future enhancement

### Fridge Monitoring
- **Fridge model:** Flex Adventure fridge/freezer
- **Method:** BLE reverse engineering

## Build System
- **IDE:** VS Code with PlatformIO
- **Framework:** Arduino
- **Platform:** espressif32
- **Board:** mhetesp32devkit
- **Monitor Speed:** 115200 baud

## Key Libraries
- **ESP32 BLE Arduino** - for Bluetooth connectivity (Victron, EcoFlow, Fridge)
- **OneWire + DallasTemperature** - for DS18B20 temperature sensors (future)
- **WiFi + PubSubClient** - for MQTT if needed (EcoFlow fallback option)

## Communication Protocols

### Victron (BLE)
- **Protocol:** Text-based key=value format over BLE
- **Service UUID:** 6e400001-b5a3-f393-e0a9-e50e24dcca9e
- **Characteristic UUID:** 6e400003-b5a3-f393-e0a9-e50e24dcca9e
- **Data format:** \r\nKEY\tVALUE\r\n (e.g., V\t12847 for 12.847V)

### EcoFlow (BLE preferred, MQTT fallback)
- **BLE Protocol:** Requires reverse engineering
- **MQTT Option:** Requires WiFi network, port 1883
- **Status:** To be determined based on BLE feasibility

### Tank Sensors (GPIO)
- **Type:** Digital sensors (HIGH/LOW)
- **Connection:** Direct to ESP32 GPIO pins
- **Reading:** Simple digitalRead() for level detection

### Gas Sensor (Analog/Digital - TBD)
- **Type:** To be determined based on sensor choice
- **Connection:** Analog or digital GPIO pins
- **Reading:** Based on specific sensor protocol

### Fridge (BLE + Temperature Sensors)
- **BLE Protocol:** Requires reverse engineering with nRF Connect
- **Status:** In development

## Development Workflow
1. Work on one module at a time in isolation
2. Test thoroughly before enabling next module
3. Keep existing modules working while adding new ones
4. Use Serial Monitor for debugging (115200 baud)
5. Document any reverse-engineered protocols

## Testing Approach
- **Phase 1:** Test on MH-ET LIVE ESP32 DevKit
- **Phase 2:** Migrate to BDD ESP32 LVGL 2.8" display with touch interface
- **Phase 3:** Final deployment in trailer with proper mounting
- Each module should handle connection failures gracefully
- Auto-reconnect logic for all BLE devices (30-second retry interval)

## Display Strategy
- **Initial:** Serial Monitor only (current phase)
- **Development:** BDD ESP32 LVGL 2.8" LCD with touch GUI
- **Features:** Visual indicators for all sensors, touch controls, alerts
- **Portable option:** Consider battery-powered portable display + fixed sensor ESP32

## Power Requirements
- **Development:** USB powered
- **Deployment:** 12V trailer system with buck converter
- **Display consideration:** Low-power sleep modes when not in use
- **Target:** <200mA total system consumption

## Network Architecture
- **Local WiFi option:** CYD as WiFi AP ("TrailerNet") for EcoFlow MQTT
- **No internet required:** Pure local communication
- **Future:** Possible Raspberry Pi + Home Assistant integration

## Future Enhancements
- Add WiFi for data logging
- Add MQTT for remote monitoring
- Add web dashboard interface
- Integrate Home Assistant for advanced automation
- Add weather data integration (requires internet)
- Automatic tank switching with solenoid valves
- Flow rate monitoring for leak detection

## Important Technical Notes
- Never use localStorage/sessionStorage (not supported in ESP32)
- Use in-memory state management only (variables, structs)
- All BLE connections must handle disconnects gracefully
- Use FreeRTOS tasks for parallel processing when needed
- Keep code modular for easy display migration
- Document all reverse-engineered protocols thoroughly

## Cost Considerations
Current system cost estimate (South African Rand):
- BDD ESP32 LVGL 2.8" display: ~R280
- Tank sensors (4x XKC-Y23A-V): R414
- Fridge sensors (2x DS18B20): R50
- Gas sensor: ~R100-200 (TBD)
- Wiring & installation: R400
- **Total:** ~R1,350-1,450

## Development Tools
- **nRF52840 BLE dongle:** For BLE protocol reverse engineering (~R450)
- **nRF Connect app:** Mobile app for BLE scanning and packet capture
- **VS Code:** Primary development environment
- **PlatformIO:** Build system and library management
- **Serial Monitor:** Primary debugging interface