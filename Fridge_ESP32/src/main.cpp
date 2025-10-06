#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include "FridgeData.h"

/**
 * PHASE 3: CONTROL COMMANDS TEST
 *
 * Now that we have stable connection and accurate decoding, test CONTROL commands!
 *
 * CONTROL COMMANDS DISCOVERED:
 * - Set Temperature: FE FE 04 [zone] [temp] 02 [checksum]
 * - ECO Mode: FE FE 1C 02 00 01 [eco] [battery] ...
 * - Battery Protection: FE FE 1C 02 00 01 [eco] [battery] ...
 *
 * TEST SEQUENCE:
 * 1. Connect to fridge (Phase 2.5 proven stable)
 * 2. Send keep-alive frames (maintain connection)
 * 3. After 30 seconds: Send temperature change command
 * 4. Observe fridge response in notifications
 * 5. Verify change in official app
 *
 * SAFETY:
 * - Only changes setpoint (safe operation)
 * - Fridge will reject invalid commands
 * - Can always revert via official app
 * - Conservative 30-second wait before test
 *
 * What happens:
 * 1. Connect and observe (30 seconds)
 * 2. Send: Set LEFT temp to 5°C
 * 3. Watch for status frame confirmation
 * 4. User verifies in app
 */

// ============ CONFIGURATION ============
#define SCAN_TIME 10                  // Scan for 10 seconds
#define MIN_OPERATION_DELAY 5000      // 5 seconds between operations
#define MEDIUM_DELAY 4000             // 4 seconds for less critical ops
#define POST_SCAN_DELAY 5000          // 5 seconds after finding fridge
#define CONNECTION_TIMEOUT 30000      // 30 second timeout
#define RECONNECT_DELAY 300000        // Wait 5 MINUTES before reconnecting
#define MAX_CONNECTION_ATTEMPTS 5     // Try up to 5 times per scan (like nRF Connect)
#define RETRY_DELAY 200               // 200ms between connection retries (like nRF's fast retry)
#define KEEPALIVE_INTERVAL 2000       // Send keep-alive every 2 seconds (REQUIRED!)
#define KEEPALIVE_START_DELAY 3000    // Wait 3 seconds after connection before first keep-alive

// ============ GLOBAL OBJECTS ============
BLEScan* pBLEScan;
BLEClient* pClient = nullptr;
BLERemoteService* pRemoteService = nullptr;
BLERemoteCharacteristic* pNotifyCharacteristic = nullptr;
BLERemoteCharacteristic* pWriteCharacteristic = nullptr;

// ============ STATE MANAGEMENT ============
ConnectionState currentState = STATE_DISCONNECTED;
FridgeData fridgeData;
BLEAdvertisedDevice* targetDevice = nullptr;

unsigned long lastOperationTime = 0;
unsigned long lastConnectionAttempt = 0;
unsigned long connectionStartTime = 0;
unsigned long lastKeepAlive = 0;
uint32_t notificationCount = 0;
uint32_t connectionAttempts = 0;
uint32_t keepAlivesSent = 0;
uint32_t connectionRetries = 0;
bool shouldReconnect = false;

// ============ UTILITY FUNCTIONS ============

void setState(ConnectionState newState, const char* message) {
    currentState = newState;
    Serial.printf("\n[STATE] %s\n", message);
}

void safeDelay(unsigned long ms, const char* reason) {
    Serial.printf("[DELAY] Waiting %lu ms - %s\n", ms, reason);
    delay(ms);
    lastOperationTime = millis();
}

void logError(const char* error) {
    Serial.printf("\n❌ ERROR: %s\n", error);
    setState(STATE_ERROR, "Error occurred");
}

// ============ BLE CALLBACKS ============

