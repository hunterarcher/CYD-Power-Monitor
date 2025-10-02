# Victron BLE Monitor - ESPHome Setup

This project uses ESPHome to monitor Victron BLE devices (Battery Monitor/Shunt and Solar Charger) on an ESP32.

## Prerequisites

1. **Install ESPHome**:
   ```bash
   pip install esphome
   ```

2. **Hardware**:
   - ESP32 development board (like your MH-ET LIVE ESP32 DevKit)
   - USB cable for programming

## Configuration

1. **Update WiFi Settings** in `victron_monitor.yaml`:
   - Replace `YOUR_WIFI_SSID` with your WiFi network name
   - Replace `YOUR_WIFI_PASSWORD` with your WiFi password

2. **Update Encryption Keys**:
   - Replace `your_api_encryption_key_here` with a 32-character random string
   - Replace `your_ota_password_here` with a secure password

3. **Device MAC Addresses** (already configured):
   - SHUNT: `c0:3b:98:39:e6:fe`
   - SOLAR: `e8:86:01:5d:79:38`
   - AC Charger: Not yet detected (commented out in config)

## Build and Flash

1. **Validate Configuration**:
   ```bash
   esphome config victron_monitor.yaml
   ```

2. **Compile**:
   ```bash
   esphome compile victron_monitor.yaml
   ```

3. **Upload to ESP32** (first time via USB):
   ```bash
   esphome upload victron_monitor.yaml
   ```

4. **Monitor Logs**:
   ```bash
   esphome logs victron_monitor.yaml
   ```

## Features

### Current Sensors
- **Battery Monitor (SHUNT)**:
  - Battery Voltage
  - Battery Current  
  - Battery Power
  - Consumed Ah
  - State of Charge
  - Time to Go

- **Solar Charger**:
  - Solar Voltage
  - Solar Current
  - Solar PV Power
  - Solar Yield Today

### Standalone Operation
- **Web Interface**: Access at `http://[ESP32_IP]` (username: admin, password: admin)
- **Serial Logging**: Periodic status updates every 30 seconds
- **No Home Assistant Required**: Runs completely standalone

### Future Display Integration
The configuration includes commented sections for:
- ILI9341 display support (for CYD - Cheap Yellow Display)
- Font configuration
- Display layout with all sensor data

## Troubleshooting

1. **AC Charger Detection**: Currently not detected. The configuration includes a commented section for when the AC charger is found.

2. **WiFi Issues**: If WiFi fails, the device creates a fallback hotspot "Victron-Monitor Fallback Hotspot" (password: 12345678).

3. **BLE Issues**: ESPHome's victron_ble component should provide better device detection than the Arduino version.

## Next Steps

1. Flash the configuration and test BLE detection
2. Add AC charger configuration once detected
3. Integrate display for standalone monitoring
4. Optimize for your specific use case

## Advantages over Arduino Version

- **Official Component**: Uses ESPHome's official victron_ble component
- **Better Device Detection**: More robust BLE handling
- **Web Interface**: Built-in web server for monitoring
- **OTA Updates**: Update firmware over WiFi
- **Easier Configuration**: YAML-based configuration vs C++ code