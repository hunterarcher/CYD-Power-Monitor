# Victron ESPHome Setup Guide

## Quick Start

1. **Update WiFi Settings**
   Edit either `victron_test.yaml` or `victron_monitor.yaml` and replace:
   ```yaml
   wifi:
     ssid: "YOUR_WIFI_SSID"      # Replace with your WiFi network name
     password: "YOUR_WIFI_PASSWORD"  # Replace with your WiFi password
   ```

2. **Compile and Upload**
   ```powershell
   # Validate configuration
   python -m esphome config victron_test.yaml
   
   # Compile
   python -m esphome compile victron_test.yaml
   
   # Upload to ESP32 (make sure ESP32 is connected via USB)
   python -m esphome upload victron_test.yaml
   
   # Monitor logs
   python -m esphome logs victron_test.yaml
   ```

## What This Does

- **ESPHome Victron Monitor**: Uses official ESPHome victron_ble component
- **Your Devices**: Pre-configured with your SHUNT and SOLAR device MAC addresses and encryption keys
- **Standalone Operation**: Creates web server at http://[ESP32_IP] 
- **Fallback WiFi**: Creates "Victron-Test" hotspot if WiFi fails (password: 12345678)
- **Automatic Monitoring**: Logs readings every 30 seconds

## Expected Results

If working correctly, you should see logs like:
```
[INFO] Battery: 13.51V | Solar: 10.70V
[INFO] Battery: 13.52V | Solar: 10.69V
```

## Next Steps

1. **Test Basic Functionality**: Use `victron_test.yaml` first
2. **Full Configuration**: Switch to `victron_monitor.yaml` for all sensors
3. **Add AC Charger**: When detected, uncomment the AC device section
4. **Display Integration**: Add CYD display later

## Troubleshooting

- **WiFi Issues**: Device creates "Victron-Test" hotspot - connect and configure
- **No BLE Data**: Check device MAC addresses and encryption keys
- **Compilation Errors**: Ensure WiFi credentials are set correctly

## Files

- `victron_test.yaml` - Simple test configuration
- `victron_monitor.yaml` - Full configuration with all sensors  
- `ESPHOME_README.md` - Detailed documentation