// Notification callback - receives data from fridge
void notifyCallback(BLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    notificationCount++;

    Serial.println("\n========================================");
    Serial.printf("📨 NOTIFICATION #%d RECEIVED!\n", notificationCount);
    Serial.println("========================================");
    Serial.printf("Length: %d bytes\n", length);
    Serial.print("Data: ");

    for (size_t i = 0; i < length; i++) {
        Serial.printf("%02X ", pData[i]);
        if ((i + 1) % 16 == 0) Serial.print("\n      ");
    }
    Serial.println();

    // Check for known frame types
    if (length >= 3 && pData[0] == 0xFE && pData[1] == 0xFE) {
        uint8_t frameType = pData[2];
        Serial.printf("\n✓ Valid frame detected (FE FE %02X)\n", frameType);

        switch(frameType) {
            case 0x03:
                Serial.println("  Type: Keep-alive response");
                break;
            case 0x04:
                Serial.println("  Type: Temperature response");
                if (length >= 5) {
                    Serial.printf("  Subtype: 0x%02X\n", pData[3]);
                    Serial.printf("  Value: 0x%02X (dec: %d, signed: %d)\n",
                                 pData[4], pData[4], (int8_t)pData[4]);
                }
                break;
            case 0x21:
                Serial.println("  Type: Status frame (DECODED)");

                // Decode status frame - CONFIRMED PROTOCOL
                // FE FE 21 [3] [4] [5] [6] [7] [8] [9] [10-19]...
                if (length >= 20) {
                    Serial.println("\n  📊 DECODED STATUS:");

                    // Byte 3: Zone/compartment ✓ CONFIRMED
                    uint8_t zone = pData[3];
                    Serial.printf("     🔹 Zone: 0x%02X ", zone);
                    if (zone == 0x01) Serial.println("(LEFT compartment)");
                    else if (zone == 0x02) Serial.println("(RIGHT compartment)");
                    else Serial.println("(UNKNOWN)");

                    // Byte 6: ECO mode ✓ CONFIRMED
                    uint8_t eco_mode = pData[6];
                    Serial.printf("     🔹 ECO Mode: 0x%02X ", eco_mode);
                    if (eco_mode == 0x00) Serial.println("(ON)");
                    else if (eco_mode == 0x01) Serial.println("(OFF)");
                    else Serial.println("(UNKNOWN)");

                    // Byte 7: Battery protection level ✓ CONFIRMED
                    uint8_t battery_protection = pData[7];
                    Serial.printf("     🔹 Battery Protection: 0x%02X ", battery_protection);
                    if (battery_protection == 0x00) Serial.println("(L = 8.5V)");
                    else if (battery_protection == 0x01) Serial.println("(M = 10.1V - assumed)");
                    else if (battery_protection == 0x02) Serial.println("(H = 11.1V)");
                    else Serial.println("(UNKNOWN)");

                    // Byte 8: Temperature setpoint ✓ CONFIRMED
                    int8_t setpoint = (int8_t)pData[8];
                    Serial.printf("     🔹 Setpoint: %d°C", setpoint);
                    if (zone == 0x01) Serial.println(" (LEFT)");
                    else if (zone == 0x02) Serial.println(" (RIGHT)");
                    else Serial.println();

                    // Byte 18: LEFT/RIGHT ACTUAL temperature ✓ CONFIRMED
                    int8_t actual_temp = (int8_t)pData[18];
                    Serial.printf("     🔹 Actual Temp: %d°C", actual_temp);
                    if (zone == 0x01) Serial.println(" (LEFT)");
                    else if (zone == 0x02) Serial.println(" (RIGHT)");
                    else Serial.println();

                    // Status flags
                    Serial.printf("\n     Status flags: 0x%02X 0x%02X\n", pData[4], pData[5]);

                    // Sequence/counter byte
                    Serial.printf("     Sequence: 0x%02X\n", pData[18]);

                    // Store decoded values
                    fridgeData.last_zone = zone;
                    fridgeData.eco_mode = (eco_mode == 0x00);

                    // Store setpoint and actual temp based on zone
                    if (zone == 0x01) {
                        fridgeData.left_setpoint = setpoint;
                        fridgeData.left_actual = actual_temp;  // ✓ CONFIRMED: Byte[18] is LEFT actual
                    } else if (zone == 0x02) {
                        fridgeData.right_setpoint = setpoint;
                        // RIGHT actual comes from 16-byte frame (byte[10])
                    }

                    // Store battery protection
                    if (battery_protection == 0x00) fridgeData.battery_percent = 85;      // L
                    else if (battery_protection == 0x01) fridgeData.battery_percent = 101; // M
                    else if (battery_protection == 0x02) fridgeData.battery_percent = 111; // H

                    fridgeData.status_byte1 = pData[4];
                    fridgeData.status_byte2 = pData[5];
                    fridgeData.last_status_frame = millis();

                    Serial.println("     ✓ Values stored");
                } else {
                    Serial.printf("  ⚠️  Status frame too short (%d bytes, expected 20+)\n", length);
                }
                break;
            default:
                Serial.printf("  Type: Unknown (0x%02X)\n", frameType);
                break;
        }
    } else {
        // This is the ACTUAL TEMPERATURE frame! (16-byte)
        if (length == 16) {
            Serial.println("\n  Type: Actual Temperature Frame (DECODED)");
            Serial.println("\n  🌡️  RIGHT COMPARTMENT DATA:");

            // Byte[2]: RIGHT setpoint ✓ CONFIRMED
            int8_t right_setpoint = (int8_t)pData[2];
            Serial.printf("     🔹 RIGHT Setpoint: %d°C\n", right_setpoint);

            // Byte[10]: RIGHT actual temperature ✓ CONFIRMED
            int8_t right_actual = (int8_t)pData[10];
            Serial.printf("     🔹 RIGHT Actual: %d°C\n", right_actual);

            // Byte[0]: Sequence/counter
            Serial.printf("\n     Sequence: 0x%02X\n", pData[0]);

            // Store in global data (LEFT actual comes from status frame)
            fridgeData.right_setpoint = right_setpoint;
            fridgeData.right_actual = right_actual;

            Serial.println("     ✓ Values stored");
        } else if (length >= 3) {
            Serial.println("\n⚠️  Unknown frame format (not FE FE ...)");
            // Check for other FE FE frame types we might have missed
            if (pData[0] == 0xFE && pData[1] == 0xFE) {
                Serial.printf("  ⚠️  Found FE FE frame with type 0x%02X (not in switch!)\n", pData[2]);
            }
        } else {
            Serial.println("\n⚠️  Unknown frame format");
        }
    }

    Serial.println("========================================\n");

    fridgeData.last_seen = millis();
}

