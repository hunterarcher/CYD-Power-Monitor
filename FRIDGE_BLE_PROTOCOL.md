# Fridge BLE Protocol Documentation

**Device:** Flex Adventure Camping Fridge
**Model:** Dual-zone (LEFT/RIGHT) portable fridge/freezer
**BLE Name:** "TY"
**MAC Address:** FF:FF:11:C6:29:50

---

## BLE Service & Characteristics

### Service UUID
```
00001234-0000-1000-8000-00805f9b34fb
```

### Characteristics

#### Write Characteristic (Commands → Fridge)
```
UUID: 00001235-0000-1000-8000-00805f9b34fb
Properties: Write
Handle: 0x0006
```

#### Notify Characteristic (Fridge → App)
```
UUID: 00001236-0000-1000-8000-00805f9b34fb
Properties: Notify
Handle: 0x0003
CCCD Descriptor: 0x2902 (Write 0x0100 to enable notifications)
```

---

## Connection Sequence

### 1. Scanning
```cpp
BLEScan* pScan = BLEDevice::getScan();
pScan->setActiveScan(true);
BLEScanResults* results = pScan->start(10, false);  // 10 second scan

// Find device by MAC address
for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice device = results->getDevice(i);
    if (device.getAddress().toString() == "ff:ff:11:c6:29:50") {
        // Found fridge!
    }
}
```

### 2. Connection
```cpp
BLEClient* pClient = BLEDevice::createClient();
pClient->setClientCallbacks(new FridgeClientCallback());

// Connect with retries (like nRF Connect)
for (int retry = 0; retry < 5; retry++) {
    if (pClient->connect(pFridgeDevice)) {
        connected = true;
        break;
    }
    delay(200);  // 200ms between retries
}

delay(1000);  // Stabilization delay
```

### 3. Service Discovery
```cpp
BLERemoteService* pService = pClient->getService(FRIDGE_SERVICE_UUID);
if (pService == nullptr) {
    // Service not found - increase delay and retry
}
delay(500);
```

### 4. Characteristic Discovery
```cpp
// Get both characteristics
BLERemoteCharacteristic* pWrite = pService->getCharacteristic(WRITE_UUID);
BLERemoteCharacteristic* pNotify = pService->getCharacteristic(NOTIFY_UUID);

delay(500);
```

### 5. Enable Notifications
```cpp
// Register callback
pNotify->registerForNotify(fridgeNotifyCallback, false, false);
delay(500);

// Enable notifications via CCCD
BLERemoteDescriptor* pCCCD = pNotify->getDescriptor(BLEUUID((uint16_t)0x2902));
uint8_t notificationOn[] = {0x01, 0x00};
pCCCD->writeValue(notificationOn, 2, true);
delay(1000);
```

### 6. Send First Keep-Alive
```cpp
uint8_t keepalive[] = {0xFE, 0xFE, 0x03, 0x01, 0x02, 0x00};
pWrite->writeValue(keepalive, 6, true);
delay(1000);
```

### 7. Start Keep-Alive Timer
```cpp
// Send keep-alive every 2 seconds
// If keep-alives stop, fridge disconnects after ~10 seconds
```

---

## Frame Types

### Keep-Alive Frame (0x03)
**Direction:** App → Fridge
**Frequency:** Every 2 seconds (required!)
**Length:** 6 bytes

```
Offset  Value  Description
------  -----  -----------
0-1     FE FE  Frame header
2       03     Frame type (keep-alive)
3       01     Parameter 1
4       02     Parameter 2
5       00     Padding/reserved
```

**Example:**
```
FE FE 03 01 02 00
```

**Critical:** If keep-alives stop, fridge disconnects. Must be sent every 2 seconds.

---

### Temperature Command Frame (0x04)
**Direction:** App → Fridge
**Purpose:** Set temperature for LEFT or RIGHT zone
**Length:** 7 bytes

```
Offset  Value  Description
------  -----  -----------
0-1     FE FE  Frame header
2       04     Frame type (temperature command)
3       ZONE   Zone selector (0x05=LEFT, 0x06=RIGHT)
4       TEMP   Temperature (signed int8, -20 to +20°C)
5       02     Fixed value
6       CHKSUM Checksum (sum of bytes 0-5, minus 6)
```

**Zone Values:**
- `0x05` = LEFT compartment (fridge)
- `0x06` = RIGHT compartment (freezer)

**Temperature Range:**
- Minimum: -20°C
- Maximum: +20°C
- Step: 1°C

