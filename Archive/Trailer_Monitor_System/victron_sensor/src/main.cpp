#include "victron_ble.h"
#include "mbedtls/aes.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Main Display ESP32 MAC - UPDATE THIS WITH YOUR ACTUAL MAC
uint8_t mainDisplayMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Placeholder

// Forward declaration for the parser test harness added in test_victron_parser.cpp
extern void runVictronParserTests();

// Forward-declare global instance
VictronBLE victronBLE;

// BLE callback forwards advertised devices to the VictronBLE parser
class VictronBLECallback : public BLEAdvertisedDeviceCallbacks {
public:
    VictronBLECallback(VictronBLE* parent) : _parent(parent) {}
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        // forward to parent parse function
        _parent->parseVictronAdvertisement(String(advertisedDevice.getAddress().toString().c_str()), advertisedDevice.getManufacturerData(), advertisedDevice.getRSSI());
    }
private:
    VictronBLE* _parent;
};

void setup() {
    Serial.begin(115200);
    delay(2000);  // Increased delay for startup
    
    Serial.println("\n\n=== Victron BLE Sensor ESP32 ===");
    Serial.println("Setup started...");
    Serial.flush();
    
    Serial.println("Initializing ESP-NOW + BLE scanner...");
    Serial.flush();
    
    if (victronBLE.begin()) {
        Serial.println("✓ Victron BLE sensor ready!");
        Serial.println("✓ Scanning for Victron devices...");
        Serial.flush();

        // Run the local parser parity tests once at startup.
        Serial.println("--- Running Victron parser tests (one-shot) ---");
        runVictronParserTests();
        Serial.println("--- Parser tests complete; continuing normal operation ---");
        Serial.flush();
    } else {
        Serial.println("✗ Failed to initialize Victron BLE sensor");
        Serial.flush();
        while(1) {
            Serial.println("ERROR: BLE initialization failed");
            delay(5000);
        }
    }
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    
    // Print heartbeat every 10 seconds
    if (millis() - lastHeartbeat > 10000) {
        Serial.println("[HEARTBEAT] System running...");
        Serial.flush();
        lastHeartbeat = millis();
    }
    
    victronBLE.loop();
    delay(100);  // Small delay to prevent watchdog issues
}

// ========================= VictronBLE Implementation =========================

VictronBLE::VictronBLE() {
    pBLEScan = nullptr;
}

bool VictronBLE::begin() {
    // Initialize ESP-NOW first
    initESPNOW();
    
    // Initialize BLE
    Serial.println("Initializing BLE...");
    BLEDevice::init("VictronSensor");
    
    BLEScan* scan = BLEDevice::getScan();
    if (!scan) {
        Serial.println("✗ Failed to create BLE scan object");
        return false;
    }

    // store opaque pointer
    pBLEScan = (void*)scan;

    // Set advertised device callbacks to our forwarding callback
    scan->setAdvertisedDeviceCallbacks(new VictronBLECallback(&victronBLE), false);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);

    bleInitialized = true;
    Serial.println("✓ BLE initialized");

    // Start continuous scanning
    scan->start(0, false);  // 0 = continuous scan
    Serial.println("✓ BLE scanning started");
    
    return true;
}

void VictronBLE::loop() {
    // ESP-NOW and BLE are event-driven, just keep running
    delay(1000);
}

// BLE callbacks are handled by VictronBLECallback which forwards to
// VictronBLE::parseVictronAdvertisement(). The detailed device handling
// is implemented in parseVictronAdvertisement below.

void VictronBLE::initESPNOW() {
    Serial.println("Initializing ESP-NOW...");
    
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ ESP-NOW init failed");
        return;
    }
    
    // Register send callback
    esp_now_register_send_cb(onDataSent);
    
    // Add main display as peer if MAC is set (not placeholder FF:FF:...)
    bool isPlaceholder = true;
    for (int i = 0; i < 6; i++) {
        if (mainDisplayMAC[i] != 0xFF) { isPlaceholder = false; break; }
    }

    if (isPlaceholder) {
        Serial.println("⚠️ ESP-NOW peer MAC is placeholder; skipping peer add. Enable peer to send data.");
        espNowPeerAdded = false;
    } else {
        esp_now_peer_info_t peerInfo;
        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, mainDisplayMAC, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("✗ Failed to add ESP-NOW peer");
            espNowPeerAdded = false;
        } else {
            espNowPeerAdded = true;
            Serial.println("✓ ESP-NOW initialized and peer added");
        }
    }
}

