# Fridge ESP32 - BLE Integration Project

## Project Overview

Reverse engineering and integrating the Flex Adventure camping fridge via BLE into a custom ESP32 monitoring system.

**Device:** Flex Adventure Fridge
**MAC Address:** FF:FF:11:C6:29:50
**Model:** A1-FFFF11C62950

---

## 🚨 IMPORTANT: Phased Approach

This project uses a **slow, methodical, safety-first approach** due to:
- Risk of security lockouts (5-15 minute timeout)
- Controls a physical fridge (food safety concerns)
- Complex BLE protocol with strict timing requirements
- Previous testing showed aggressive connections trigger lockouts

**We proceed ONE PHASE AT A TIME** and do not advance until confident.

---

## Current Status

**✅ Phase 1: PASSIVE SCANNER (Current)**
- Code: Complete
- Testing: In Progress
- Risk Level: ZERO
- Status: Ready for upload and testing

**⏳ Phase 2: PASSIVE CONNECTION**
- Code: Not started
- Testing: Not started
- Risk Level: LOW
- Status: Waiting for Phase 1 completion

**⏳ Phase 3: KEEP-ALIVE ONLY**
- Code: Not started
- Testing: Not started
- Risk Level: MEDIUM
- Status: Waiting for Phase 2 completion

**⏳ Phase 4: ACTIVE CONTROL**
- Code: Not started
- Testing: Not started
- Risk Level: HIGHER
- Status: Waiting for Phase 3 completion

---

## Phase Descriptions

### Phase 1: Passive Scanner (ZERO RISK) ✅
**What it does:**
- Scans for BLE advertisements only
- NO connection attempts
- Logs fridge presence, RSSI, device name, manufacturer data
- Observes behavior during phone app usage

**Duration:** 24-48 hours of monitoring

**Goal:** Understand baseline BLE behavior

**Testing Guide:** See `PHASE1_TESTING.md`

---

### Phase 2: Passive Connection (LOW RISK) ⏳
**What it will do:**
- Connect to fridge
- Enable notifications
- Observe incoming data
- NO writes (not even keep-alive)

**Duration:** 2-4 hours of testing

**Goal:** Decode status frame structure

**Prerequisites:** Phase 1 success criteria met

---

### Phase 3: Keep-Alive Only (MEDIUM RISK) ⏳
**What it will do:**
- Full connection sequence
- Enable notifications
- Send keep-alive every 2 seconds
- Monitor responses
- NO temperature commands

**Duration:** 2-4 hours of testing

**Goal:** Establish stable "trusted" connection

**Prerequisites:** Phase 2 success criteria met

---

### Phase 4: Active Control (HIGHER RISK) ⏳
**What it will do:**
- Everything from Phase 3
- Add temperature control commands
- Full dashboard integration

**Duration:** Ongoing integration

**Goal:** Complete control and monitoring

**Prerequisites:** Phase 3 success criteria met + user approval

---

## Project Structure

```
Fridge_ESP32/
├── platformio.ini          # PlatformIO configuration
├── README.md               # This file
├── PHASE1_TESTING.md       # Current phase testing guide
├── include/
│   └── FridgeData.h        # Data structures and constants
└── src/
    └── main.cpp            # Phase 1: Passive scanner
```

---

## Known Protocol Details

### BLE Service & Characteristics
```
Service UUID:     00001234-0000-1000-8000-00805f9b34fb
Notify Char:      00001236-0000-1000-8000-00805f9b34fb (Handle: 0x0004)
Write Char:       00001235-0000-1000-8000-00805f9b34fb (Handle: 0x0005)
```

### Keep-Alive Frame (Phase 3+)
```
Hex:    FE FE 03 01 02 00
Timing: MUST send every 2 seconds
Purpose: Maintains "trusted" connection
```