**Checksum Calculation:**
```cpp
uint8_t calculateChecksum(uint8_t* data, int len) {
    uint16_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum - 6;
}
```

**Examples:**
```
// Set LEFT to 4°C
FE FE 04 05 04 02 13
         ↑  ↑     ↑
       Zone Temp  Checksum

// Set RIGHT to -15°C (-15 = 0xF1 in int8)
FE FE 04 06 F1 02 0A
         ↑  ↑     ↑
       Zone Temp  Checksum
```

---

### Settings Command Frame (0x1C)
**Direction:** App → Fridge
**Purpose:** Set ECO mode and/or battery protection
**Length:** 20 bytes

```
Offset  Value  Description
------  -----  -----------
0-1     FE FE  Frame header
2       1C     Frame type (settings)
3-5     020001 Fixed header
6       ECO    ECO mode (0x00=ON, 0x01=OFF)
7       BAT    Battery protection (0x00=L, 0x01=M, 0x02=H)
8-19    Fixed  0314EC020000FDFDFD00F100
```

**ECO Mode Values:**
- `0x00` = ECO mode ON
- `0x01` = ECO mode OFF

**Battery Protection Values:**
- `0x00` = Low (8.5V cutoff)
- `0x01` = Medium (10.1V cutoff)
- `0x02` = High (11.1V cutoff)

**Examples:**
```
// Set ECO ON, Battery H
FE FE 1C 02 00 01 00 02 03 14 EC 02 00 00 FD FD FD 00 F1 00
                  ↑  ↑
                ECO  Battery

// Set ECO OFF, Battery M
FE FE 1C 02 00 01 01 01 03 14 EC 02 00 00 FD FD FD 00 F1 00
                  ↑  ↑
                ECO  Battery

// Set ECO OFF, Battery L
FE FE 1C 02 00 01 01 00 03 14 EC 02 00 00 FD FD FD 00 F1 00
                  ↑  ↑
                ECO  Battery
```

**Note:** Both ECO and Battery must be sent together in one command. The fridge may reject commands if values are inconsistent or invalid.