// Client callbacks - connection events
class ClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pClient) {
        Serial.println("\n✓ BLE Client connected!");
    }

    void onDisconnect(BLEClient* pClient) {
        Serial.println("\n⚠️  BLE Client disconnected!");
        setState(STATE_DISCONNECTED, "Disconnected from fridge");
        fridgeData.connected = false;
        shouldReconnect = true;
        lastConnectionAttempt = millis();
    }
};

// ============ CONTROL COMMAND FUNCTIONS ============

/**
 * Calculate checksum for temperature command
 * Pattern from btsnoop: Sum of bytes [2..5], then subtract 6
 */
uint8_t calculateChecksum(uint8_t* data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 2; i < len; i++) {  // Sum bytes starting from index 2
        sum += data[i];
    }
    return sum - 6;  // Subtract 6 (pattern from btsnoop analysis)
}

/**
 * Set temperature for LEFT or RIGHT compartment
 *
 * @param temp Temperature in Celsius (-20 to +20)
 * @param zone 0x05 = LEFT, 0x06 = RIGHT
 */
bool setTemperature(int8_t temp, uint8_t zone) {
    if (!pWriteCharacteristic) {
        Serial.println("⚠️  Cannot set temperature - not connected");
        return false;
    }

    if (temp < -20 || temp > 20) {
        Serial.println("⚠️  Temperature out of range (-20 to +20)");
        return false;
    }

    if (zone != 0x05 && zone != 0x06) {
        Serial.println("⚠️  Invalid zone (0x05=LEFT, 0x06=RIGHT)");
        return false;
    }

    uint8_t cmd[7] = {0xFE, 0xFE, 0x04, zone, (uint8_t)temp, 0x02, 0x00};
    cmd[6] = calculateChecksum(cmd, 6);

    Serial.printf("\n📤 Setting temperature to %d°C (%s compartment)\n",
                  temp, zone == 0x05 ? "LEFT" : "RIGHT");
    Serial.print("   Command: ");
    for (int i = 0; i < 7; i++) {
        Serial.printf("%02X ", cmd[i]);
    }
    Serial.println();

    pWriteCharacteristic->writeValue(cmd, 7, true);
    return true;
}

/**
 * Toggle ECO mode
 *
 * @param enabled true = ECO ON, false = ECO OFF
 */
