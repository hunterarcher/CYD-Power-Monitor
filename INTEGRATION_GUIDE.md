# Fridge Integration Guide for Victron_ESP32

## Status
- ✓ Backups created in `Backups/PreFridgeIntegration/`
- ✓ Data structures updated in `VictronData.h`
- ✓ FridgeBLE.h helper created
- ⏳ Main integration into `main.cpp` needed

## What's Been Done

### 1. Updated VictronData.h
Added:
- `FridgeData` struct with all temperature and settings
- `ControlCommand` struct for bidirectional ESP-NOW
- Fridge commands: `CMD_FRIDGE_SET_TEMP`, `CMD_FRIDGE_SET_ECO`, `CMD_FRIDGE_SET_BATTERY`

### 2. Created FridgeBLE.h
Contains:
- Notification callback (decodes status and temperature frames)
- Control functions: `setFridgeTemperature()`, `setFridgeEcoMode()`, `setFridgeBatteryProtection()`
- Checksum calculator
- BLE client callbacks

### 3. Updated main.cpp includes
Added:
- `#include <BLEClient.h>`
- Fridge MAC and UUID definitions
- Global fridge objects

## What Needs to Be Done

### Step 1: Add FridgeBLE.h include
In `main.cpp` after other includes:
```cpp
#include "FridgeBLE.h"
```

### Step 2: Add ESP-NOW receive callback
After `onDataSent()`, add:
```cpp
// ESP-NOW receive callback for control commands
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (len == sizeof(ControlCommand)) {
        ControlCommand cmd;
        memcpy(&cmd, data, sizeof(cmd));

        if (cmd.device == 2) {  // Fridge command
            Serial.printf("[Control] Fridge command received: %d\n", cmd.command);
            switch (cmd.command) {
                case CMD_FRIDGE_SET_TEMP:
                    setFridgeTemperature(cmd.value1, cmd.value2);
                    break;
                case CMD_FRIDGE_SET_ECO:
                    setFridgeEcoMode(cmd.value1);
                    break;
                case CMD_FRIDGE_SET_BATTERY:
                    setFridgeBatteryProtection(cmd.value1);
                    break;
            }
        }
    }
}
```

### Step 3: Add fridge connection function
Before `setup()`, add:
```cpp
// Connect to fridge
bool connectToFridge() {
    Serial.println("[Fridge] Attempting connection...");

    // Create client
    if (!pFridgeClient) {
        pFridgeClient = BLEDevice::createClient();
        pFridgeClient->setClientCallbacks(new FridgeClientCallback());
    }

    // Connect
    BLEAddress fridgeAddress(FRIDGE_MAC);
    if (!pFridgeClient->connect(fridgeAddress)) {
        Serial.println("[Fridge] ✗ Connection failed");
        return false;
    }

    // Get service
    pFridgeService = pFridgeClient->getService(FRIDGE_SERVICE_UUID);
    if (!pFridgeService) {
        Serial.println("[Fridge] ✗ Service not found");
        pFridgeClient->disconnect();
        return false;
    }

    // Get characteristics
    pFridgeNotify = pFridgeService->getCharacteristic(FRIDGE_NOTIFY_UUID);
    pFridgeWrite = pFridgeService->getCharacteristic(FRIDGE_WRITE_UUID);

    if (!pFridgeNotify || !pFridgeWrite) {
        Serial.println("[Fridge] ✗ Characteristics not found");
        pFridgeClient->disconnect();
        return false;
    }

    // Enable notifications
    pFridgeNotify->registerForNotify(fridgeNotifyCallback);

    // Manually enable CCCD
    BLERemoteDescriptor* pCCCD = pFridgeNotify->getDescriptor(BLEUUID((uint16_t)0x2902));
    if (pCCCD) {
        uint8_t notificationOn[] = {0x01, 0x00};
        pCCCD->writeValue(notificationOn, 2, true);
    }

    delay(1000);

    // Send first keep-alive
    pFridgeWrite->writeValue((uint8_t*)FRIDGE_KEEPALIVE, 6, true);
    lastFridgeKeepAlive = millis();

    fridgeData.connected = true;
    fridgeConnected = true;

    Serial.println("[Fridge] ✓ Connected successfully");
    return true;
}
```

### Step 4: In `setup()`, register receive callback
After `esp_now_register_send_cb(onDataSent);`:
```cpp
// Register receive callback for control commands
esp_now_register_recv_cb(onDataRecv);
```

### Step 5: In `loop()`, add fridge logic
After packing EcoFlow data and before sending ESP-NOW:
```cpp
// ========== FRIDGE CONNECTION & DATA ==========
unsigned long currentTime = millis();

// Try to connect if not connected and enough time passed
if (!fridgeConnected && (currentTime - lastFridgeAttempt >= FRIDGE_RETRY_INTERVAL)) {
    lastFridgeAttempt = currentTime;
    connectToFridge();
}

// Send keep-alive if connected
if (fridgeConnected && (currentTime - lastFridgeKeepAlive >= FRIDGE_KEEPALIVE_INTERVAL)) {
    if (pFridgeWrite) {
        pFridgeWrite->writeValue((uint8_t*)FRIDGE_KEEPALIVE, 6, true);
        lastFridgeKeepAlive = currentTime;
    }
}

// Pack fridge data
memcpy(&packet.fridge, &fridgeData, sizeof(FridgeData));
```

### Step 6: Add fridge status display
After EcoFlow status display:
```cpp
// Fridge
Serial.println("\n--- Fridge ---");
if (packet.fridge.valid && packet.fridge.connected) {
    Serial.println("Status: ONLINE ✓");
    Serial.printf("LEFT:  Actual: %d°C, Setpoint: %d°C\n",
                  packet.fridge.left_actual, packet.fridge.left_setpoint);
    Serial.printf("RIGHT: Actual: %d°C, Setpoint: %d°C\n",
                  packet.fridge.right_actual, packet.fridge.right_setpoint);
    Serial.printf("ECO Mode: %s, Battery: %s\n",
                  packet.fridge.eco_mode ? "ON" : "OFF",
                  packet.fridge.battery_protection == 0 ? "L" :
                  packet.fridge.battery_protection == 1 ? "M" : "H");
} else {
    Serial.println("Status: OFFLINE ✗");
}
```

## Testing Steps

1. Build and upload to Victron_ESP32
2. Monitor serial output
3. Should see fridge connection attempt
4. Should see keep-alive messages every 2 seconds
5. Should see fridge data in ESP-NOW packet
6. Verify Master_ESP32 receives fridge data

## Rollback if Needed

```bash
cp -r "C:\Trailer\CYD Build\Backups\PreFridgeIntegration\Victron_ESP32_BACKUP\*" "C:\Trailer\CYD Build\Victron_ESP32\"
```

## Next: Master_ESP32 Updates

After Victron_ESP32 integration is complete:
1. Update Master_ESP32 VictronData.h (copy from Victron_ESP32)
2. Add fridge display to web UI
3. Create fridge.html detail page
4. Add control command sender via ESP-NOW

