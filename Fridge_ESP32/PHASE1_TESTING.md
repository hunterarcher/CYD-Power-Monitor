# Phase 1: Passive Scanner - Testing Protocol

## 🛡️ SAFETY LEVEL: ZERO RISK
This code **only scans for BLE advertisements**. It makes **NO connection attempts** to the fridge. There is **ZERO risk** of triggering a lockout.

---

## What This Code Does

✅ **DOES:**
- Scans for BLE devices every 30 seconds
- Detects the fridge by MAC address: `FF:FF:11:C6:29:50`
- Logs RSSI (signal strength)
- Logs device name (if advertised)
- Logs manufacturer data (if present)
- Shows all other BLE devices nearby (optional verbose mode)

❌ **DOES NOT:**
- Connect to the fridge
- Write any commands
- Read any characteristics
- Enable notifications
- Trigger any security mechanisms

---

## Upload & Run

### 1. Open in VS Code
```
Open folder: Fridge_ESP32
```

### 2. Build & Upload
- Click PlatformIO Upload button
- Or use terminal: `platformio run --target upload`

### 3. Monitor Serial Output
- Open Serial Monitor at 115200 baud
- Watch for fridge detection

---

## Expected Output

### When Fridge is Detected:
```
========================================
🍺 FRIDGE DETECTED!
========================================
Name: A1-FFFF11C62950
MAC: ff:ff:11:c6:29:50
RSSI: -65 dBm

Service UUIDs advertised:
  - 00001234-0000-1000-8000-00805f9b34fb (expected)

Manufacturer Data (8 bytes):
  FF FF 00 00 00 00 00 0A
========================================

✓ Fridge is ONLINE (seen 2 seconds ago)
  Signal: -65 dBm (Good)
```

### When Fridge is NOT Detected:
```
✗ Fridge not detected
   - Check fridge is powered on
   - Check fridge is in range
   - Verify MAC address is correct
```

---

## Testing Protocol

### Test 1: Baseline Observation (30 minutes)
**Goal:** Establish normal behavior

1. Upload code to ESP32
2. Power on fridge
3. Let scanner run for 30 minutes
4. Document:
   - Is fridge consistently detected?
   - What's the average RSSI?
   - Does device name change?
   - Is manufacturer data present?
   - Is advertisement frequency consistent?

**Expected Results:**
- Fridge detected every scan
- RSSI stable (±10 dBm variance is normal)
- Device name should be consistent
- No unexpected changes

---

### Test 2: Phone App Connection (15 minutes)
**Goal:** Observe behavior when official app connects

1. Ensure ESP32 scanner is running
2. Watch serial output
3. Open Flex Adventure phone app
4. Connect to fridge via phone app
5. Document any changes:
   - Does RSSI change?
   - Does device name change?
   - Does manufacturer data change?
   - Does fridge disappear from scans?
6. Change temperature via phone app
7. Observe any changes
8. Disconnect phone app
9. Watch for return to baseline

**Questions to Answer:**
- Does fridge remain visible during phone connection?
- Are there any advertisement pattern changes?
- Does manufacturer data encode any status?

---

### Test 3: Extended Monitoring (24 hours)
**Goal:** Long-term stability and pattern recognition

1. Leave ESP32 running overnight
2. Check logs next day
3. Look for:
   - Any detection gaps
   - RSSI patterns over time
   - Unexpected behavior
   - Advertisement changes during compressor cycles
   - Changes when switching battery/AC mode

**This helps us understand:**
- If fridge BLE is always-on or intermittent
- If there are patterns related to fridge operation
- Signal strength variations

---

## Troubleshooting

### "Fridge not detected"
**Possible causes:**
1. Fridge is powered off → Turn on fridge
2. Out of BLE range → Move ESP32 closer (<10 meters)
3. Wrong MAC address → Verify MAC with phone app or nRF Connect
4. BLE interference → Try different location

**How to verify MAC address:**
- Use nRF Connect app on phone
- Scan for devices
- Look for "A1-FFFF11C62950" or similar
- Note the exact MAC address

### "No serial output"
**Possible causes:**
1. Wrong COM port selected
2. Wrong baud rate (should be 115200)
3. USB cable is charge-only (not data)
4. Driver issues

**Solutions:**
- Check Device Manager for COM port
- Try different USB cable
- Reinstall ESP32 drivers

### "Scan takes too long"
**Normal behavior:**
- Each scan takes 5 seconds
- Scans repeat every 30 seconds
- This is intentional for thorough detection

---

## Data to Collect

Create a log file with this information:

```
Timestamp: [time]
Scan #: [number]
Detected: [yes/no]
RSSI: [value] dBm
Device Name: [name]
Manufacturer Data: [hex bytes if present]
Notes: [any observations]

Example:
Timestamp: 14:23:15
Scan #: 12
Detected: yes
RSSI: -68 dBm
Device Name: A1-FFFF11C62950
Manufacturer Data: FF FF 00 00 00 00 00 0A
Notes: Phone app connected at 14:22, no visible changes
```

---

## Success Criteria for Phase 1

Before moving to Phase 2, we need:

- [ ] Fridge consistently detected (>95% of scans)
- [ ] Stable RSSI readings
- [ ] Device name documented
- [ ] Manufacturer data pattern understood
- [ ] Behavior during phone app usage documented
- [ ] 24-hour stability confirmed
- [ ] No unexpected changes or anomalies

---

## When to Move to Phase 2

**DO NOT proceed to Phase 2 until:**
1. All success criteria met
2. We understand baseline behavior completely
3. We've documented all patterns
4. We're confident in our understanding
5. User explicitly approves moving forward

**Phase 2 will involve:**
- Actual connection to the fridge (low risk, but risk present)
- More complex testing
- Requires more careful monitoring

---

## Notes Section

Use this space to document your observations:

### Observation 1: [Date/Time]
```
[Your notes here]
```

### Observation 2: [Date/Time]
```
[Your notes here]
```

### Observation 3: [Date/Time]
```
[Your notes here]
```

---

## Questions for Analysis

After completing Phase 1 testing, answer these:

1. **Is the fridge always advertising?** Yes / No
2. **Does it disappear when phone connects?** Yes / No
3. **Does manufacturer data contain useful info?** Yes / No / Unknown
4. **What's the typical RSSI range?** _____ to _____ dBm
5. **Does device name change?** Yes / No
6. **Are there patterns we didn't expect?** Describe:
7. **Ready for Phase 2?** Yes / No / Needs more time

---

**Remember:** Take your time. There's no rush. Phase 1 is about building confidence and understanding. We only move forward when we're ready.
