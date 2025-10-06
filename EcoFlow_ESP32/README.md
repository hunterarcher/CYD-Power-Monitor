# EcoFlow Delta Max 2 BLE Connection Test

## Purpose
Test direct BLE connection to EcoFlow Delta Max 2 to reverse-engineer the protocol for full data access and control.

## Hardware
- ESP32 DevKit (your Master ESP32 hardware)
- EcoFlow Delta Max 2 (MAC: 74:4D:BD:CE:A2:49)

## What This Does

1. **Connects** to EcoFlow via BLE using discovered service/characteristic UUIDs
2. **Enables notifications** to receive data from the device
3. **Sends keep-alive messages** every 3 seconds to maintain connection
4. **Dumps all received data** to serial monitor for analysis

## Key Findings from Analysis

Based on Bluetooth HCI snoop logs from EcoFlow app:

- **Service UUID:** `00000001-0000-1000-8000-00805f9b34fb`
- **Write Char:** `00000002-0000-1000-8000-00805f9b34fb` (for commands)
- **Notify Char:** `00000003-0000-1000-8000-00805f9b34fb` (for data)

**Keep-alive pattern discovered:**
```
00 19 FE 07 01 00 [XX XX] 00 00 08 00 41 04 0C 00 00 00 89 00
```
- 20 bytes total
- Type `FE 07` appears to be ping/keep-alive
- Bytes 6-7 vary (likely counter/sequence)
- Without keep-alive, device disconnects after ~4-9 seconds

## How to Use

1. **Upload to ESP32:**
   ```bash
   cd "C:\Trailer\CYD Build\EcoFlow_ESP32"
   pio run --target upload
   ```

2. **Open Serial Monitor:**
   ```bash
   pio device monitor
   ```

3. **Watch for:**
   - Connection success messages
   - Keep-alive send confirmations
   - **NOTIFICATION RECEIVED** messages with data dumps

## What to Look For

### If Connection Succeeds:
- ✓ Device should stay connected (keep-alives working)
- ✓ Watch for notifications with actual data
- ✓ Hex dumps will show what the EcoFlow is sending

### If Connection Fails:
- Check EcoFlow is powered on
- Verify no other device (phone app) is connected
- Check MAC address is correct
- Ensure EcoFlow is in range

### If Disconnects After ~4 Seconds:
- Keep-alive message format might be wrong
- Try different byte patterns in keep-alive
- Device might require authentication first

## Next Steps

Once we get notifications:
1. Analyze the notification data structure
2. Compare with btsnoop app captures
3. Identify battery %, temperature, power data in bytes
4. Map out command structures for AC/DC/USB control
5. Build full protocol implementation

## Files

- `src/main.cpp` - Main test code
- `platformio.ini` - PlatformIO configuration
- `README.md` - This file

## Notes

- Uses NimBLE library (lighter than Bluedroid)
- Debug level set to 3 for verbose output
- Auto-reconnect every 10 seconds if disconnected
- Keep-alive every 3 seconds when connected