void VictronBLE::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("✓ Data sent successfully");
    } else {
        Serial.println("✗ Data send failed");
    }
}

void VictronBLE::sendData(const VictronData& data) {
    if (!espNowPeerAdded) {
        Serial.println("⚠️ ESP-NOW peer not configured. Skipping send.");
        return;
    }

    esp_err_t result = esp_now_send(mainDisplayMAC, (uint8_t*) &data, sizeof(data));
    
    if (result == ESP_OK) {
        Serial.println("✓ Sending ESP-NOW data...");
    } else {
        Serial.printf("✗ ESP-NOW send error: %s\n", esp_err_to_name(result));
    }
}

uint8_t VictronBLE::getDeviceType(const String& address) {
    if (address == BMV712_MAC) return 1;  // BMV-712
    if (address == MPPT_MAC) return 2;    // MPPT
    if (address == IP22_MAC) return 3;    // IP22
    return 0;  // Unknown
}

void VictronBLE::parseVictronAdvertisement(const String& address, const std::string& manufacturerData, int rssi) {
    // Skip manufacturer ID (first 2 bytes)
    if (manufacturerData.length() < 4) return;
    
    const uint8_t* data = (const uint8_t*)manufacturerData.c_str() + 2;
    size_t len = manufacturerData.length() - 2;
    
    VictronData victronData;
    victronData.deviceType = getDeviceType(address);
    address.toCharArray(victronData.deviceMAC, 18);
    victronData.rssi = rssi;
    victronData.timestamp = millis();
    
    // Initialize values
    victronData.voltage = 0.0;
    victronData.current = 0.0;
    victronData.power = 0.0;
    victronData.soc = 0.0;
    
    // Parse based on your working ESPHome/HACS logic
    parseInstantReadout(data, len, address, victronData);
    
    // Send data via ESP-NOW
    sendData(victronData);
}

String VictronBLE::getEncryptionKey(const String& address) {
    if (address == BMV712_MAC) return BMV712_KEY;
    if (address == MPPT_MAC) return MPPT_KEY;
    if (address == IP22_MAC) return IP22_KEY;
    return "";
}

bool VictronBLE::decryptVictronData(const uint8_t* encryptedData, size_t dataLen, const String& key, uint16_t iv, uint8_t* decrypted) {
    Serial.print("[AES] Starting decryption, key length: "); Serial.print(key.length());
    Serial.print(", data length: "); Serial.println(dataLen);
    Serial.flush();

    if (key.length() != 32 || dataLen == 0) {
        Serial.println("[AES] ✗ Invalid key length or data length");
        Serial.flush();
        return false;
    }

    // Convert hex key string to bytes
    uint8_t keyBytes[16];
    for (int i = 0; i < 16; i++) {
        String byteStr = key.substring(i * 2, i * 2 + 2);
        keyBytes[i] = strtoul(byteStr.c_str(), NULL, 16);
    }

    // Construct 16-byte IV per victron convention (nonce LSB-first in bytes 0..1)
    uint8_t ivBytes[16];
    memset(ivBytes, 0, sizeof(ivBytes));
    ivBytes[0] = iv & 0xFF;
    ivBytes[1] = (iv >> 8) & 0xFF;

    // Use mbedtls AES-CTR
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    int ret = mbedtls_aes_setkey_enc(&ctx, keyBytes, 128);
    if (ret != 0) {
        Serial.printf("[AES] ✗ mbedtls setkey failed: %d\n", ret);
        mbedtls_aes_free(&ctx);
        return false;
    }

    size_t nc_off = 0;
    unsigned char stream_block[16];
    memset(stream_block, 0, sizeof(stream_block));

    ret = mbedtls_aes_crypt_ctr(&ctx, dataLen, &nc_off, ivBytes, stream_block, encryptedData, decrypted);
    mbedtls_aes_free(&ctx);
    if (ret != 0) {
        Serial.printf("[AES] ✗ mbedtls ctr failed: %d\n", ret);
        return false;
    }

    Serial.println("[AES] ✓ Decryption complete");
    return true;
}

