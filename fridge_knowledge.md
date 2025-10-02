# Flex Adventure Fridge - BLE Protocol Reverse Engineering

## Project Overview
Reverse engineering the Flex Adventure camping fridge BLE protocol for integration into a custom ESP32-based monitoring dashboard (CYD display) alongside Victron, EcoFlow, and tank sensors.

**Device:** Flex Adventure Fridge (Model: A1-FFFF11C62950)  
**MAC Address:** FF:FF:11:C6:29:50  
**Status:** Protocol decoded, ESP32 passive sniffer implemented, active control pending

---

## BLE Protocol - Complete Breakdown

### Service & Characteristics
```
Service UUID:     00001234-0000-1000-8000-00805f9b34fb
Notify Char:      00001236-0000-1000-8000-00805f9b34fb (Handle: 0x0004)
Write Char:       00001235-0000-1000-8000-00805f9b34fb
```

### Authentication
**IMPORTANT:** No traditional authentication/pairing required!
- No PIN codes
- No encryption key exchange
- No security manager protocol (SMP)
- No bonding

**Authentication Method:**
The fridge uses "protocol compliance" authentication:
- Must send keep-alive frame every 2 seconds
- Fridge accepts any device that follows the correct frame format
- Connection is considered "trusted" once keep-alive pattern is established

### Command Protocol

#### Keep-Alive Frame (CRITICAL)
```
Hex:    FE FE 03 01 02 00
Format: [Header] [Cmd] [Data] [Checksum?]
Timing: MUST send every 2 seconds
Purpose: Maintains "trusted" connection status
```

**Without keep-alive:** Fridge will ignore all commands

#### Temperature Control Commands
```
Left Compartment:
  FE FE 04 05 [temp] 02 [checksum]
  Example: FEFE0405F602XX (set to -10°C)

Right Compartment:
  FE FE 04 06 [temp] 02 [checksum]
  Example: FEFE0406F602XX (set to -10°C)

Temperature Format:
  - Signed 8-bit integer (int8_t)
  - Range typically: -20°C to +20°C
  - Example: 0xF6 = -10°C, 0x00 = 0°C, 0x0A = 10°C
```

#### Status Notifications (Received from Fridge)
```
Status Frame Type 1 (Common):
  FE FE 21 [19+ bytes of data]
  
Status Frame Type 2 (Temperature responses):
  FE FE 04 05 [temp] 02 [checksum]  (Left temp)
  FE FE 04 06 [temp] 02 [checksum]  (Right temp)

Keep-Alive Response:
  FE FE 03 [response data]
```

**Known Byte Positions in Status Frames:**
- Byte 0-1: Always `FE FE` (header)
- Byte 2: Frame type (`21` = status, `04` = temp, `03` = keep-alive response)
- Byte 8: Likely setpoint temperature (needs confirmation)
- Byte 9: Unknown
- Bytes 10-11: Possibly current temperature sensor reading
- Byte 18: Possibly status flags (battery mode, eco mode, etc.)
- Byte 19: Possibly checksum

**Status fields still to decode:**
- Current actual temperature (not setpoint)
- Battery mode vs. AC mode
- Eco mode status
- Compressor state
- Error codes
- Door open/close status (if available)

---

## Testing History & Lessons Learned

### What Worked (via nRF Connect)
✅ Conservative, step-by-step connection process  
✅ 3+ second delays between BLE operations  
✅ Following standard BLE client behavior  
✅ Proper characteristic discovery before access  
✅ Clean disconnect procedures  
✅ Enabling notifications before sending commands  
✅ Keep-alive pattern for maintaining connection  

### What Caused Lockouts ⚠️

#### Critical Issues That Triggered Security Lockout:
1. **Immediate characteristic access** after connection
   - Accessing characteristics before service discovery completed
   - Not waiting for connection to stabilize

2. **Aggressive connection attempts**
   - Reconnecting too quickly after disconnect
   - Multiple rapid connection attempts
   - Not respecting minimum delay between operations

3. **Improper BLE cleanup**
   - Not properly disconnecting before reconnecting
   - Leaving characteristics in inconsistent state
   - Not clearing BLE stack between attempts