bool setEcoMode(bool enabled) {
    if (!pWriteCharacteristic) {
        Serial.println("⚠️  Cannot set ECO mode - not connected");
        return false;
    }

    uint8_t cmd[20] = {
        0xFE, 0xFE, 0x1C, 0x02, 0x00, 0x01,
        enabled ? 0x00 : 0x01,  // ECO byte (0x00=ON, 0x01=OFF)
        0x02,  // Keep current battery setting (H)
        0x03, 0x14, 0xEC, 0x02, 0x00, 0x00,
        0xFD, 0xFD, 0xFD, 0x00, 0xF1, 0x00
    };

    Serial.printf("\n📤 Setting ECO mode to %s\n", enabled ? "ON" : "OFF");

    pWriteCharacteristic->writeValue(cmd, 20, true);
    return true;
}

/**
 * Set battery protection level
 *
 * @param level 0 = L (8.5V), 1 = M (10.1V), 2 = H (11.1V)
 */
bool setBatteryProtection(uint8_t level) {
    if (!pWriteCharacteristic) {
        Serial.println("⚠️  Cannot set battery protection - not connected");
        return false;
    }

    if (level > 2) {
        Serial.println("⚠️  Invalid battery level (0=L, 1=M, 2=H)");
        return false;
    }

    uint8_t cmd[20] = {
        0xFE, 0xFE, 0x1C, 0x02, 0x00, 0x01,
        0x00,  // Keep current ECO setting (ON)
        level,  // Battery protection level
        0x03, 0x14, 0xEC, 0x02, 0x00, 0x00,
        0xFD, 0xFD, 0xFD, 0x00, 0xF1, 0x00
    };

    const char* levelNames[] = {"L (8.5V)", "M (10.1V)", "H (11.1V)"};
    Serial.printf("\n📤 Setting battery protection to %s\n", levelNames[level]);

    pWriteCharacteristic->writeValue(cmd, 20, true);
    return true;
}

// Scan callback - find fridge
class FridgeScanCallback : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String deviceMAC = advertisedDevice.getAddress().toString().c_str();
        deviceMAC.toLowerCase();

        if (deviceMAC == FRIDGE_TARGET_MAC) {
            Serial.println("\n🍺 FRIDGE FOUND!");
            Serial.printf("   MAC: %s\n", deviceMAC.c_str());
            Serial.printf("   RSSI: %d dBm\n", advertisedDevice.getRSSI());

            // Store device for connection
            targetDevice = new BLEAdvertisedDevice(advertisedDevice);

            // Stop scanning
            pBLEScan->stop();

            fridgeData.detected = true;
            fridgeData.rssi = advertisedDevice.getRSSI();
            fridgeData.mac_address = deviceMAC;
        }
    }
};

// ============ CONNECTION SEQUENCE ============