void VictronBLE::parseBitStream(const uint8_t* data, size_t dataLen, uint16_t model_id, uint8_t mode, VictronData& victronData) {
    // Parse using exact device detection logic from victron_ble Python library
    Serial.printf("[PARSE] Model: 0x%04X, Mode: 0x%02X\n", model_id, mode);
    
    // Use the same detection logic as victron_ble library
    if (mode == 0x02) {  // BatteryMonitor
        Serial.println("[PARSE] Battery Monitor (BMV) parsing...");
        parseBatteryMonitor(data, dataLen, victronData);
        
    } else if (mode == 0x01) {  // SolarCharger
        Serial.println("[PARSE] Solar Charger (MPPT) parsing...");
        parseSolarCharger(data, dataLen, victronData);
        
    } else if (mode == 0x08) {  // AcCharger
        Serial.println("[PARSE] AC Charger parsing...");
        parseAcCharger(data, dataLen, victronData);
        
    } else if (mode == 0x0D) {  // DcEnergyMeter
        Serial.println("[PARSE] DC Energy Meter parsing...");
        // TODO: Add DC Energy Meter parsing
        
    } else if (mode == 0x04) {  // DcDcConverter
        Serial.println("[PARSE] DC-DC Converter parsing...");
        // TODO: Add DC-DC Converter parsing
        
    } else if (mode == 0x03) {  // Inverter
        Serial.println("[PARSE] Inverter parsing...");
        // TODO: Add Inverter parsing
        
    } else {
        Serial.printf("[PARSE] Unsupported device: Model=0x%04X, Mode=0x%02X\n", model_id, mode);
        // For now, try to guess based on our known devices and model IDs
        if (model_id == 0x8302) {  // Known BMV/SmartShunt
            Serial.println("[PARSE] Fallback: Battery Monitor (BMV) parsing...");
            parseBatteryMonitor(data, dataLen, victronData);
        } else if (model_id == 0x6002) {  // Known MPPT
            Serial.println("[PARSE] Fallback: Solar Charger (MPPT) parsing...");
            parseSolarCharger(data, dataLen, victronData);
        } else if (model_id == 0x2E00) {  // Known AC Charger
            Serial.println("[PARSE] Fallback: AC Charger parsing...");
            parseAcCharger(data, dataLen, victronData);
        }
    }
}