4. **Wrong client behavior patterns**
   - Not following standard BLE connection sequence
   - Skipping MTU exchange
   - Not properly enabling notifications via CCCD

**Lockout Symptoms:**
- Fridge refuses all connection attempts
- Timeout period: ~5-15 minutes (variable)
- No error message, just connection rejection
- Phone app also locked out during this period

**Recovery:**
- Wait 10-15 minutes
- Power cycle fridge (if desperate)
- Ensure all BLE clients disconnected properly

### Safe Connection Sequence (MUST FOLLOW)

```
1. Initialize BLE stack
   └─ Wait 1000ms

2. Scan for fridge
   └─ Wait until found
   └─ Wait 2000ms after detection

3. Connect to device
   └─ Wait for connection confirmation
   └─ Wait 3000ms (critical!)

4. Discover services
   └─ Wait for discovery complete
   └─ Wait 2000ms

5. Get characteristics
   └─ Verify handles exist
   └─ Wait 2000ms

6. Enable notifications (CCCD)
   └─ Write 0x0001 to CCCD descriptor
   └─ Wait for confirmation
   └─ Wait 3000ms

7. Start keep-alive loop
   └─ Send FEFE03010200 every 2 seconds
   └─ Monitor for responses

8. Send commands only after keep-alive established
```

**Minimum delays between operations:** 3 seconds  
**Rationale:** Fridge firmware appears to have rate limiting / security checks

---

## Data Capture Methods

### 1. Android btsnoop_hci.log (Completed)
**How to capture:**
- Android Developer Options → Enable Bluetooth HCI snoop log
- Use phone app normally
- Extract log: `/sdcard/Android/data/btsnoop_hci.log`
- Analyze with Wireshark

**What we learned:**
- Complete protocol frame structure
- Keep-alive pattern and timing
- Command formats for temperature control
- Status notification patterns
- MTU exchange details (517 ↔ 247 bytes)

**Analysis Tools:**
- Wireshark with Bluetooth filters
- Filter: `bthci_acl && btl2cap && btatt`
- Focus on handle 0x0004 (notifications) and 0x0005 (writes)

### 2. nRF Connect (Completed)
**Successful testing approach:**
- Manual connection via app
- Service/characteristic discovery
- Enable notifications first
- Send keep-alive frames
- Test temperature commands
- Observe responses

**Benefits:**
- Visual confirmation of protocol
- Safe, controlled testing environment
- Easy to add delays between operations
- Good for validating findings

### 3. ESP32 Passive Sniffer (Current Phase)
**Implementation:** See `fridge_sniffer.ino`

**Purpose:**
- Monitor BLE advertisements without connecting
- Observe fridge behavior during phone app usage
- Capture timing patterns
- Zero risk of triggering lockouts

**What to observe:**
- Advertisement frequency
- Service UUIDs advertised
- Manufacturer data (if any)
- RSSI patterns
- Changes when phone connects/disconnects
- Name changes during different states

**Testing Protocol:**
1. Start ESP32 sniffer, establish baseline (30s)
2. Connect phone app, observe changes
3. Change temperature, observe advertisements
4. Disconnect phone app, observe return to idle
5. Reconnect phone app, verify pattern consistency

### 4. ESP32 Active Connection (Next Phase)
**Approach:** Passive connection mode
- Connect to fridge
- Enable notifications only
- DO NOT send any commands initially
- Just observe notification traffic
- Build confidence before writing

**Then:** Active control mode
- Implement keep-alive loop
- Send temperature commands
- Full integration into dashboard

---

## ESP32 Implementation Notes

### Libraries
```cpp
#include <BLEDevice.h>   // Core BLE functionality
#include <BLEUtils.h>    // Utilities
#include <BLEClient.h>   // Client operations
#include <BLEScan.h>     // Scanning
```

### Recommended Configuration
```cpp
// Scanning
#define SCAN_TIME 5
#define SCAN_INTERVAL 100
#define SCAN_WINDOW 99

// Timing safety
#define MIN_OPERATION_DELAY 3000  // 3 seconds between operations
#define KEEPALIVE_INTERVAL 2000   // 2 seconds for keep-alive

// Connection parameters
#define CONNECTION_TIMEOUT 30000  // 30 second timeout
```