bool connectToFridge() {
    setState(STATE_CONNECTING, "Attempting connection to fridge...");

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  PHASE 2.5: CONNECTION + KEEP-ALIVE   ║");
    Serial.println("╚════════════════════════════════════════╝\n");

    Serial.println("⚠️  SAFETY: Keep-alive only");
    Serial.println("   - Will enable notifications");
    Serial.println("   - Will send keep-alive every 2 seconds");
    Serial.println("   - Will NOT send temperature commands");
    Serial.println("   - Just maintain connection\n");

    connectionAttempts++;
    connectionStartTime = millis();
    connectionRetries = 0;

    // Step 1: Create client
    Serial.println("[Step 1] Creating BLE client...");
    if (pClient != nullptr) {
        delete pClient;
    }
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallback());
    Serial.println("✓ Client created");

    safeDelay(MIN_OPERATION_DELAY, "Post client creation");

    // Step 2: Connect to device (WITH RETRIES like nRF Connect)
    Serial.println("\n[Step 2] Connecting to fridge...");
    Serial.printf("   Target: %s\n", FRIDGE_TARGET_MAC);
    Serial.printf("   Will retry up to %d times\n\n", MAX_CONNECTION_ATTEMPTS);

    bool connected = false;
    for (connectionRetries = 0; connectionRetries < MAX_CONNECTION_ATTEMPTS; connectionRetries++) {
        Serial.printf("   Attempt %d/%d...\n", connectionRetries + 1, MAX_CONNECTION_ATTEMPTS);

        if (pClient->connect(targetDevice)) {
            connected = true;
            Serial.println("   ✓ Connected!");
            break;
        } else {
            Serial.printf("   ✗ Failed (attempt %d)\n", connectionRetries + 1);
            if (connectionRetries < MAX_CONNECTION_ATTEMPTS - 1) {
                Serial.printf("   Waiting %d ms before retry...\n", RETRY_DELAY);
                delay(RETRY_DELAY);
            }
        }
    }

    if (!connected) {
        Serial.printf("\n❌ All %d connection attempts failed!\n", MAX_CONNECTION_ATTEMPTS);
        logError("Connection failed after all retries!");
        return false;
    }

    Serial.printf("\n✓ Connected successfully on attempt %d\n", connectionRetries + 1);
    safeDelay(1000, "Post connection stabilization (reduced)");

    // Step 3: Discover services
    Serial.println("\n[Step 3] Discovering services...");
    setState(STATE_DISCOVERING_SERVICES, "Discovering BLE services");

    pRemoteService = pClient->getService(FRIDGE_SERVICE_UUID);
    if (pRemoteService == nullptr) {
        logError("Service not found!");
        pClient->disconnect();
        return false;
    }

    Serial.println("✓ Service found");
    Serial.printf("   UUID: %s\n", FRIDGE_SERVICE_UUID);
    safeDelay(500, "Post service discovery (reduced)");

    // Step 4: Get characteristics AND ENUMERATE ALL HANDLES
    Serial.println("\n[Step 4] Getting characteristics...");
    setState(STATE_GETTING_CHARACTERISTICS, "Getting characteristics");

    // DIAGNOSTIC: List ALL characteristics and their handles
    Serial.println("\n🔍 DIAGNOSTIC: Enumerating ALL characteristics:");
    std::map<std::string, BLERemoteCharacteristic*>* charMap = pRemoteService->getCharacteristics();
    for (auto &entry : *charMap) {
        BLERemoteCharacteristic* pChar = entry.second;
        Serial.printf("   Handle: 0x%04X, UUID: %s\n",
                     pChar->getHandle(),
                     pChar->getUUID().toString().c_str());
    }
    Serial.println("🔍 END DIAGNOSTIC\n");

    // Get notify characteristic
    pNotifyCharacteristic = pRemoteService->getCharacteristic(FRIDGE_NOTIFY_CHAR_UUID);
    if (pNotifyCharacteristic == nullptr) {
        logError("Notify characteristic not found!");
        pClient->disconnect();
        return false;
    }
    Serial.printf("✓ Notify characteristic found\n");
    Serial.printf("   UUID: %s\n", FRIDGE_NOTIFY_CHAR_UUID);
    Serial.printf("   Handle: 0x%04X\n", pNotifyCharacteristic->getHandle());

    // Get write characteristic
    pWriteCharacteristic = pRemoteService->getCharacteristic(FRIDGE_WRITE_CHAR_UUID);
    if (pWriteCharacteristic == nullptr) {
        logError("Write characteristic not found!");
        pClient->disconnect();
        return false;
    }
    Serial.printf("✓ Write characteristic found\n");
    Serial.printf("   UUID: %s\n", FRIDGE_WRITE_CHAR_UUID);
    Serial.printf("   Handle: 0x%04X ⚠️  CHECK IF THIS IS 0x0006!\n", pWriteCharacteristic->getHandle());

    safeDelay(500, "Post characteristic discovery (reduced)");

    // Step 5: Enable notifications (MANUALLY like nRF Connect)
    Serial.println("\n[Step 5] Enabling notifications...");
    setState(STATE_ENABLING_NOTIFICATIONS, "Enabling notifications");

    Serial.println("⚠️  IMPORTANT: Enabling notifications ONLY");
    Serial.println("   - Using manual CCCD write (like nRF Connect)");
    Serial.println("   - Not sending any data");
    Serial.println("   - Not sending keep-alive");
    Serial.println("   - Just listening\n");

    if (!pNotifyCharacteristic->canNotify()) {
        logError("Characteristic cannot notify!");
        pClient->disconnect();
        return false;
    }

    // First, register the callback (but don't enable yet)
    Serial.println("[5a] Registering callback function...");
    pNotifyCharacteristic->registerForNotify(notifyCallback, false, false);
    Serial.println("✓ Callback registered (notifications not enabled yet)");

    safeDelay(500, "Post callback registration (reduced)");

    // Now manually write to CCCD descriptor (0x2902) like nRF Connect does
    Serial.println("\n[5b] Manually enabling notifications via CCCD write...");
    Serial.println("   Finding CCCD descriptor (0x2902)...");

    BLERemoteDescriptor* pCCCD = pNotifyCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
    if (pCCCD == nullptr) {
        logError("CCCD descriptor not found!");
        pClient->disconnect();
        return false;
    }

    Serial.println("✓ CCCD descriptor found");
    safeDelay(500, "Pre-CCCD write (reduced)");

    // Write 0x01 0x00 to enable notifications (exactly like nRF Connect)
    Serial.println("   Writing 0x01 0x00 to CCCD...");
    uint8_t notificationOn[] = {0x01, 0x00};
    pCCCD->writeValue(notificationOn, 2, true);

    Serial.println("✓ CCCD write complete - Notifications enabled!");

    safeDelay(1000, "Post notification enable (reduced)");

    // Step 6: Start keep-alive sender
    Serial.println("\n[Step 6] Starting keep-alive loop...");
    Serial.println("⚠️  CRITICAL: Keep-alive is REQUIRED");
    Serial.println("   - Sending FE FE 03 01 02 00 every 2 seconds");
    Serial.println("   - Fridge will disconnect without it");
    Serial.println("   - This prevents GATT CONN TIMEOUT\n");

    // Wait a bit before first keep-alive (reduced based on nRF timing)
    safeDelay(1000, "Before first keep-alive (reduced)");

    // Send first keep-alive immediately
    Serial.println("   Sending FIRST keep-alive...");
    Serial.print("   Data: ");
    for (int i = 0; i < KEEPALIVE_FRAME_SIZE; i++) {
        Serial.printf("%02X ", KEEPALIVE_FRAME[i]);
    }
    Serial.println();

    pWriteCharacteristic->writeValue((uint8_t*)KEEPALIVE_FRAME, KEEPALIVE_FRAME_SIZE, true);
    keepAlivesSent++;
    lastKeepAlive = millis();

    Serial.println("   ✓ First keep-alive sent!\n");
    delay(1000);  // Small delay after first keep-alive

    // Step 7: Connected and maintaining connection
    Serial.println("\n[Step 7] Connection complete!");
    setState(STATE_CONNECTED_OBSERVING, "Connected - Keep-alive active");

    fridgeData.connected = true;

    unsigned long connectionTime = millis() - connectionStartTime;
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  CONNECTION SUCCESSFUL!                ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.printf("\nConnection time: %lu ms\n", connectionTime);
    Serial.printf("Connection attempts: %d\n", connectionAttempts);
    Serial.printf("Connection retries: %d\n\n", connectionRetries);

    Serial.println("📝 NOW RUNNING:");
    Serial.println("   - Keep-alive every 2 seconds (automatic)");
    Serial.println("   - Monitoring notifications from fridge");
    Serial.println("   - Connection should stay stable");
    Serial.println("   - Watch for responses to keep-alive\n");

    return true;
}