void VictronBLE::parseBatteryMonitor(const uint8_t* data, size_t dataLen, VictronData& victronData) {
    Serial.print("[BMV] Parsed bytes: ");
    for (size_t i = 0; i < min(dataLen, (size_t)20); i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();

    // Need at least header (32 bits) + fields up to SOC (bit 150 -> 151 bits) => 19 bytes
    if (dataLen * 8 < 150) {
        Serial.println("[BMV] Insufficient bit length for full battery monitor parsing");
    }

    BitReader reader(data, dataLen);
    // Skip 32-bit record header (record type, nonce, key byte) per Victron spec
    if (reader.canRead(32)) {
        (void)reader.readUnsignedInt(32);
    } else {
        Serial.println("[BMV] Not enough bits to skip header");
    }

    // TTG (16 bits)
    uint32_t ttg = 0;
    if (reader.canRead(16)) ttg = reader.readUnsignedInt(16);
    (void)ttg;

    // Battery voltage (16 bits) in 0.01 V units
    uint32_t raw_voltage = 0;
    if (reader.canRead(16)) raw_voltage = reader.readUnsignedInt(16);
    bool voltageValid = true;
    if (raw_voltage == 0x7FFF) {
        victronData.voltage = 0.0;
        voltageValid = false;
    } else {
        victronData.voltage = raw_voltage / 100.0;
    }

    // Alarm reason (16 bits) - skip
    uint32_t alarm_reason = 0;
    if (reader.canRead(16)) alarm_reason = reader.readUnsignedInt(16);
    (void)alarm_reason;

    // Aux / mid voltage / temperature (16 bits) - skip
    uint32_t aux = 0;
    if (reader.canRead(16)) aux = reader.readUnsignedInt(16);
    (void)aux;

    // Aux input (2 bits) - skip
    uint32_t aux_input = 0;
    if (reader.canRead(2)) aux_input = reader.readUnsignedInt(2);
    (void)aux_input;

    // Battery current (22 bits) in mA, signed
    uint32_t raw_current_u = 0;
    if (reader.canRead(22)) raw_current_u = reader.readUnsignedInt(22);
    bool currentValid = true;
    if (raw_current_u == 0x3FFFFF) {
        victronData.current = 0.0;
        currentValid = false;
    } else {
        int32_t raw_current_ma = BitReader::toSignedInt(raw_current_u, 22);
        victronData.current = raw_current_ma / 1000.0; // convert mA to A
    }

    // Consumed Ah (20 bits) - skip
    uint32_t consumed_ah = 0;
    if (reader.canRead(20)) consumed_ah = reader.readUnsignedInt(20);
    (void)consumed_ah;

    // SOC (10 bits) in 0.1% units
    uint32_t raw_soc = 0;
    if (reader.canRead(10)) raw_soc = reader.readUnsignedInt(10);
    if (raw_soc == 0x3FF) {
        victronData.soc = 0.0;
    } else {
        victronData.soc = raw_soc / 10.0;
    }

    // Compute power only if both voltage and current were present
    if (voltageValid && currentValid) {
        victronData.power = victronData.voltage * victronData.current;
    } else {
        victronData.power = 0.0;
    }

    Serial.printf("[BMV] V=%.2fV%s, I=%.3fA%s, SOC=%.1f%%\n",
                  victronData.voltage, (voltageValid?"":" (NA)"),
                  victronData.current, (currentValid?"":" (NA)"),
                  victronData.soc);
}

void VictronBLE::parseSolarCharger(const uint8_t* data, size_t dataLen, VictronData& victronData) {
    Serial.print("[MPPT] Parsed bytes: ");
    for (size_t i = 0; i < min(dataLen, (size_t)20); i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();

    // Minimal length check - fields extend through PV power (bit ~112)
    if (dataLen * 8 < 112) {
        Serial.println("[MPPT] Insufficient bit length for full solar charger parsing");
    }

    BitReader reader(data, dataLen);
    // Skip 32-bit record header (record type, nonce, key byte) per Victron spec
    if (reader.canRead(32)) {
        (void)reader.readUnsignedInt(32);
    } else {
        Serial.println("[MPPT] Not enough bits to skip header");
    }

    // Device state (8 bits)
    uint32_t device_state = 0;
    if (reader.canRead(8)) device_state = reader.readUnsignedInt(8);
    (void)device_state;

    // Charger error (8 bits)
    uint32_t charger_error = 0;
    if (reader.canRead(8)) charger_error = reader.readUnsignedInt(8);
    (void)charger_error;

    // Battery voltage (16 bits) 0.01 V
    uint32_t raw_batt_v = 0;
    if (reader.canRead(16)) raw_batt_v = reader.readUnsignedInt(16);
    bool battVValid = true;
    if (raw_batt_v == 0x7FFF) {
        victronData.voltage = 0.0;
        battVValid = false;
    } else {
        victronData.voltage = raw_batt_v / 100.0;
    }

    // Battery current (16 bits) signed in 0.1A units
    uint32_t raw_batt_i_u = 0;
    if (reader.canRead(16)) raw_batt_i_u = reader.readUnsignedInt(16);
    bool battIValid = true;
    if (raw_batt_i_u == 0x7FFF) {
        victronData.current = 0.0;
        battIValid = false;
    } else {
        int32_t raw_batt_i = BitReader::toSignedInt(raw_batt_i_u, 16);
        victronData.current = raw_batt_i / 10.0;
    }

    // Yield today (16 bits) - skip
    uint32_t yield_today = 0;
    if (reader.canRead(16)) yield_today = reader.readUnsignedInt(16);
    (void)yield_today;

    // PV power (16 bits) in W
    uint32_t pv_power = 0;
    if (reader.canRead(16)) pv_power = reader.readUnsignedInt(16);
    if (pv_power == 0xFFFF) pv_power = 0;
    // Prefer PV power field if present; otherwise compute from V*I when both valid
    if (pv_power > 0) {
        victronData.power = (float)pv_power;
    } else if (battVValid && battIValid) {
        victronData.power = victronData.voltage * victronData.current;
    } else {
        victronData.power = 0.0;
    }

    victronData.soc = 0.0; // MPPT doesn't report SOC

    Serial.printf("[MPPT] V=%.2fV%s, I=%.2fA%s, P=%.0fW\n",
                  victronData.voltage, (battVValid?"":" (NA)"),
                  victronData.current, (battIValid?"":" (NA)"),
                  victronData.power);
}

void VictronBLE::parseAcCharger(const uint8_t* data, size_t dataLen, VictronData& victronData) {
    // AcCharger parse_decrypted() from victron_ble library
    BitReader reader(data, dataLen);

    // Skip 32-bit record header (record type, nonce, key byte) per Victron spec
    if (reader.canRead(32)) {
        (void)reader.readUnsignedInt(32);
    } else {
        Serial.println("[AC] Not enough bits to skip header");
    }

    uint32_t charge_state = 0;
    if (reader.canRead(8)) charge_state = reader.readUnsignedInt(8);

    uint32_t charger_error = 0;
    if (reader.canRead(8)) charger_error = reader.readUnsignedInt(8);

    uint32_t output_voltage1 = 0;
    if (reader.canRead(13)) output_voltage1 = reader.readUnsignedInt(13);

    uint32_t output_current1 = 0;
    if (reader.canRead(11)) output_current1 = reader.readUnsignedInt(11);

    uint32_t output_voltage2 = 0;
    if (reader.canRead(13)) output_voltage2 = reader.readUnsignedInt(13);

    uint32_t output_current2 = 0;
    if (reader.canRead(11)) output_current2 = reader.readUnsignedInt(11);

    uint32_t output_voltage3 = 0;
    if (reader.canRead(13)) output_voltage3 = reader.readUnsignedInt(13);

    uint32_t output_current3 = 0;
    if (reader.canRead(11)) output_current3 = reader.readUnsignedInt(11);

    uint32_t temperature = 0;
    if (reader.canRead(7)) temperature = reader.readUnsignedInt(7);

    uint32_t ac_current = 0;
    if (reader.canRead(9)) ac_current = reader.readUnsignedInt(9);

    // Apply conversions as per victron_ble library  
    victronData.voltage = (output_voltage1 != 0x1FFF) ? output_voltage1 / 100.0 : 0.0;
    victronData.current = (output_current1 != 0x7FF) ? output_current1 / 10.0 : 0.0;
    victronData.power = victronData.voltage * victronData.current;
    victronData.soc = 0.0;  // AC charger doesn't report SOC

    Serial.printf("[AC] Output: %.2fV, %.1fA, %.1fW\n", 
                  victronData.voltage, victronData.current, victronData.power);
}

void VictronBLE::parseInstantReadout(const uint8_t* data, size_t len, const String& address, VictronData& victronData) {
    if (len < 7) return;
    
    // Check for instant readout record type (0x10)
    if (data[0] == 0x10) {
        Serial.println("✓ Instant readout packet found");
        
        // Print raw manufacturer data for analysis
        Serial.print("Raw MfgData: ");
        for (size_t i = 0; i < len; i++) {
            Serial.printf("%02X ", data[i]);
        }
        Serial.println();
        
        // Parse Victron BLE advertisement structure matching victron_ble library:
        // [0x10][model_id(2 bytes, little-endian)][mode][iv(2 bytes, little-endian)][encrypted_data...]
        uint16_t model_id = data[1] | (data[2] << 8);  // Little-endian like Python struct.unpack("<H")
        uint8_t mode = data[3];  // This is the mode field used for device detection
        uint16_t iv = data[4] | (data[5] << 8);  // Little-endian like Python struct.unpack("<H")
        
        Serial.printf("Model ID: 0x%04X, Mode: 0x%02X, IV: 0x%04X\n", model_id, mode, iv);
        
        // Detect device type using the same logic as victron_ble library
        String deviceTypeStr = "Unknown";
        
        // Handle non-standard modes first (observed in actual devices)
        if (mode == 0xA0 || mode == 0xA3) {
            // For devices with non-standard modes, detect by specific model_id
            if (model_id == 0x6002) {
                deviceTypeStr = "SolarCharger";
            } else if (model_id == 0x8302) {
                deviceTypeStr = "BatteryMonitor";
            } else if (model_id == 0x2E00) {
                deviceTypeStr = "AcCharger";
            }
        }
        
        // Standard mode-based detection for normal devices
        if (deviceTypeStr == "Unknown") {
            if (mode == 0x02) {
                deviceTypeStr = "BatteryMonitor";
            } else if (mode == 0x01) {
                deviceTypeStr = "SolarCharger";
            } else if (mode == 0x08) {
                deviceTypeStr = "AcCharger";
            } else if (mode == 0x0D) {
                deviceTypeStr = "DcEnergyMeter";
            } else if (mode == 0x04) {
                deviceTypeStr = "DcDcConverter";
            } else if (mode == 0x03) {
                deviceTypeStr = "Inverter";
            } else if (mode == 0x0A) {
                deviceTypeStr = "LynxSmartBMS";
            } else if (mode == 0x05) {
                deviceTypeStr = "SmartLithium";
            } else if (mode == 0x09) {
                deviceTypeStr = "SmartBatteryProtect";
            } else if (mode == 0x0C) {
                deviceTypeStr = "VEBus";
            } else if (mode == 0x0F) {
                deviceTypeStr = "OrionXS";
            }
        }
        Serial.printf("[DETECT] Device type: %s (mode=0x%02X)\n", deviceTypeStr.c_str(), mode);
        
        // Get encryption key for this device using the MAC address
        String encryptionKey = getEncryptionKey(address);
        if (encryptionKey.length() == 0) {
            Serial.println("✗ No encryption key available for this device");
            return;
        }
        
        // Decrypt the encrypted payload - some devices include extra header bytes
        // inside the encrypted section. Try offsets 0..6 to find the correct slice
        // that yields a plausible parse (voltage in expected range, current sane).
        const uint8_t* encryptedDataFull = &data[6];
        size_t encryptedFullLen = len - 6;

        // Helper: attempt decrypt+parse on offsets until plausible
        bool parsedOk = false;
        uint8_t decrypted[32];
        for (size_t offset = 0; offset <= 6 && offset < encryptedFullLen; offset++) {
            size_t sliceLen = encryptedFullLen - offset;
            const uint8_t* slicePtr = encryptedDataFull + offset;
            // Decrypt slice into buffer
            if (!decryptVictronData(slicePtr, sliceLen, encryptionKey, iv, decrypted)) {
                Serial.printf("[AES] Decrypt failed for offset %u\n", (unsigned)offset);
                continue;
            }
            // Temporary VictronData to test parse
            VictronData tmp = victronData; // copy basic fields like deviceType/MAC/RSSI
            parseBitStream(decrypted, sliceLen, model_id, mode, tmp);
            // Heuristic plausibility check
            bool plaus = (tmp.voltage >= 10.0 && tmp.voltage <= 150.0 && fabs(tmp.current) <= 1000.0);
            Serial.printf("[PARSE-TRY] offset=%u -> V=%.2f I=%.3f plaus=%s\n", (unsigned)offset, tmp.voltage, tmp.current, (plaus?"YES":"NO") );
            if (plaus) {
                // Accept this parse
                victronData = tmp;
                parsedOk = true;
                Serial.printf("[PARSE] Accepted offset %u\n", (unsigned)offset);
                break;
            }
        }
        if (!parsedOk) {
            Serial.println("[PARSE] No plausible parse found with offsets 0..6; using offset 0 result");
            // Fallback: decrypt and parse the full slice (offset 0)
            if (encryptedFullLen > 0 && decryptVictronData(encryptedDataFull, encryptedFullLen, encryptionKey, iv, decrypted)) {
                parseBitStream(decrypted, encryptedFullLen, model_id, mode, victronData);
            }
        }
        
        Serial.printf("Parsed: %.2fV, %.2fA, %.1fW, %.1f%% SOC\n", 
                     victronData.voltage, victronData.current, victronData.power, victronData.soc);
    }
}