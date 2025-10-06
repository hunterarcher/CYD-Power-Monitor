# Trailer Monitor System

Comprehensive ESP32-based monitoring system for trailer power, water, gas, and appliances.

## Quick Start

### Hardware Required
- MH-ET LIVE ESP32 DevKit (CP2102)
- USB cable for programming
- Victron device with BLE (for initial testing)

### Setup Steps

1. **Open in VS Code with PlatformIO**
   - Install PlatformIO extension if not already installed
   - Open this project folder

2. **Connect ESP32**
   - Plug in your MH-ET LIVE ESP32 DevKit via USB
   - Check Device Manager to confirm COM port is detected

3. **Build Project**
   - Click checkmark icon (✓) in bottom status bar
   - Wait for "SUCCESS" message

4. **Upload to ESP32**
   - Click arrow icon (→) in bottom status bar
   - Wait for upload to complete

5. **Monitor Serial Output**
   - Click plug icon (🔌) in bottom status bar
   - Set baud rate to 115200
   - Watch for connection messages

## Current Modules

### ✅ Active Modules
- **Victron BLE:** Battery/solar monitoring via Bluetooth
  - Monitors voltage, current, power, state of charge
  - Auto-reconnect on disconnect
  - 5-second update interval

### ⏳ Planned Modules
- **EcoFlow:** Power station monitoring (BLE/MQTT)
- **Tank Monitor:** Water tank levels (XKC-Y23A-V sensors)
- **Gas Monitor:** Gas bottle level tracking
- **Fridge Monitor:** Flex Adventure fridge (BLE reverse engineering in progress)
  - Successfully decoded protocol via Wireshark/nRF Connect
  - Service: `00001234-0000-1000-8000-00805f9b34fb`
  - Keep-alive pattern: `FEFE03010200` (every 2 seconds)
  - Ready for ESP32 testing phase

## Configuration

Edit `src/config.h` to enable/disable modules:

```cpp
#define ENABLE_VICTRON_BLE 1  // Currently active
#define ENABLE_ECOFLOW 0      // Coming soon
#define ENABLE_TANK_MONITOR 0 // Coming soon
#define ENABLE_GAS_MONITOR 0  // Coming soon
#define ENABLE_FRIDGE 0       // In development
```

## Expected Serial Output

When working correctly, you should see:

```
=================================
Trailer Monitor System Starting
=================================

[Victron Module]
Initializing Victron BLE Monitor...
✓ Victron BLE initialized
Scanning for Victron devices...
Found Victron device: SmartShunt 500A, Address: XX:XX:XX:XX:XX:XX
Connected to device
✓ Ready to monitor Victron data

=================================
Setup complete - starting monitoring
=================================

=== Victron Data ===
Device: SmartShunt 500A/50mV
Connected: Yes
Battery Voltage: 12.847 V
Battery Current: -2.341 A
Instantaneous Power: -29.2 W
State of Charge: 87%
Consumed Ah: 15.23 Ah
Time to Go: 245 minutes
==================
```

## Troubleshooting

### Upload Issues
- **"Upload port not found"**
  - Check USB cable connection
  - Verify CP2102 driver is installed
  - Try different USB port
  - Check Device Manager for COM port

### Compilation Errors
- **"File not found"**
  - Ensure all files are in `src/` folder
  - Check file names match exactly
  
- **"Library not found"**
  - PlatformIO should auto-install libraries
  - Check `platformio.ini` has correct `lib_deps`

### BLE Connection Issues
- **"No Victron devices found"**
  - Ensure Victron device is powered on
  - Check device is within Bluetooth range (~10m)
  - Verify Bluetooth is enabled on Victron device
  - Some devices need to be woken up or accessed via app first

- **"Failed to connect to device"**
  - Device may be paired to another device (phone app)
  - Try power cycling the Victron device
  - Check if device appears in phone's BLE scanner

### Serial Monitor Issues
- **No output or garbage characters**
  - Check baud rate is set to 115200
  - Try pressing reset button on ESP32
  - Verify correct COM port is selected

## Development Workflow

### Fridge BLE Testing (Current Priority)
Based on last night's reverse engineering work, we've decoded the Flex Adventure fridge protocol:

**What we know:**
- **BLE Service:** `00001234-0000-1000-8000-00805f9b34fb`
- **Notify Characteristic:** `00001236-0000-1000-8000-00805f9b34fb`
- **Write Characteristic:** `00001235-0000-1000-8000-00805f9b34fb`
- **Authentication:** Keep-alive frame `FEFE03010200` every 2 seconds
- **Command format:**
  - Left temp: `FEFE0405[temp]02[checksum]`
  - Right temp: `FEFE0406[temp]02[checksum]`
- **Status frames:** `FEFE21...` periodic notifications

**Today's Testing Plan:**
1. Upload ESP32 fridge test code (from last night's session)
2. Connect to fridge via BLE and enable notifications
3. Verify keep-alive maintains connection
4. Test temperature control commands
5. Decode status frame fields (temp, setpoint, battery mode, eco mode)
6. Log extended session to map all control functions

**Why ESP32 instead of nRF Connect:**
- Real-time hex dumps and logging
- No XML macro syntax issues
- Direct path to CYD integration
- Faster testing iteration (30 second cycles)
- Automated keep-alive prevents disconnections

### Adding a New Module

1. **Create header file** (`src/new_module.h`)
2. **Create implementation file** (`src/new_module.cpp`)
3. **Add enable flag to** `config.h`
4. **Include in** `main.cpp` with `#if ENABLE_NEW_MODULE`
5. **Test independently** before enabling other modules

### Testing a Module

1. Disable all other modules in `config.h`
2. Enable only the module you're testing
3. Build and upload
4. Monitor serial output for errors
5. Once stable, enable next module

## File Structure

```
Trailer_Monitor_System/
├── src/
│   ├── main.cpp           # Main program loop
│   ├── config.h           # Configuration flags
│   ├── victron_ble.h      # Victron BLE interface
│   └── victron_ble.cpp    # Victron BLE implementation
├── platformio.ini         # Build configuration
├── PROJECT_CONTEXT.md     # Detailed project documentation
└── README.md             # This file
```

## Hardware Specifications

### ESP32 Board
- **Model:** MH-ET LIVE ESP32 DevKit
- **USB Chip:** CP2102
- **Flash:** 4MB
- **RAM:** 520KB
- **Bluetooth:** BLE 4.2
- **WiFi:** 802.11 b/g/n

### Pin Usage (Future Modules)
- **Tank sensors:** GPIO pins (TBD)
- **Temperature sensors:** OneWire bus (TBD)
- **Display:** SPI interface (when migrating to CYD)

## Power Consumption
- **Development:** USB powered (~100-150mA)
- **Deployment:** 12V trailer system with buck converter
- **Target:** <200mA total system draw

## Contributing

When adding new modules:
1. Follow the existing code structure
2. Use the same error handling patterns
3. Include auto-reconnect logic
4. Add configuration flags to `config.h`
5. Update this README with new module info
6. Document any reverse-engineered protocols

## License

Personal project - all rights reserved

## Support

For issues or questions about:
- **Hardware:** Check Device Manager and connections
- **Code:** Review serial monitor output
- **Protocols:** See PROJECT_CONTEXT.md for details

## Version History

- **v0.1.0** - Initial Victron BLE implementation
- **v0.2.0** - Fridge BLE protocol decoded (Wireshark + nRF Connect)
  - Identified service UUIDs and characteristics
  - Decoded keep-alive and command frame formats
  - Ready for ESP32 testing phase
- More versions coming as modules are added

## Current Testing Focus

### Fridge BLE Integration
**Last night's progress:**
- Successfully captured and decoded BLE communication via Wireshark
- Identified authentication pattern (2-second keep-alive)
- Documented command frame structures for temperature control
- Ready to test on ESP32 hardware

**Today's goals:**
1. Test ESP32 BLE connection to fridge
2. Verify keep-alive maintains authenticated session
3. Test temperature control commands (left/right compartments)
4. Decode status notification fields
5. Document all control functions for display integration

**Testing commands via Serial Monitor:**
- `s` - Scan for fridge
- `c` - Connect and authenticate
- `left [temp]` - Set left compartment temperature
- `right [temp]` - Set right compartment temperature  
- `d` - Disconnect

## Future Roadmap

1. ✅ Victron BLE monitoring (current)
2. ⏳ EcoFlow BLE/MQTT integration
3. ⏳ Water tank level monitoring
4. ⏳ Gas bottle monitoring
5. ⏳ Fridge temperature monitoring
6. ⏳ Touch display interface (CYD migration)
7. ⏳ Data logging and graphing
8. ⏳ Remote monitoring via WiFi/MQTT
9. ⏳ Home Assistant integration