// ============ SETUP ============
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  FRIDGE PHASE 3: CONTROL COMMANDS");
    Serial.println("========================================");
    Serial.println();
    Serial.println("⚠️  TEST MODE: Control Commands");
    Serial.println("   - Will connect to fridge (with retries)");
    Serial.println("   - Will enable notifications");
    Serial.println("   - Will send keep-alive every 2 seconds");
    Serial.println("   - Will TEST temperature change after 30 seconds");
    Serial.println();
    Serial.printf("Current Phase: %d (KEEPALIVE)\n", CURRENT_PHASE);
    Serial.printf("Target MAC: %s\n", FRIDGE_TARGET_MAC);
    Serial.printf("Connection retries: %d max\n", MAX_CONNECTION_ATTEMPTS);
    Serial.printf("Keep-alive interval: %d ms\n", KEEPALIVE_INTERVAL);
    Serial.printf("Min operation delay: %d ms\n", MIN_OPERATION_DELAY);
    Serial.printf("Reconnect delay: %d seconds\n\n", RECONNECT_DELAY / 1000);

    // Initialize BLE
    Serial.println("Initializing BLE...");
    BLEDevice::init("Fridge-Observer");

    // Create scanner
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new FridgeScanCallback());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    Serial.println("✓ BLE initialized\n");

    // Initialize fridge data
    fridgeData.detected = false;
    fridgeData.connected = false;
    fridgeData.rssi = 0;
    fridgeData.last_seen = 0;

    setState(STATE_DISCONNECTED, "Ready to scan for fridge");

    Serial.println("========================================");
    Serial.println("Starting scan for fridge...");
    Serial.println("========================================\n");
}