### Temperature Commands (Phase 4)
```
Left Compartment:  FE FE 04 05 [temp] 02 [checksum]
Right Compartment: FE FE 04 06 [temp] 02 [checksum]

Temperature: Signed 8-bit integer (-20°C to +20°C)
Example: 0xF6 = -10°C, 0x00 = 0°C, 0x0A = 10°C
```

### Critical Timing Requirements
- **3 seconds** minimum between BLE operations
- **2 seconds** for keep-alive interval
- **Connection sequence MUST be followed exactly**
- Rushing = lockout risk

---

## Safety Mechanisms

### Built-in Protections
1. **Phase gating** - cannot skip ahead
2. **No connection code in Phase 1** - physically impossible to trigger lockout
3. **Extensive logging** - understand everything happening
4. **Conservative delays** - always erring on safe side
5. **State validation** - prevent out-of-sequence operations

### Recovery Options
- Phone app available as backup control
- Fridge can be power cycled if needed
- Testing during daytime when monitoring is easy
- Stop-and-wait approach if anything unexpected

---

## Documentation

### Primary References
- `fridge_knowledge.md` - Comprehensive protocol documentation (in main project folder)
- `flex_ble_summary.md` - Quick reference and testing notes (in main project folder)
- `PHASE1_TESTING.md` - Current phase testing protocol (this folder)

### Key Learnings from Previous Testing
- ⚠️ Immediate characteristic access after connection → lockout
- ⚠️ Rapid reconnection attempts → lockout
- ⚠️ Skipping connection sequence steps → lockout
- ⚠️ Not respecting timing delays → lockout
- ✅ Conservative, step-by-step approach → success
- ✅ 3+ second delays between operations → success
- ✅ Proper notification enable before commands → success

---

## Getting Started

### 1. Upload Phase 1 Code
```bash
# In VS Code with PlatformIO
1. Open Fridge_ESP32 folder
2. Click Upload button
3. Open Serial Monitor (115200 baud)
```

### 2. Follow Testing Protocol
See `PHASE1_TESTING.md` for detailed testing steps.

### 3. Document Observations
Keep detailed notes of:
- RSSI patterns
- Device name behavior
- Manufacturer data
- Changes when phone app connects
- Any unexpected behavior

### 4. Review & Decide
After 24-48 hours:
- Review all data collected
- Verify success criteria met
- Discuss moving to Phase 2
- Only proceed with explicit approval

---

## Success Criteria (Phase 1)

Before advancing to Phase 2:

- [ ] Fridge consistently detected (>95% of scans)
- [ ] Stable RSSI readings documented
- [ ] Device name behavior understood
- [ ] Manufacturer data pattern analyzed
- [ ] Phone app interaction documented
- [ ] 24-hour stability confirmed
- [ ] All observations logged
- [ ] No unexpected anomalies
- [ ] User approves proceeding to Phase 2

---

## Troubleshooting

See `PHASE1_TESTING.md` for detailed troubleshooting steps.

**Quick checks:**
- Is fridge powered on?
- Is ESP32 within 10 meters of fridge?
- Is serial monitor at 115200 baud?
- Is correct COM port selected?

---

## Contact & Support

- **Phone App:** Flex Adventure (available as backup)
- **Power Cycle:** Available if needed
- **Testing Time:** Daytime hours for easy monitoring

---

## Version History

- **v1.0 (2025-10-01)** - Phase 1 implementation
  - Passive scanner only
  - Zero connection risk
  - Comprehensive documentation
  - Conservative testing protocol

---

## Critical Reminders

1. **This is a physical fridge** - food safety matters
2. **Lockouts are real** - we've seen them before
3. **Timing is critical** - respect the delays
4. **Slow is fast** - methodical approach prevents setbacks
5. **When in doubt, wait** - no rush to advance phases
6. **Document everything** - helps troubleshooting and learning

---

**Current Phase:** PHASE 1 - PASSIVE SCANNER
**Risk Level:** ZERO
**Status:** Ready for testing
**Next Review:** After 24-48 hours of data collection