### Code Structure (Modular Architecture)
Based on existing Victron/EcoFlow implementations:

```
project/
├── fridge_ble.h          // Header with data structures
├── fridge_ble.cpp        // Implementation
├── fridge_scanner.ino    // Main integration
└── docs/
    └── FRIDGE_KNOWLEDGE.md  // This document
```

### Data Structure
```cpp
struct FridgeData {
  // Temperature
  int8_t left_setpoint;      // °C
  int8_t right_setpoint;     // °C
  int8_t left_actual;        // °C (if we can decode)
  int8_t right_actual;       // °C (if we can decode)
  
  // Status
  bool connected;
  bool battery_mode;         // vs AC mode
  bool eco_mode;
  bool compressor_running;
  uint8_t error_code;
  
  // Metadata
  unsigned long last_update;
  int rssi;
  String status_message;
};
```

---

## Open Questions / To Be Decoded

### High Priority
1. **Actual temperature readings** - which bytes in status frame?
2. **Battery vs AC mode** - which flag/byte?
3. **Eco mode status** - which flag/byte?
4. **Compressor state** - is this reported?
5. **Checksum algorithm** - how is it calculated?

### Medium Priority
6. Error codes - are there any?
7. Door open/close detection - available?
8. Battery voltage - reported?
9. Power consumption - available?
10. Defrost cycle status - detectable?

### Low Priority
11. Firmware version - how to query?
12. Device capabilities - different models?
13. Historical data - stored on device?

### Testing Needed
- [ ] Confirm temperature byte positions in status frames
- [ ] Test temperature range limits
- [ ] Test invalid command rejection
- [ ] Test multiple simultaneous connections (if possible)
- [ ] Test behavior when fridge is off
- [ ] Test behavior during compressor cycles
- [ ] Long-duration connection stability (hours)
- [ ] Keep-alive timeout threshold (what happens if we miss one?)

---

## Integration Plan

### Phase 1: Passive Monitoring ✅ (Current)
- [x] ESP32 passive sniffer
- [x] Document protocol
- [x] Understand timing requirements
- [ ] Capture baseline advertisement data

### Phase 2: Passive Connection (Next)
- [ ] ESP32 connects but doesn't write
- [ ] Enable notifications only
- [ ] Log all incoming data
- [ ] Decode remaining status fields
- [ ] Build confidence in connection stability

### Phase 3: Active Control
- [ ] Implement keep-alive loop
- [ ] Add temperature control commands
- [ ] Error handling
- [ ] Reconnection logic
- [ ] Status monitoring

### Phase 4: Dashboard Integration
- [ ] Create fridge module matching Victron/EcoFlow pattern
- [ ] Display on CYD screen
- [ ] Temperature control UI
- [ ] Status indicators
- [ ] Historical graphing

---

## Wireshark Analysis Tips

### Finding Keep-Alive Pattern
```
Filter: btatt.opcode == 0x52 && btatt.handle == 0x0005
Look for: Repeating frames every 2 seconds
Pattern: 12 06 00 FE FE 03 01 02 00
```

### Finding Temperature Commands
```
Filter: btatt.opcode == 0x52 && btatt.handle == 0x0005
Look for: FE FE 04 05 or FE FE 04 06
```

### Finding Status Notifications
```
Filter: btatt.opcode == 0x1b && btatt.handle == 0x0004
Look for: FE FE 21 frames (most common)
```

### Frame Structure in Wireshark
```
Frame Header:        12 06 00          (ATT layer prefix)
Actual Command:      FE FE 03 01 02 00 (The command we care about)

To extract just the command bytes:
Right-click → Copy → ...as Hex Stream
Remove first 3 bytes (12 06 00)
```

---

## Known Compatible Hardware

### ESP32 Boards (Tested/Compatible)
- ESP32 Dev Module
- ESP32-WROOM-32
- ESP32-S3 (should work, untested)

### Display Integration
- CYD (Cheap Yellow Display) - target platform
- SPI TFT displays
- Serial debugging as fallback

### Requirements
- Bluetooth 4.0 or higher (BLE)
- Sufficient RAM for BLE stack (~40KB)
- USB programming capability