// ============ LOOP ============
void loop() {
    unsigned long currentTime = millis();

    switch (currentState) {
        case STATE_DISCONNECTED:
            // Check if enough time has passed since last attempt
            if (shouldReconnect && (currentTime - lastConnectionAttempt < RECONNECT_DELAY)) {
                // Still in cooldown period
                unsigned long remaining = (RECONNECT_DELAY - (currentTime - lastConnectionAttempt)) / 1000;
                if (remaining % 10 == 0) {  // Log every 10 seconds
                    Serial.printf("⏳ Cooldown: %lu seconds remaining...\n", remaining);
                    delay(1000);
                }
                break;
            }

            // Ready to scan
            setState(STATE_SCANNING, "Scanning for fridge...");
            Serial.printf("\n[SCAN] Looking for fridge (MAC: %s)...\n", FRIDGE_TARGET_MAC);
            Serial.printf("[SCAN] Duration: %d seconds\n\n", SCAN_TIME);

            targetDevice = nullptr;
            fridgeData.detected = false;

            pBLEScan->start(SCAN_TIME, false);

            if (targetDevice != nullptr && fridgeData.detected) {
                Serial.println("✓ Fridge found!");
                safeDelay(POST_SCAN_DELAY, "Post-scan stabilization");

                // Attempt connection
                if (connectToFridge()) {
                    // Success!
                    shouldReconnect = false;
                } else {
                    // Failed
                    if (MAX_CONNECTION_ATTEMPTS > 0 && connectionAttempts >= MAX_CONNECTION_ATTEMPTS) {
                        Serial.println("\n⚠️  Max connection attempts reached!");
                        Serial.println("   SAFETY: Auto-reconnect disabled");
                        Serial.println("   Power cycle ESP32 to try again");
                        Serial.println("   This prevents aggressive retry behavior\n");
                        shouldReconnect = false;  // Stop trying
                        setState(STATE_ERROR, "Max attempts reached - halted");
                    } else {
                        Serial.println("\n⚠️  Connection failed. Will retry after cooldown.\n");
                        shouldReconnect = true;
                        lastConnectionAttempt = millis();
                        setState(STATE_DISCONNECTED, "Connection failed - cooldown");
                    }
                }
            } else {
                Serial.println("\n✗ Fridge not found in scan");
                Serial.println("   - Check fridge is powered on");
                Serial.println("   - Check fridge is in range");
                Serial.println("   - Will retry in 30 seconds\n");
                delay(30000);
            }

            pBLEScan->clearResults();
            break;

        case STATE_CONNECTED_OBSERVING:
            // Send keep-alive every 2 seconds (CRITICAL!)
            if (currentTime - lastKeepAlive >= KEEPALIVE_INTERVAL) {
                Serial.printf("[Keep-Alive #%d] Sending... ", keepAlivesSent + 1);

                pWriteCharacteristic->writeValue((uint8_t*)KEEPALIVE_FRAME, KEEPALIVE_FRAME_SIZE, true);
                keepAlivesSent++;
                lastKeepAlive = currentTime;

                Serial.println("✓ Sent");
            }

            // ============ TEST MODE: CONTROL COMMANDS ============
            // Send test command after 30 seconds of connection
            static bool testCommand1Sent = false;
            static bool testCommand2Sent = false;
            static unsigned long testCommandTime = 0;

            if (!testCommand1Sent && (currentTime - connectionStartTime >= 30000)) {
                testCommand1Sent = true;
                testCommandTime = currentTime;

                Serial.println("\n╔════════════════════════════════════════╗");
                Serial.println("║  PHASE 3: TESTING CONTROL COMMANDS    ║");
                Serial.println("╚════════════════════════════════════════╝\n");

                Serial.println("⚠️  TEST 1: Will send temperature change in 5 seconds...");
                Serial.println("   Current LEFT setpoint: 3°C");
                Serial.println("   Will change to: 5°C");
                Serial.println("   Watch for fridge response!\n");

                delay(5000);

                // Test 1: Set LEFT temperature to 5°C
                setTemperature(5, 0x05);  // 5°C, LEFT compartment

                Serial.println("\n✓ Test command 1 sent!");
                Serial.println("   Watch notifications for setpoint change to 5°C");
                Serial.println("   Will send command 2 in 20 seconds...\n");
            }

            // Send second test command 20 seconds after first
            if (testCommand1Sent && !testCommand2Sent &&
                (currentTime - testCommandTime >= 20000)) {
                testCommand2Sent = true;

                Serial.println("\n⚠️  TEST 2: Changing back to 3°C...\n");

                // Test 2: Set LEFT temperature back to 3°C
                setTemperature(3, 0x05);  // 3°C, LEFT compartment

                Serial.println("\n✓ Test command 2 sent!");
                Serial.println("   Watch notifications for setpoint change to 3°C");
                Serial.println("   Check app to verify both commands worked!\n");
            }

            // Periodic status update every 60 seconds
            static unsigned long lastStatusUpdate = 0;
            if (currentTime - lastStatusUpdate >= 60000) {
                lastStatusUpdate = currentTime;

                unsigned long connectedTime = (currentTime - connectionStartTime) / 1000;
                unsigned long timeSinceLastKeepAlive = (currentTime - lastKeepAlive) / 1000;

                Serial.println("\n╔════════════════════════════════════════╗");
                Serial.println("║  PERIODIC STATUS UPDATE                ║");
                Serial.println("╚════════════════════════════════════════╝");
                Serial.printf("\nConnected for: %lu seconds\n", connectedTime);
                Serial.printf("Keep-alives sent: %d\n", keepAlivesSent);
                Serial.printf("Last keep-alive: %lu seconds ago\n", timeSinceLastKeepAlive);
                Serial.printf("Notifications received: %d\n", notificationCount);

                if (fridgeData.last_seen > 0) {
                    Serial.printf("Last notification: %lu seconds ago\n", (currentTime - fridgeData.last_seen) / 1000);
                } else {
                    Serial.println("Last notification: Never");
                }

                Serial.printf("RSSI: %d dBm\n", fridgeData.rssi);

                // Display last decoded status
                if (fridgeData.last_status_frame > 0) {
                    Serial.println("\n╔════════════════════════════════════════╗");
                    Serial.println("║  🌡️  FRIDGE STATUS (FULLY DECODED)    ║");
                    Serial.println("╚════════════════════════════════════════╝");

                    Serial.println("\n📦 LEFT Compartment:");
                    Serial.printf("   Actual:    %d°C\n", fridgeData.left_actual);
                    Serial.printf("   Setpoint:  %d°C\n", fridgeData.left_setpoint);

                    Serial.println("\n📦 RIGHT Compartment:");
                    Serial.printf("   Actual:    %d°C\n", fridgeData.right_actual);
                    Serial.printf("   Setpoint:  %d°C\n", fridgeData.right_setpoint);

                    Serial.println("\n⚙️  Settings:");
                    Serial.printf("   ECO Mode: %s\n", fridgeData.eco_mode ? "ON" : "OFF");

                    if (fridgeData.battery_percent == 85) Serial.println("   Battery Protection: L (8.5V)");
                    else if (fridgeData.battery_percent == 101) Serial.println("   Battery Protection: M (10.1V)");
                    else if (fridgeData.battery_percent == 111) Serial.println("   Battery Protection: H (11.1V)");

                    Serial.printf("\n   Last update: %lu seconds ago\n",
                                 (currentTime - fridgeData.last_status_frame) / 1000);
                }

                // Watchdog check
                if (timeSinceLastKeepAlive > 5) {
                    Serial.println("\n⚠️  WARNING: Keep-alive delayed!");
                    Serial.println("   This should send every 2 seconds");
                }

                Serial.println();
            }
            break;

        case STATE_ERROR:
            Serial.println("⚠️  In error state. Resetting in 30 seconds...");
            delay(30000);
            setState(STATE_DISCONNECTED, "Reset from error");
            shouldReconnect = true;
            lastConnectionAttempt = millis();
            break;

        default:
            // Should not reach here
            delay(100);
            break;
    }

    delay(100);  // Small delay to prevent watchdog issues
}
