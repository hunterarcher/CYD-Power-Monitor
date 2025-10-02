# Victron ESP-NOW Distributed Monitor System

A distributed ESP32-based monitoring system for Victron Energy devices using ESP-NOW communication.

## Architecture Overview

This system solves memory and complexity limitations of single-ESP32 approaches by using a distributed architecture:

- **Victron Sensor ESP32**: Dedicated BLE scanner for Victron devices with ESP-NOW transmission
- **Main Display ESP32**: ESP-NOW receiver with OLED display and web interface

## Features

- 🔋 **Multi-Device Support**: BMV-712, MPPT, IP22 Charger
- 📡 **ESP-NOW Communication**: Low-latency, reliable device-to-device communication
- 📱 **Web Interface**: Real-time monitoring via built-in web server
- 🖥️ **OLED Display**: Local status display on main unit
- ⚡ **Memory Optimized**: Each ESP32 handles specific tasks efficiently
- 🔄 **Auto-Recovery**: Automatic reconnection and data validation

## Hardware Requirements

### Victron Sensor ESP32
- ESP32 development board
- Must support BLE (built-into ESP32)
- Power supply (USB or external)

### Main Display ESP32  
- ESP32 development board
- SSD1306 OLED display (128x64, I2C)
- WiFi connectivity
- Power supply (USB or external)

## Project Structure

```
Trailer_Monitor_System/
├── victron_sensor/          # BLE scanner ESP32 project
│   ├── platformio.ini       # Optimized for BLE + ESP-NOW
│   ├── include/
│   │   └── victron_ble.h    # BLE scanning and ESP-NOW transmission
│   └── src/
│       └── main.cpp         # Main sensor logic
├── main_display/            # Display ESP32 project
│   ├── platformio.ini       # Optimized for WiFi + Display + ESP-NOW
│   ├── include/
│   │   └── victron_display.h # Display and web interface
│   └── src/
│       └── main.cpp         # Main display logic
└── archive/                 # Previous single-ESP32 attempts (reference)
```

## Setup Instructions

### 1. Configure Device MAC Addresses

Both ESP32s need to know each other's MAC addresses for ESP-NOW communication.

**Find your ESP32 MAC addresses:**
```cpp
#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
}
```

**Update the MAC addresses in the code:**

In `victron_sensor/src/main.cpp`:
```cpp
uint8_t mainDisplayMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Replace with actual MAC
```

### 2. Configure Your Victron Device MAC Addresses

In `victron_sensor/include/victron_ble.h`, update the MAC addresses with your actual devices:
```cpp
#define BMV712_MAC "C0:3B:98:39:E6:FE"  // Your BMV-712 MAC
#define MPPT_MAC "E8:86:01:5D:79:38"    // Your MPPT MAC  
#define IP22_MAC "C7:A2:C2:61:9F:C4"    // Your IP22 MAC
```

### 3. Configure WiFi (Main Display Only)

In `main_display/src/main.cpp`:
```cpp
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";
```

### 4. Hardware Connections

**Main Display ESP32 + OLED:**
```
ESP32    SSD1306
-----    -------
3.3V  -> VCC
GND   -> GND
GPIO21 -> SDA
GPIO22 -> SCL
```

### 5. Build and Upload

**Victron Sensor ESP32:**
```bash
cd victron_sensor
pio run -t upload
pio device monitor
```

**Main Display ESP32:**
```bash
cd main_display  
pio run -t upload
pio device monitor
```

## Usage

### Serial Monitor Output

**Victron Sensor:**
```
=== Victron BLE Sensor ESP32 ===
✓ ESP-NOW initialized
✓ BLE scanning started
Found Victron device: C0:3B:98:39:E6:FE (RSSI: -45)
✓ Instant readout packet found
Parsed: 12.85V, -15.2A, -195.3W, 87% SOC
✓ Data sent successfully
```

**Main Display:**
```
=== Victron Main Display ESP32 ===
✓ WiFi connected! IP: 192.168.1.100
✓ ESP-NOW receiver initialized
✓ Web server started
Received data from device type 1: 12.85V, -15.20A
```

### Web Interface

Connect to the main display's IP address in your browser:
- **Local Network**: `http://192.168.1.100` (actual IP shown in serial)
- **AP Mode**: `http://192.168.4.1` (if WiFi fails)

### OLED Display

The main display shows real-time data on the connected OLED:
```
Victron Monitor
---------------
BMV: 12.8V -15.2A
SOC: 87% PWR:-195W

MPPT: 14.2V 8.5A  
Solar: 121W

IP22: 13.6V 5.2A
```

## Victron Device Data

The system monitors these Victron BLE advertisements:

### BMV-712 (Battery Monitor)
- Battery voltage
- Battery current (+ charging, - discharging)
- State of Charge (SOC %)
- Power calculation

### MPPT Solar Controller
- PV panel voltage
- PV current
- Solar power generation

### IP22 Charger
- Charger output voltage
- Charger current
- Charging power

## Troubleshooting

### No BLE Data Received
1. **Check MAC addresses** in `victron_ble.h` match your devices exactly
2. **BLE range**: Ensure sensor ESP32 is within ~10m of Victron devices
3. **Enable BLE** on Victron devices (usually automatic with VictronConnect app usage)
4. **Monitor serial output** for BLE scan results

### ESP-NOW Communication Issues
1. **Verify MAC addresses** between both ESP32s are correct
2. **Check power supply** - unstable power can cause communication failures
3. **WiFi channel conflicts** - ESP-NOW uses same channels as WiFi
4. **Distance**: Keep ESP32s within ~100m line-of-sight

### Memory Issues
1. **Use provided platformio.ini** files - they're optimized for each ESP32's role
2. **Monitor serial output** for memory warnings during compilation
3. **Reduce library dependencies** if adding features

### Display Issues
1. **Check I2C wiring** (SDA/SCL connections)
2. **Verify OLED address** (usually 0x3C or 0x3D)
3. **Power supply**: OLED requires stable 3.3V

## Memory Usage

Both ESP32 projects are optimized for their specific roles:

**Victron Sensor**: ~50% flash usage (BLE + ESP-NOW only)
**Main Display**: ~75% flash usage (WiFi + Web server + Display + ESP-NOW)

## Supported Victron Devices

This system uses the Victron Instant Readout BLE protocol. Confirmed working devices:
- BMV-712 Smart Battery Monitor
- SmartSolar MPPT controllers  
- Phoenix IP22 Smart Chargers
- Most Victron devices with BLE capability

## Development Notes

### Based on Working ESPHome/HACS Integration

This implementation is designed to replicate the functionality of your working Home Assistant setup:
- Uses same BLE scanning approach as ESPHome Bluetooth Proxy
- Parses Victron Instant Readout protocol like HACS integration
- Maintains the same data accuracy and reliability

### Why ESP-NOW vs WiFi?

ESP-NOW provides several advantages for this application:
- **Lower latency**: Direct device communication without router/network stack
- **More reliable**: No dependency on WiFi network stability  
- **Lower power**: More efficient than full TCP/IP stack
- **Simpler setup**: No network configuration needed between devices

### Future Enhancements

- Battery-powered sensor nodes with deep sleep
- Multiple sensor ESP32s for larger installations
- MQTT integration for Home Assistant
- Historical data logging
- Solar production forecasting
- Alert system for low battery/charging issues

## License

This project is provided as-is for educational and personal use.

## Contributing

Feel free to submit issues and enhancement requests!