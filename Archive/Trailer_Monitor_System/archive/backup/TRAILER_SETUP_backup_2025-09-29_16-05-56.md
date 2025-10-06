# Trailer Victron Monitor - ESPHome Setup

This project uses the proven Victron BLE ESPHome component to monitor your trailer's electrical system standalone.

## Your Specific Configuration

**Devices Detected:**
- ✅ SHUNT Device: `c0:3b:98:39:e6:fe` (Battery Monitor)  
- ✅ SOLAR Device: `e8:86:01:5d:79:38` (Solar Charger)
- ❌ AC Charger: Not yet detected (will add when found)

**WiFi:** Already configured for "Rocket" network

## Quick Start

### 1. Current ESP32 Setup (No Display)

Use `trailer_monitor.yaml` for your current ESP32:

```bash
# Validate configuration
python -m esphome config trailer_monitor.yaml

# Compile and upload
python -m esphome run trailer_monitor.yaml

# Monitor logs
python -m esphome logs trailer_monitor.yaml
```

### 2. Future CYD Display Setup

When you get your CYD (Cheap Yellow Display), use `trailer_cyd.yaml`:
- Uncomment the display section 
- Flash to a new ESP32 with CYD attached
- Get beautiful on-screen dashboard

## What You'll See

### Web Interface
- Go to `http://[ESP32_IP]` 
- See all sensor readings in real-time
- Control restart and settings

### Expected Readings
Based on your Victron Connect screenshots:
- **Battery Voltage**: ~13.51V
- **Battery Current**: ~0.30A  
- **Solar Voltage**: ~13.50V
- **Solar Current**: 0.00A (when no sun)
- **State of Charge**: Percentage
- **Solar Power**: Watts generated

### Serial Logs
Every 30 seconds you'll see:
```
Battery: 13.51V/0.30A (85%) | Solar: 0W/0.00A | State: Float
```

## Advantages Over Arduino Version

✅ **Better Device Detection**: Uses official Victron BLE component  
✅ **Real Data Parsing**: Gets actual voltage/current values  
✅ **Web Interface**: Built-in monitoring dashboard  
✅ **OTA Updates**: Update firmware over WiFi  
✅ **Proven Reliable**: Based on tested CYD configuration  
✅ **Display Ready**: Easy path to add CYD display later

## Files Created

- `secrets.yaml` - Your WiFi and device credentials
- `trailer_monitor.yaml` - Current ESP32 configuration  
- `trailer_cyd.yaml` - Future display configuration
- `TRAILER_SETUP.md` - This guide

## Troubleshooting

**No Data from Devices:**
- Ensure VictronConnect app is closed (blocks BLE advertising)
- Check MAC addresses and bindkeys are correct
- Verify "Instant readout via Bluetooth" is enabled on devices

**AC Charger Missing:**
- Some AC chargers don't support instant readout
- Check VictronConnect → Settings → Product info → Instant readout
- If available, get MAC/bindkey and add to configuration

**WiFi Issues:**
- Device creates "Trailer-Monitor-Setup" hotspot if WiFi fails
- Connect to configure new credentials

## Next Steps

1. **Test Current Setup**: Flash `trailer_monitor.yaml` and verify data
2. **Order CYD Display**: Get WT32-SC01 or similar ESP32+ILI9488 combo
3. **Add AC Charger**: When detected, uncomment AC device section
4. **Display Dashboard**: Flash `trailer_cyd.yaml` to CYD for visual monitoring

This gives you a robust, standalone trailer monitoring system that can grow with your needs!