---

## Safety & Best Practices

### Connection Safety
1. **Always use delays** - minimum 3 seconds between operations
2. **Always enable notifications** before sending commands
3. **Always send keep-alive** every 2 seconds once connected
4. **Always disconnect cleanly** when done
5. **Never spam reconnect** - wait 10+ seconds between attempts

### Testing Safety
1. **Test with benign commands first** (keep-alive only)
2. **Monitor fridge behavior** during testing
3. **Have phone app available** as backup control
4. **Document everything** - what worked, what didn't
5. **Stop immediately** if lockout symptoms appear

### Code Safety
```cpp
// Good: Safe delay management
unsigned long lastOperation = 0;
if (millis() - lastOperation >= 3000) {
  // Safe to perform operation
  lastOperation = millis();
}

// Bad: Immediate consecutive operations
client->connect();
service->getCharacteristic();  // TOO FAST!
```

---

## Troubleshooting

### "Fridge won't connect"
- Wait 10-15 minutes (possible lockout)
- Check fridge is powered on
- Verify MAC address is correct
- Ensure no other devices connected
- Try power cycling ESP32
- Verify BLE stack initialized properly

### "Connection drops immediately"
- Implement proper delays (3+ seconds)
- Enable notifications before commands
- Start keep-alive immediately after connection
- Check for BLE stack cleanup issues

### "Commands ignored"
- Ensure keep-alive is running
- Verify notification characteristic enabled
- Check command frame format exactly
- Confirm write characteristic handle correct

### "Lockout occurred"
- Stop all connection attempts immediately
- Wait 15 minutes minimum
- Power cycle fridge if available
- Review code for aggressive timing
- Add more delays between operations

---

## Success Metrics

### Phase 1 Success Criteria
- [ ] ESP32 detects fridge consistently
- [ ] Advertisement data captured and logged
- [ ] Behavior during app usage documented
- [ ] No lockouts triggered

### Phase 2 Success Criteria
- [ ] Stable passive connection maintained
- [ ] All status notifications received and logged
- [ ] Keep-alive loop runs reliably for 1+ hours
- [ ] No disconnections

### Phase 3 Success Criteria
- [ ] Temperature commands work consistently
- [ ] Status fields decoded (temp, mode, etc.)
- [ ] Automatic reconnection on disconnect
- [ ] Error handling robust

### Phase 4 Success Criteria
- [ ] Full dashboard integration
- [ ] User can control temperature from CYD
- [ ] Real-time status display
- [ ] Data logging and history

---

## Resources & References

### Official App
- Android: Flex Adventure (package name unknown)
- iOS: Available but not tested

### Tools Used
- **Wireshark:** BLE protocol analysis
- **nRF Connect:** Manual BLE testing
- **Android Developer Options:** btsnoop log capture
- **Arduino IDE:** ESP32 development
- **VSCode + PlatformIO:** Alternative development (optional)

### Related Documentation
- See other project chats for Victron/EcoFlow implementations
- ESP32 BLE Arduino library documentation
- BLE GATT specification for protocol understanding

---

## Next Steps for Claude Code

1. **Review this document completely**
2. **Test ESP32 passive sniffer** - upload and monitor
3. **Capture baseline data** - fridge idle and during app usage
4. **Implement passive connection mode** - connect, enable notifications, observe
5. **Decode remaining status fields** - analyze notification frames
6. **Build active control** - implement keep-alive and temperature commands
7. **Create modular integration** - match Victron/EcoFlow architecture
8. **Dashboard integration** - CYD display with UI

---

## Version History

- **v1.0** - Initial knowledge base (2025-10-01)
  - Complete protocol documentation
  - Gotchas and lockout prevention
  - ESP32 passive sniffer implementation
  - Testing protocols and safety guidelines

---

## Notes

- This fridge protocol is relatively simple compared to commercial products
- The lack of real authentication makes reverse engineering easier but also less secure
- Keep-alive pattern is the only "authentication" mechanism
- Timing is critical - respect the delays!
- When in doubt, be more conservative with delays, not less

**CRITICAL REMINDER:** This is a physical fridge that could spoil food if misconfigured. Always test conservatively and have the official app as backup!