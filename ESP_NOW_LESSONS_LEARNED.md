# ESP-NOW Bidirectional Communication - Lessons Learned
## Date: 2025-10-01

## THE BREAKTHROUGH - What Actually Works

### Critical Configuration Discovery

We isolated ESP-NOW bidirectional communication and **proved it works** with these exact settings:

#### Master Device (with Web Server):
```cpp
// WiFi Setup
WiFi.mode(WIFI_AP_STA);  // ← CRITICAL: Must use AP_STA, not pure AP or pure STA
WiFi.softAP("PowerMonitor", "12345678");

// ESP-NOW Peer Config
peerInfo.channel = 1;  // AP defaults to channel 1
peerInfo.encrypt = false;

// Loop
void loop() {
    server.handleClient();
    yield();  // ← CRITICAL: Use yield(), not delay(10)
}
```

#### Slave Device (BLE Scanner):
```cpp
// WiFi Setup
WiFi.mode(WIFI_STA);
esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // ← CRITICAL: Must match Master

// ESP-NOW Peer Config
peerInfo.channel = 1;  // Must match Master AP channel
peerInfo.encrypt = false;

// Loop
void loop() {
    // Keep scans SHORT - long scans block ESP-NOW receive!
    victron.scan(0.5);  // 500ms max
    yield();
}
```

### Test Results

**Test_ESP_NOW_Master + Test_ESP_NOW_Slave:**
- ✅ Both directions working perfectly
- ✅ Master sends via serial 'S' command → Slave receives
- ✅ Slave sends via serial 'S' command → Master receives
- ✅ Web server responsive on http://192.168.4.1
- ✅ Send success rate: 100%

**Real Victron_ESP32 + Master_ESP32:**
- ✅ Victron → Master: Works perfectly (192 byte packets with all device data)
- ❌ Master → Victron: **FAILS** - send returns OK but callback shows failed
- Issue: BLE scanning blocks ESP-NOW receive interrupt

---

## What We Tried Before (All Failed)

### Failed Approach #1: Pure WIFI_AP Mode
```cpp
WiFi.mode(WIFI_AP);  // ✗ DOES NOT WORK
WiFi.softAP(ssid, password);
```
**Result:** ESP-NOW sends hang/crash. No callback triggered.

### Failed Approach #2: WIFI_STA Connected to External Router
```cpp
WiFi.mode(WIFI_STA);
WiFi.begin("Rocket", password);  // Connect to external WiFi
```
**Result:**
- Sends fail with "✗ Failed" callback
- Channel matching issues
- Not standalone (needs router)

### Failed Approach #3: WiFi.channel() Auto-Detect
```cpp
peerInfo.channel = WiFi.channel();  // Auto-detect from connection
```
**Result:** Channel mismatch, unreliable, fails after WiFi events

### Failed Approach #4: delay(10) in Loop
```cpp
void loop() {
    server.handleClient();
    delay(10);  // ✗ Blocks web server and ESP-NOW
}
```
**Result:** Extremely slow web page (20-30 second loads)

---

## The Real Problem: BLE Scanning Blocks ESP-NOW

### Current Issue

**Victron_ESP32** runs three BLE operations in loop:
1. `victron.scan(duration)` - Victron devices BLE scan
2. `scanEcoFlow()` - EcoFlow BLE scan
3. Fridge BLE connection & keep-alive

**Problem:** These are **blocking operations** that prevent ESP-NOW receive callback from firing.

### Evidence

Master serial shows:
```
[Control] Set left temp to 5°C - ✓ Sent
[ESP-NOW] Send status: ✗ Failed
  Failed to send to: 78:21:84:9C:9B:88
```

Victron serial shows:
- **NO** "[ESP-NOW] *** RECEIVE CALLBACK TRIGGERED ***" message
- Messages never reach receive callback
- Device is stuck in BLE scan

### Attempts to Fix

| Scan Duration | Result |
|--------------|--------|
| 10 seconds   | Completely blocked, no receives |
| 3 seconds    | Still blocked, occasional timeout |
| 1 second     | Improved but still fails |
| 0.5 seconds  | **Crashes** - "Starting 0 second scan..." then stalls |

**0.5 second scan issue:** Library may have minimum scan time requirement.

---

## MAC Address Discovery

In **WIFI_AP_STA** mode, ESP32 has TWO MAC addresses:

```
Master Device:
   STA MAC: 7C:87:CE:31:FE:50  ← Used for ESP-NOW
   AP MAC:  7C:87:CE:31:FE:51  ← Used for WiFi clients
```

**Important:** Victron must send to the **STA MAC**, not AP MAC.

---

## Architecture That Works

### Test Setup (Proven Working)

```
Test_ESP_NOW_Master (CYD)          Test_ESP_NOW_Slave (Victron)
├─ WiFi: AP_STA mode               ├─ WiFi: STA mode, channel 1
├─ Creates "PowerMonitor" AP       ├─ No web server
├─ Web server on 192.168.4.1       ├─ No BLE scanning
├─ ESP-NOW on channel 1            ├─ ESP-NOW on channel 1
├─ Loop: yield() only              ├─ Loop: yield() only
└─ ✅ Bidirectional works          └─ ✅ Bidirectional works
```

### Real System (Partially Working)