**Known Issue:** In testing, these commands were sent correctly but the fridge did not respond (values didn't change). Temperature commands work perfectly. This may require:
- Specific fridge state/mode
- Additional validation we haven't discovered
- Different fridge firmware version

---

### Status Notification Frame (0x21)
**Direction:** Fridge → App
**Trigger:** Sent automatically when values change, or every ~5 seconds
**Length:** 20+ bytes

```
Offset  Value  Description
------  -----  -----------
0-1     FE FE  Frame header
2       21     Frame type (status notification)
3       ZONE   Zone for this notification (0x01=LEFT, 0x02=RIGHT)
4-5     ----   Unknown/reserved
6       ECO    ECO mode status (0x00=OFF, 0x01=ON) *See note below*
7       BAT    Battery protection (0x00=L, 0x01=M, 0x02=H)
8       SET    Setpoint temperature (signed int8)
9-17    ----   Unknown/reserved
18      ACTUAL Actual temperature (signed int8)
19+     ----   Additional data
```

**Important:** The ECO byte in notifications uses **OPPOSITE** encoding from commands!
- **In Commands:** `0x00 = ON`, `0x01 = OFF`
- **In Notifications:** `0x01 = ON`, `0x00 = OFF`

This is a quirk of the protocol - don't assume bidirectional symmetry!

**Zone Values in Notifications:**
- `0x01` = LEFT compartment data
- `0x02` = RIGHT compartment data

**Example:**
```
FE FE 21 01 00 01 01 02 03 14 EC 02 00 00 FD FD FD 00 02 64
         ↑        ↑  ↑  ↑                             ↑
       Zone      ECO Bat Setpoint                   Actual
       (LEFT)    ON  H   3°C                         2°C
```

**Parsing Logic:**
```cpp
void fridgeNotifyCallback(BLERemoteCharacteristic* pChar,
                          uint8_t* pData, size_t length, bool isNotify) {
    if (length >= 3 && pData[0] == 0xFE && pData[1] == 0xFE) {
        uint8_t frameType = pData[2];

        if (frameType == 0x21 && length >= 20) {
            uint8_t zone = pData[3];
            uint8_t eco_mode = pData[6];
            uint8_t battery_prot = pData[7];
            int8_t setpoint = (int8_t)pData[8];
            int8_t actual = (int8_t)pData[18];

            // Store based on zone
            if (zone == 0x01) {  // LEFT
                fridgeData.left_setpoint = setpoint;
                fridgeData.left_actual = actual;
            } else if (zone == 0x02) {  // RIGHT
                fridgeData.right_setpoint = setpoint;
                fridgeData.right_actual = actual;
            }

            // ECO and Battery are global (not per-zone)
            fridgeData.eco_mode = (eco_mode == 0x01);  // Inverted!
            fridgeData.battery_protection = battery_prot;
        }
    }
}
```

---

### Temperature Only Frame (16 bytes)
**Direction:** Fridge → App
**Purpose:** Quick temperature update without full status
**Length:** 16 bytes

```
Offset  Value  Description
------  -----  -----------
0-1     ----   Header (not FE FE)
2       SET    RIGHT setpoint (signed int8)
3-9     ----   Unknown
10      ACTUAL RIGHT actual temp (signed int8)
11-15   ----   Unknown
```

**Example:**
```cpp
if (length == 16) {  // Not a FE FE frame
    int8_t right_setpoint = (int8_t)pData[2];
    int8_t right_actual = (int8_t)pData[10];

    fridgeData.right_setpoint = right_setpoint;
    fridgeData.right_actual = right_actual;
}
```

---

## Complete Code Example

### Connection and Keep-Alive
```cpp
#define FRIDGE_SERVICE_UUID "00001234-0000-1000-8000-00805f9b34fb"
#define FRIDGE_WRITE_UUID   "00001235-0000-1000-8000-00805f9b34fb"
#define FRIDGE_NOTIFY_UUID  "00001236-0000-1000-8000-00805f9b34fb"
#define FRIDGE_MAC          "ff:ff:11:c6:29:50"

const uint8_t FRIDGE_KEEPALIVE[] = {0xFE, 0xFE, 0x03, 0x01, 0x02, 0x00};
unsigned long lastKeepAlive = 0;

void loop() {
    // Send keep-alive every 2 seconds
    if (millis() - lastKeepAlive > 2000) {
        if (pFridgeWrite && fridgeConnected) {
            pFridgeWrite->writeValue((uint8_t*)FRIDGE_KEEPALIVE, 6, true);
            lastKeepAlive = millis();
        }
    }
}
```

### Set Temperature
```cpp
bool setFridgeTemperature(int8_t temp, uint8_t zone) {
    if (!pFridgeWrite || !fridgeConnected) return false;
    if (temp < -20 || temp > 20) return false;
    if (zone != 0x05 && zone != 0x06) return false;

    uint8_t cmd[7] = {
        0xFE, 0xFE, 0x04, zone, (uint8_t)temp, 0x02, 0x00
    };

    // Calculate checksum
    uint16_t sum = 0;
    for (int i = 0; i < 6; i++) sum += cmd[i];
    cmd[6] = sum - 6;

    pFridgeWrite->writeValue(cmd, 7, true);
    return true;
}

// Usage:
setFridgeTemperature(4, 0x05);   // Set LEFT to 4°C
setFridgeTemperature(-15, 0x06); // Set RIGHT to -15°C
```

### Set ECO and Battery
```cpp
bool setFridgeSettings(bool eco_on, uint8_t battery_level) {
    if (!pFridgeWrite || !fridgeConnected) return false;
    if (battery_level > 2) return false;

    uint8_t cmd[20] = {
        0xFE, 0xFE, 0x1C, 0x02, 0x00, 0x01,
        eco_on ? 0x00 : 0x01,  // ECO: 0x00=ON, 0x01=OFF
        battery_level,          // Battery: 0=L, 1=M, 2=H
        0x03, 0x14, 0xEC, 0x02, 0x00, 0x00,
        0xFD, 0xFD, 0xFD, 0x00, 0xF1, 0x00
    };

    pFridgeWrite->writeValue(cmd, 20, true);
    return true;
}

// Usage:
setFridgeSettings(true, 2);   // ECO ON, Battery H
setFridgeSettings(false, 1);  // ECO OFF, Battery M
```

---

## Timing Requirements

### Critical Delays
```cpp
// After scanning, before connecting
delay(5000);  // Allow BLE stack to settle

// After creating client
delay(5000);  // Critical for stability

// After connection
delay(1000);  // Allow connection to stabilize

// After service discovery
delay(500);

// After characteristic discovery
delay(500);

// After registering for notifications
delay(500);

// After enabling CCCD
delay(1000);

// Before first keep-alive
delay(1000);

// After first keep-alive
delay(1000);
```

### Keep-Alive Requirements
- **Frequency:** Every 2000ms (2 seconds)
- **Tolerance:** Can vary ±500ms
- **Timeout:** Fridge disconnects after ~10 seconds without keep-alive
- **First keep-alive:** Send 1-2 seconds after enabling notifications

---

## Troubleshooting

### Fridge Disconnects Frequently
**Cause:** Keep-alive frames not being sent
**Solution:** Ensure keep-alive timer is running and not blocked by other code

### Service Not Found (NULL)
**Causes:**
1. Insufficient delay after connection
2. WiFi/BLE coexistence issues
3. Service discovery interrupted

**Solutions:**
1. Increase post-connection delay to 3000ms
2. Ensure WiFi initialization doesn't block BLE
3. Don't call `getServices()` before `getService()` - go direct to specific UUID

### Commands Not Working
**Temperature Commands:**
- ✅ Should work if connection is stable
- Verify zone codes: 0x05 (LEFT), 0x06 (RIGHT)
- Check checksum calculation
- Ensure temp in range -20 to +20

**ECO/Battery Commands:**
- ⚠️ Known issue - commands send but fridge doesn't respond
- May require specific conditions we haven't discovered
- Fridge firmware may vary

### Notifications Not Received
**Causes:**
1. CCCD not written
2. Callback not registered
3. Keep-alive stopped (fridge disconnected)

**Debug:**
```cpp
void fridgeNotifyCallback(...) {
    Serial.printf("Notification: %d bytes\n", length);
    for (int i = 0; i < length; i++) {
        Serial.printf("%02X ", pData[i]);
    }
    Serial.println();
}
```

---

## Differences from Official App

### What Works the Same
- ✅ Temperature control (LEFT & RIGHT)
- ✅ Reading current temps and setpoints
- ✅ Reading ECO mode status
- ✅ Reading battery protection level
- ✅ Connection stability with keep-alive

### What Doesn't Work
- ❌ ECO mode control (app can change it, we can't)
- ❌ Battery protection control (app can change it, we can't)

### Possible Reasons
1. **Additional validation:** App might send additional frames we haven't captured
2. **Timing requirements:** Settings changes might need specific timing
3. **State requirements:** Fridge might need to be in specific state (idle, not cooling, etc.)
4. **Firmware differences:** Fridge firmware may have changed since snoop capture
5. **Handshake:** App might perform additional handshake we're missing

---

## Protocol Quirks & Gotchas

1. **Asymmetric ECO encoding:**
   - Commands use `0x00=ON, 0x01=OFF`
   - Notifications use `0x01=ON, 0x00=OFF`
   - Don't assume symmetry!

2. **Zone codes differ in commands vs notifications:**
   - Commands: `0x05=LEFT, 0x06=RIGHT`
   - Notifications: `0x01=LEFT, 0x02=RIGHT`

3. **Keep-alive is mandatory:**
   - Missing keep-alive = disconnection
   - No grace period, fridge is strict about timing

4. **Service discovery is timing-sensitive:**
   - Too fast = service not found
   - Just right = works every time
   - Working test doesn't call `getServices()` first

5. **Settings commands need both values:**
   - Can't change ECO without specifying battery
   - Can't change battery without specifying ECO
   - Must send both together

---

## Future Investigation

### To-Do
- [ ] Capture Bluetooth snoop when app successfully changes ECO mode
- [ ] Test if fridge needs to be idle (not cooling) for settings changes
- [ ] Try settings changes with different sequences of frames
- [ ] Investigate if there's a "lock" command that must be sent first
- [ ] Test with different fridge firmware versions
- [ ] Check if there's an initialization handshake we're missing

### Hypotheses
1. **Handshake theory:** App might send secret handshake before settings commands
2. **Timing theory:** Settings might need specific delay before/after
3. **State theory:** Fridge might only accept settings when in specific mode
4. **Sequence theory:** Might need to send multiple frames in specific order

---

## References

- Bluetooth snoop logs: `Export btsnoop.txt`
- Working test code: `Fridge_ESP32/src/main.cpp`
- Connection comparison: `fridge_connection_comparison.txt`
- Session notes: `SESSION_SUMMARY.md`

---

## Conclusion

The Flex Adventure Camping Fridge BLE protocol is partially reverse-engineered:
- ✅ **Temperature control:** Fully working and reliable
- ✅ **Data reading:** All values parse correctly
- ⚠️ **Settings control:** Commands send but fridge doesn't respond
- ✅ **Connection stability:** Keep-alive system works perfectly

For practical use, temperature control is the most important feature and works flawlessly. ECO and battery protection can be monitored and changed manually via the official app if needed.