```
Master_ESP32 (CYD)                 Victron_ESP32
├─ WiFi: AP_STA mode               ├─ WiFi: STA mode, channel 1
├─ Creates "PowerMonitor" AP       ├─ BLE scan: victron.scan()
├─ Web server (5 devices)          ├─ BLE scan: scanEcoFlow()
├─ ESP-NOW receive: ✅ WORKS       ├─ BLE: Fridge connection
├─ ESP-NOW send: ❌ FAILS          ├─ ESP-NOW send: ✅ WORKS
└─ Loop: yield()                   └─ ESP-NOW receive: ❌ BLOCKED
```

---

## Next Steps to Try

### Option 1: Reduce BLE Scan Frequency
Instead of scanning every loop, scan once every 5-10 seconds:
```cpp
unsigned long lastScan = 0;
void loop() {
    if (millis() - lastScan > 5000) {
        victron.scan(1);
        scanEcoFlow();
        lastScan = millis();
    }
    yield();  // Free time for ESP-NOW
}
```

### Option 2: Use FreeRTOS Tasks
Put BLE scanning on separate task with lower priority:
```cpp
xTaskCreate(bleScanTask, "BLE", 4096, NULL, 1, NULL);  // Low priority
xTaskCreate(espNowTask, "ESP-NOW", 4096, NULL, 5, NULL);  // High priority
```

### Option 3: Non-Blocking BLE Scan
Check if victron-ble library supports async/callback mode instead of blocking scan.

### Option 4: Separate Devices
- One ESP32: BLE scanning only (sends via ESP-NOW)
- Another ESP32: ESP-NOW receive + control (bidirectional)

---

## Key Files Modified

### Test_ESP_NOW_Master/ (Working Test)
- `src/main.cpp` - Bare-bones ESP-NOW + web server
- `platformio.ini` - ESP32 config

### Test_ESP_NOW_Slave/ (Working Test)
- `src/main.cpp` - Bare-bones ESP-NOW receiver

### Master_ESP32/ (Real System)
**Changes Applied:**
- Removed `WiFi.begin("Rocket")` connection
- Changed to standalone AP: "PowerMonitor" / "12345678"
- Set `peerInfo.channel = 1` (fixed, not auto)
- Changed `delay(10)` → `yield()` in loop
- Removed auto-refresh from web page (was reloading every 5s)

### Victron_ESP32/ (Real System)
**Changes Applied:**
- Already had `esp_wifi_set_channel(1)`
- Set `peerInfo.channel = 1`
- Reduced scan from 10s → 0.5s (but crashes)
- Added `yield()` calls after scans

---

## What We Know For Sure

✅ **ESP-NOW bidirectional DOES work** with AP_STA + STA configuration
✅ **WIFI_AP_STA mode is mandatory** for Master with web server
✅ **Channel 1 explicit config works** better than auto-detect
✅ **yield() is critical** for responsive web server
✅ **BLE scanning blocks ESP-NOW** receive interrupts
✅ **Short scans (< 1s) help** but may have minimum limits
❌ **Current architecture incompatible** - BLE + ESP-NOW conflict

---

## Recommended Solution

**Split the BLE scanning into timed intervals:**

```cpp
unsigned long lastVictronScan = 0;
unsigned long lastEcoFlowScan = 0;
const unsigned long SCAN_INTERVAL = 5000;  // 5 seconds

void loop() {
    unsigned long now = millis();

    // Only scan every 5 seconds, not every loop
    if (now - lastVictronScan > SCAN_INTERVAL) {
        victron.scan(1);
        lastVictronScan = now;
    }

    if (now - lastEcoFlowScan > SCAN_INTERVAL + 2000) {  // Offset by 2s
        scanEcoFlow();
        lastEcoFlowScan = now;
    }

    // Handle fridge keep-alive (already time-gated)
    if (fridgeConnected && (now - lastFridgeKeepAlive >= FRIDGE_KEEPALIVE_INTERVAL)) {
        if (pFridgeWrite) {
            pFridgeWrite->writeValue((uint8_t*)FRIDGE_KEEPALIVE, 6, true);
            lastFridgeKeepAlive = now;
        }
    }

    // Prepare and send ESP-NOW packet
    prepareAndSendPacket();

    yield();  // Critical: allow ESP-NOW receive callbacks
}
```

This gives **4-5 seconds of free time** between scans for ESP-NOW messages to be received.

---

## Device MAC Addresses (Reference)

| Device | MAC Address |
|--------|-------------|
| Master_ESP32 (CYD) STA | 7C:87:CE:31:FE:50 |
| Master_ESP32 (CYD) AP  | 7C:87:CE:31:FE:51 |
| Victron_ESP32          | 78:21:84:9C:9B:88 |

---

## Error Reference

### ESP_ERR_WIFI_SSID (0x300a)
```
[E][STA.cpp:347] connect(): STA connect failed! 0x300a: ESP_ERR_WIFI_SSID
```
**Cause:** `WiFi.begin()` called without parameters in STA mode
**Solution:** Ignore - we're using STA for ESP-NOW, not for connecting to AP

### "Starting 0 second scan..."
**Cause:** `victron.scan(0.5)` rounds down to 0
**Solution:** Use integer seconds: `victron.scan(1)`
