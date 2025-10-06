#include "victron_ble.h"// victron_ble.cpp - Victron BLE Advertising Protocol Implementation

#include <esp_aes.h>#include <Arduino.h>

#include "victron_ble.h"

VictronBLE::VictronBLE() {#include "config.h"

  // Initialize bindkeys to zero (would need proper keys for encrypted data)

  bmv712_bindkey.fill(0);// AES encryption library for ESP32

  mppt_bindkey.fill(0);#include "mbedtls/aes.h"

  ip22_bindkey.fill(0);

}String VictronBLE::findEncryptionKey(String deviceName, String macAddress) {

  // Convert MAC address to lowercase for comparison

VictronBLE::~VictronBLE() {  String macLower = macAddress;

  if (pBLEScan) {  macLower.toLowerCase();

    pBLEScan->stop();  

  }  for (int i = 0; i < NUM_KNOWN_DEVICES; i++) {

}    String knownMac = KNOWN_VICTRON_DEVICES[i].mac;

    knownMac.toLowerCase();

bool VictronBLE::setup() {    

  if (ble_initialized) {    if (KNOWN_VICTRON_DEVICES[i].name == deviceName || knownMac == macLower) {

    return true;      Serial.printf("[Victron] Found encryption key for %s\n", deviceName.c_str());

  }      return KNOWN_VICTRON_DEVICES[i].encryptionKey;

      }

  Serial.println("Initializing Victron BLE scanner...");  }

    

  if (!BLEDevice::getInitialized()) {  Serial.printf("[Victron] No encryption key found for %s (%s)\n", 

    BLEDevice::init("Victron-Monitor");                deviceName.c_str(), macAddress.c_str());

  }  return "";

  }

  pBLEScan = BLEDevice::getScan();

  if (!pBLEScan) {void VictronBLE::hexStringToBytes(const String& hex, uint8_t* bytes, size_t maxLen) {

    Serial.println("Failed to get BLE scan object");  size_t hexLen = hex.length();

    return false;  size_t byteLen = hexLen / 2;

  }  if (byteLen > maxLen) byteLen = maxLen;

    

  pBLEScan->setAdvertisedDeviceCallbacks(this);  for (size_t i = 0; i < byteLen; i++) {

  pBLEScan->setActiveScan(true);    String byteStr = hex.substring(i * 2, i * 2 + 2);

  pBLEScan->setInterval(100);    bytes[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);

  pBLEScan->setWindow(99);  }

  }

  ble_initialized = true;

  Serial.println("Victron BLE scanner initialized");bool VictronBLE::init() {

  return true;  Serial.println("Initializing Victron BLE Advertising Listener...");

}  

  BLEDevice::init("ESP32_Trailer_Monitor");

void VictronBLE::startScan() {  

  if (!ble_initialized || !pBLEScan) {  pBLEScan = BLEDevice::getScan();

    return;  pBLEScan->setAdvertisedDeviceCallbacks(new VictronScanCallback(this), false);

  }  pBLEScan->setActiveScan(true);  // Active scan to get more data

    pBLEScan->setInterval(100);

  pBLEScan->start(0, nullptr, false); // Continuous scan  pBLEScan->setWindow(99);

}  

  Serial.println("Victron BLE Advertising Listener initialized successfully");

void VictronBLE::stopScan() {  return true;

  if (pBLEScan) {}

    pBLEScan->stop();

  }bool VictronBLE::scanForDevices() {

}  Serial.println("Scanning for Victron advertising packets...");

  deviceFound = false;

void VictronBLE::onResult(BLEAdvertisedDevice advertisedDevice) {  

  std::string mac_address = advertisedDevice.getAddress().toString();  // Start scan for advertising packets

    pBLEScan->start(VICTRON_SCAN_TIME, false);

  // Check if this is one of our target devices  

  if (mac_address != bmv712_mac && mac_address != mppt_mac && mac_address != ip22_mac) {  if (deviceFound) {

    return;    Serial.printf("Found %d Victron devices\n", devices.size());

  }    

      // Check for missing AC charger specifically

  // Get manufacturer data    bool hasACCharger = false;

  std::string manu_data = advertisedDevice.getManufacturerData();    bool hasSolar = false;

  if (manu_data.length() < sizeof(VICTRON_BLE_RECORD_BASE)) {    bool hasShunt = false;

    return;    

  }    for (const auto& device : devices) {

        if (device.second.record_type == 0x2E || device.second.record_type == 0x08) {

  const uint8_t* data_ptr = (const uint8_t*)manu_data.data();        hasACCharger = true;

  size_t data_len = manu_data.length();      } else if (device.second.record_type == 0x60 || device.second.record_type == 0x01) {

          hasSolar = true;

  // Parse the advertisement      } else if (device.second.record_type == 0x83 || device.second.record_type == 0x02) {

  if (parse_victron_advertisement(mac_address, data_ptr, data_len, advertisedDevice.getRSSI())) {        hasShunt = true;

    data.valid = true;      }

    Serial.printf("Victron data updated from %s (RSSI: %d)\n",     }

                  mac_address.c_str(), advertisedDevice.getRSSI());    

  }    Serial.printf("Device Summary: SHUNT=%s, SOLAR=%s, AC_CHARGER=%s\n", 

}                  hasShunt ? "YES" : "MISSING", 

                  hasSolar ? "YES" : "MISSING", 

bool VictronBLE::parse_victron_advertisement(const std::string& mac_address,                   hasACCharger ? "YES" : "***MISSING***");

                                             const uint8_t* manufacturer_data,                   

                                             size_t data_len,     if (!hasACCharger) {

                                             int rssi) {      Serial.println("*** WARNING: AC Charger device not detected - should be record type 0x2E or 0x08 ***");

      }

  if (data_len < sizeof(VICTRON_BLE_RECORD_BASE)) {    

    return false;    return true;

  }  } else {

      Serial.println("No Victron advertising packets received");

  // Check manufacturer ID (first 2 bytes in little endian)    return false;

  uint16_t mfr_id = manufacturer_data[0] | (manufacturer_data[1] << 8);  }

  if (mfr_id != VICTRON_MANUFACTURER_ID) {}

    return false;

  }bool VictronBLE::readData() {

    // For advertising approach, we just scan for new packets

  // Parse the base record  return scanForDevices();

  const auto* base_record = reinterpret_cast<const VICTRON_BLE_RECORD_BASE*>(manufacturer_data);}

  

  // Check if this is a product advertisementVictronData VictronBLE::getData() {

  if (base_record->manufacturer_base.manufacturer_record_type !=   // Return the first device found (for backwards compatibility)

      VICTRON_MANUFACTURER_RECORD_TYPE::PRODUCT_ADVERTISEMENT) {  if (!devices.empty()) {

    return false;    return devices.begin()->second;

  }  }

    // Return empty data if no devices

  // Get data counter for duplicate detection  VictronData emptyData;

  uint16_t data_counter = base_record->data_counter_lsb | (base_record->data_counter_msb << 8);  return emptyData;

  }

  // Check for duplicates based on device

  if (mac_address == bmv712_mac && data_counter == last_bmv_counter) {void VictronScanCallback::onResult(BLEAdvertisedDevice advertisedDevice) {

    return false;  String deviceName = String(advertisedDevice.getName().c_str());

  }  String deviceMac = String(advertisedDevice.getAddress().toString().c_str());

  if (mac_address == mppt_mac && data_counter == last_mppt_counter) {  

    return false;  // Show all devices for debugging AC device issue

  }  Serial.printf("[BLE Scan] Device: \"%s\", Address: %s, RSSI: %d", 

  if (mac_address == ip22_mac && data_counter == last_ip22_counter) {                deviceName.c_str(), deviceMac.c_str(), advertisedDevice.getRSSI());

    return false;                

  }  // Check if this might be our missing AC device - expand search criteria

    if (deviceName.indexOf("KARSTEN") >= 0 || deviceName.indexOf("VICTRON") >= 0 || 

  // Get encrypted data      deviceName.indexOf("AC") >= 0 || deviceName.indexOf("MULTI") >= 0 || 

  const uint8_t* encrypted_data = manufacturer_data + sizeof(VICTRON_BLE_RECORD_BASE);      deviceName.indexOf("PHOENIX") >= 0 || deviceName.indexOf("CHARGE") >= 0 ||

  uint8_t encrypted_len = data_len - sizeof(VICTRON_BLE_RECORD_BASE);      deviceMac.indexOf("c7:a2:c2:61:9f:c4") >= 0) {

      Serial.print(" *** POTENTIAL VICTRON DEVICE ***");

  if (encrypted_len == 0) {  }

    return false;  Serial.println();

  }  

    // Debug manufacturer data for potential Victron devices

  // For now, parse unencrypted data if available or log the encrypted data  if (advertisedDevice.haveManufacturerData()) {

  Serial.printf("Victron device %s: Type=0x%02X, Counter=%d, EncryptedLen=%d\n",    std::string manufacturerData = advertisedDevice.getManufacturerData();

                mac_address.c_str(),     if (manufacturerData.length() >= 5) {

                (uint8_t)base_record->record_type,      uint8_t b0 = (uint8_t)manufacturerData[0];

                data_counter,      uint8_t b1 = (uint8_t)manufacturerData[1];

                encrypted_len);      uint8_t b4 = (uint8_t)manufacturerData[4];

        

  // Log encrypted data for debugging      // Look for possible Victron patterns or new record types including 0x2E (AC Charger)

  Serial.print("Encrypted data: ");      if ((b0 == 0xE1 && b1 == 0x02) || b4 == 0x2E) {

  for (int i = 0; i < encrypted_len; i++) {        Serial.printf("*** FOUND POTENTIAL VICTRON PATTERN: %02X %02X ... %02X (record type: 0x%02X) ***\n", 

    Serial.printf("%02X ", encrypted_data[i]);                      b0, b1, b4, b4);

  }      }

  Serial.println();    }

    }

  // Update counters  

  if (mac_address == bmv712_mac) {  // Pass to our handler

    last_bmv_counter = data_counter;  victron_ble_instance->handleAdvertisementData(advertisedDevice);

    data.bmv_last_update = millis();}

  } else if (mac_address == mppt_mac) {

    last_mppt_counter = data_counter;void VictronBLE::handleAdvertisementData(BLEAdvertisedDevice advertisedDevice) {

    data.solar_last_update = millis();  // Check if device has manufacturer data

  } else if (mac_address == ip22_mac) {  if (!advertisedDevice.haveManufacturerData()) {

    last_ip22_counter = data_counter;    return;

    data.ac_last_update = millis();  }

  }  

    std::string manufacturerData = advertisedDevice.getManufacturerData();

  // For demonstration, try to decrypt with zero key (instant readout mode)  

  uint8_t decrypted_data[16] = {0};  // Must have at least the manufacturer ID (2 bytes) + basic header

  std::array<uint8_t, 16> zero_key = {};  if (manufacturerData.length() < sizeof(VictronBLEManufacturerData)) {

      return;

  if (decrypt_victron_data(encrypted_data, encrypted_len, zero_key,   }

                           base_record->data_counter_lsb,   

                           base_record->data_counter_msb,   // Extract manufacturer ID (first 2 bytes, little endian)

                           decrypted_data)) {  uint16_t manufacturerId = (uint8_t)manufacturerData[1] << 8 | (uint8_t)manufacturerData[0];

      

    // Parse decrypted data based on record type  // Get device info once

    switch (base_record->record_type) {  String deviceName = String(advertisedDevice.getName().c_str());

      case VICTRON_BLE_RECORD_TYPE::BATTERY_MONITOR:  String deviceMac = String(advertisedDevice.getAddress().toString().c_str());

        if (encrypted_len >= sizeof(VICTRON_BLE_RECORD_BATTERY_MONITOR)) {  

          parse_battery_monitor_data(reinterpret_cast<const VICTRON_BLE_RECORD_BATTERY_MONITOR*>(decrypted_data));  if (manufacturerId != VICTRON_MANUFACTURER_ID) {

        }    // Log non-Victron devices that might be our AC device

        break;    if (deviceName.indexOf("KARSTEN") >= 0 || deviceName.indexOf("AC") >= 0 || 

                deviceMac.indexOf("c7:a2:c2") >= 0) {

      case VICTRON_BLE_RECORD_TYPE::SOLAR_CHARGER:      Serial.printf("[DEBUG] Non-Victron device with manufacturer data: %s (%s), MfgID: 0x%04X\n", 

        if (encrypted_len >= sizeof(VICTRON_BLE_RECORD_SOLAR_CHARGER)) {                    deviceName.c_str(), deviceMac.c_str(), manufacturerId);

          parse_solar_charger_data(reinterpret_cast<const VICTRON_BLE_RECORD_SOLAR_CHARGER*>(decrypted_data));      Serial.print("Raw manufacturer data: ");

        }      for (size_t i = 0; i < manufacturerData.length() && i < 10; i++) {

        break;        Serial.printf("%02X ", (uint8_t)manufacturerData[i]);

              }

      case VICTRON_BLE_RECORD_TYPE::AC_CHARGER:      Serial.println();

        if (encrypted_len >= sizeof(VICTRON_BLE_RECORD_AC_CHARGER)) {    }

          parse_ac_charger_data(reinterpret_cast<const VICTRON_BLE_RECORD_AC_CHARGER*>(decrypted_data));    return;

        }  }

        break;  

          Serial.println("Found Victron BLE device!");

      default:  Serial.printf("Manufacturer data length: %d bytes\n", manufacturerData.length());

        Serial.printf("Unsupported record type: 0x%02X\n", (uint8_t)base_record->record_type);  

        break;  // Print raw manufacturer data for debugging

    }  Serial.print("Raw manufacturer data: ");

      for (size_t i = 0; i < manufacturerData.length(); i++) {

    return true;    Serial.printf("%02X ", (uint8_t)manufacturerData[i]);

  }  }

    Serial.println();

  return false;  

}  // Skip the manufacturer ID (first 2 bytes) to get to Victron data

  const uint8_t* victronData = (const uint8_t*)manufacturerData.data() + 2;

bool VictronBLE::decrypt_victron_data(const uint8_t* encrypted_data,   size_t victronDataLen = manufacturerData.length() - 2;

                                      uint8_t data_len,  

                                      const std::array<uint8_t, 16>& bindkey,  if (victronDataLen < sizeof(VictronBLEManufacturerData) - 2) { // -2 because we already skipped manufacturer ID

                                      uint8_t counter_lsb,     Serial.println("Victron data too short");

                                      uint8_t counter_msb,    return;

                                      uint8_t* decrypted_data) {  }

    

  esp_aes_context ctx;  const VictronBLEManufacturerData* header = (const VictronBLEManufacturerData*)(victronData - 2); // Include manufacturer ID in struct

  esp_aes_init(&ctx);  

    Serial.printf("Product ID: 0x%04X\n", header->product_id);

  int status = esp_aes_setkey(&ctx, bindkey.data(), bindkey.size() * 8);  Serial.printf("Record type: 0x%02X\n", header->record_type);

  if (status != 0) {  Serial.printf("Data counter: %d\n", header->data_counter_lsb | (header->data_counter_msb << 8));

    esp_aes_free(&ctx);  Serial.printf("Encryption key first byte: 0x%02X\n", header->encryption_key_0);

    return false;  

  }  // Find encryption key (using deviceName and deviceMac already declared above)

    String encryptionKey = findEncryptionKey(deviceName, deviceMac);

  // Create nonce/counter for CTR mode  if (encryptionKey.length() == 0) {

  uint8_t nonce_counter[16] = {counter_lsb, counter_msb, 0};    Serial.println("No encryption key found for this device");

  uint8_t stream_block[16] = {0};    return;

  size_t nc_offset = 0;  }

    

  status = esp_aes_crypt_ctr(&ctx, data_len, &nc_offset, nonce_counter,   Serial.printf("[Victron] Using encryption key for device %s\n", deviceName.c_str());

                             stream_block, encrypted_data, decrypted_data);  

    // Get or create device data entry

  esp_aes_free(&ctx);  VictronData& deviceData = devices[deviceMac];

    

  if (status == 0) {  // Check for duplicate packets (same data counter for this device)

    Serial.print("Decrypted data: ");  uint16_t dataCounter = header->data_counter_lsb | (header->data_counter_msb << 8);

    for (int i = 0; i < data_len; i++) {  // Temporarily disable duplicate checking for testing

      Serial.printf("%02X ", decrypted_data[i]);  // if (dataCounter == deviceData.data_counter) {

    }  //   Serial.println("Duplicate packet, ignoring");

    Serial.println();  //   return;

  }  // }

    deviceData.data_counter = dataCounter;

  return (status == 0);  

}  // Extract encrypted data (after the header)

  size_t headerSize = sizeof(VictronBLEManufacturerData);

void VictronBLE::parse_battery_monitor_data(const VICTRON_BLE_RECORD_BATTERY_MONITOR* record) {  if (victronDataLen <= headerSize - 2) { // -2 for manufacturer ID

  Serial.println("Parsing Battery Monitor data...");    Serial.println("No encrypted data found");

      return;

  // Convert from Victron BLE format to real units  }

  data.time_to_go = record->time_to_go;  

  data.battery_voltage = record->battery_voltage * 0.01f;  // 0.01V units  const uint8_t* encryptedData = victronData + (headerSize - 2); // -2 for manufacturer ID  

  data.alarm_reason = record->alarm_reason;  size_t encryptedLen = victronDataLen - (headerSize - 2);

    

  // Parse auxiliary input based on type  Serial.printf("Encrypted data length: %d bytes\n", encryptedLen);

  data.aux_input_type = record->aux_input_type;  

  switch (record->aux_input_type) {  // Store device info

    case VE_REG_BMV_AUX_INPUT::VE_REG_DC_CHANNEL2_VOLTAGE:  deviceData.device_name = deviceName;

    case VE_REG_BMV_AUX_INPUT::VE_REG_BATTERY_MID_POINT_VOLTAGE:  deviceData.device_mac = deviceMac;

      data.aux_voltage = record->aux_input.aux_voltage * 0.01f;  // 0.01V units  deviceData.encryption_key = encryptionKey;

      break;  deviceData.record_type = header->record_type;

    case VE_REG_BMV_AUX_INPUT::VE_REG_BAT_TEMPERATURE:  deviceData.device_type = getDeviceTypeName(header->record_type);

      // Temperature in Kelvin, convert to Celsius  

      data.aux_voltage = record->aux_input.temperature - 273.15f;  // Try to decrypt the data

      break;  uint8_t decrypted[32] = {0};

    default:  if (decryptVictronData(encryptedData, encryptedLen, encryptionKey, 

      data.aux_voltage = 0.0f;                        header->data_counter_lsb, header->data_counter_msb, 

      break;                        header->encryption_key_0, decrypted)) {

  }    parseDecryptedData(decrypted, encryptedLen, header->record_type, deviceData);

      deviceFound = true;

  // Battery current in 0.001A units, sign-extend 22-bit value    deviceData.data_received = true;

  int32_t current_raw = record->battery_current;    deviceData.last_update = millis();

  if (current_raw & 0x200000) { // Check if sign bit is set    Serial.println("Successfully processed Victron BLE packet!");

    current_raw |= 0xFFC00000;  // Sign extend to 32-bit  } else {

  }    Serial.println("Failed to decrypt Victron data");

  data.battery_current = current_raw * 0.001f;  }

  }

  // Consumed Ah in -0.1Ah units (negative value)

  data.consumed_ah = -(record->consumed_ah * 0.1f);bool VictronBLE::decryptVictronData(const uint8_t* encryptedData, size_t length, 

                                     const String& encryptionKey, 

  // State of charge in 0.1% units                                     uint8_t data_counter_lsb, uint8_t data_counter_msb,

  data.battery_soc = record->state_of_charge * 0.1f;                                   uint8_t encryption_key_0,

                                     uint8_t* decrypted) {

  Serial.printf("BMV: V=%.2fV, I=%.3fA, SOC=%.1f%%, Consumed=%.1fAh, TTG=%dmin\n",  // Convert hex string to bytes

                data.battery_voltage, data.battery_current, data.battery_soc,   uint8_t key[16];

                data.consumed_ah, data.time_to_go);  hexStringToBytes(encryptionKey, key, 16);

}  

  // Check length

void VictronBLE::parse_solar_charger_data(const VICTRON_BLE_RECORD_SOLAR_CHARGER* record) {  if (length > 32) {

  Serial.println("Parsing Solar Charger data...");    Serial.println("Encrypted data too long");

      return false;

  data.solar_state = record->device_state;  }

  data.solar_error = record->charger_error_code;  

  data.solar_voltage = record->battery_voltage * 0.01f;  // 0.01V units (battery voltage)  Serial.print("Raw encrypted data: ");

  data.solar_current = record->battery_current * 0.1f;   // 0.1A units  for (size_t i = 0; i < length; i++) {

  data.yield_today = record->yield_today * 0.01f;        // 0.01kWh units    Serial.printf("%02X ", encryptedData[i]);

  data.solar_power = record->pv_power;                   // 1W units  }

    Serial.println();

  Serial.printf("Solar: State=%d, V=%.2fV, I=%.1fA, P=%dW, Yield=%.2fkWh\n",  

                (int)data.solar_state, data.solar_voltage, data.solar_current,   // Implement proper AES-CTR decryption based on ESPHome research

                (int)data.solar_power, data.yield_today);  mbedtls_aes_context aes_ctx;

}  mbedtls_aes_init(&aes_ctx);

  

void VictronBLE::parse_ac_charger_data(const VICTRON_BLE_RECORD_AC_CHARGER* record) {  // Set encryption key

  Serial.println("Parsing AC Charger data...");  int ret = mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

    if (ret != 0) {

  data.ac_state = record->device_state;    Serial.printf("AES setkey failed: %d\n", ret);

  data.ac_error = record->charger_error_code;    mbedtls_aes_free(&aes_ctx);

  data.ac_voltage = record->battery_voltage * 0.01f;     // 0.01V units (battery voltage)    return false;

  data.ac_current = record->battery_current * 0.1f;     // 0.1A units  }

  data.ac_temperature = record->battery_temperature - 273.15f; // Convert K to °C  

    // Construct nonce based on ESPHome research: {data_counter_lsb, data_counter_msb, 0, 0, 0, ...}

  Serial.printf("AC: State=%d, V=%.2fV, I=%.1fA, Temp=%.1f°C\n",  uint8_t nonce[16] = {0};

                (int)data.ac_state, data.ac_voltage, data.ac_current, data.ac_temperature);  nonce[0] = data_counter_lsb;

}  nonce[1] = data_counter_msb;  

  // Rest of nonce is zeros (not using encryption_key_0)

String VictronBLE::getSolarState() const {  

  switch (data.solar_state) {  Serial.printf("Using nonce: ");

    case VE_REG_DEVICE_STATE::OFF: return "Off";  for (int i = 0; i < 16; i++) {

    case VE_REG_DEVICE_STATE::LOW_POWER: return "Low Power";    Serial.printf("%02X ", nonce[i]);

    case VE_REG_DEVICE_STATE::FAULT: return "Fault";  }

    case VE_REG_DEVICE_STATE::BULK: return "Bulk";  Serial.println();

    case VE_REG_DEVICE_STATE::ABSORPTION: return "Absorption";  

    case VE_REG_DEVICE_STATE::FLOAT: return "Float";  // Counter for CTR mode (starts at 0)

    case VE_REG_DEVICE_STATE::STORAGE: return "Storage";  size_t nc_off = 0;

    case VE_REG_DEVICE_STATE::EQUALIZE: return "Equalize";  uint8_t stream_block[16] = {0};

    default: return "Unknown";  

  }  // Decrypt using AES-CTR

}  ret = mbedtls_aes_crypt_ctr(&aes_ctx, length, &nc_off, nonce, stream_block, encryptedData, decrypted);

  

String VictronBLE::getSolarError() const {  mbedtls_aes_free(&aes_ctx);

  switch (data.solar_error) {  

    case VE_REG_CHR_ERROR_CODE::NO_ERROR: return "No Error";  if (ret != 0) {

    case VE_REG_CHR_ERROR_CODE::BATTERY_VOLTAGE_TOO_HIGH: return "Battery voltage too high";    Serial.printf("AES decryption failed: %d\n", ret);

    case VE_REG_CHR_ERROR_CODE::BATTERY_VOLTAGE_TOO_LOW: return "Battery voltage too low";    return false;

    case VE_REG_CHR_ERROR_CODE::BATTERY_TEMPERATURE_TOO_HIGH: return "Battery temperature too high";  }

    case VE_REG_CHR_ERROR_CODE::INPUT_VOLTAGE_TOO_HIGH: return "Input voltage too high";  

    case VE_REG_CHR_ERROR_CODE::INPUT_CURRENT_TOO_HIGH: return "Input current too high";  Serial.print("Decrypted data: ");

    case VE_REG_CHR_ERROR_CODE::INPUT_POWER_TOO_HIGH: return "Input power too high";  for (size_t i = 0; i < length; i++) {

    default: return "Error " + String((int)data.solar_error);    Serial.printf("%02X ", decrypted[i]);

  }  }

}  Serial.println();

  

String VictronBLE::getACState() const {  return true;

  switch (data.ac_state) {}

    case VE_REG_DEVICE_STATE::OFF: return "Off";

    case VE_REG_DEVICE_STATE::LOW_POWER: return "Low Power";void VictronBLE::parseDecryptedData(const uint8_t* rawData, size_t length, uint8_t advertised_record_type, VictronData& data) {

    case VE_REG_DEVICE_STATE::FAULT: return "Fault";  Serial.printf("Parsing decrypted data, length: %d, advertised record type: 0x%02X\n", length, advertised_record_type);

    case VE_REG_DEVICE_STATE::BULK: return "Bulk";  Serial.printf("Device Name: '%s'\n", data.device_name.c_str());

    case VE_REG_DEVICE_STATE::ABSORPTION: return "Absorption";  

    case VE_REG_DEVICE_STATE::FLOAT: return "Float";  // Print raw decrypted bytes for debugging

    case VE_REG_DEVICE_STATE::STORAGE: return "Storage";  Serial.print("Raw decrypted bytes: ");

    default: return "Unknown";  for (size_t i = 0; i < length; i++) {

  }    Serial.printf("[%d]=0x%02X ", i, rawData[i]);

}  }

  Serial.println();

String VictronBLE::getACError() const {  

  switch (data.ac_error) {  // Identify device type based on name and record type

    case VE_REG_CHR_ERROR_CODE::NO_ERROR: return "No Error";  String deviceType = "UNKNOWN";

    case VE_REG_CHR_ERROR_CODE::BATTERY_VOLTAGE_TOO_HIGH: return "Battery voltage too high";  if (data.device_name.indexOf("SHUNT") >= 0 || advertised_record_type == 0x83) {

    case VE_REG_CHR_ERROR_CODE::BATTERY_VOLTAGE_TOO_LOW: return "Battery voltage too low";    deviceType = "BATTERY_MONITOR";

    case VE_REG_CHR_ERROR_CODE::BATTERY_TEMPERATURE_TOO_HIGH: return "Battery temperature too high";  } else if (data.device_name.indexOf("SOLAR") >= 0 || advertised_record_type == 0x60) {

    default: return "Error " + String((int)data.ac_error);    deviceType = "SOLAR_CHARGER";

  }  } else if (advertised_record_type == 0x2E) {

}    deviceType = "AC_CHARGER";  // This is likely the AC charger!
  }
  
  Serial.printf("=== DEVICE TYPE: %s (Record: 0x%02X) ===\n", deviceType.c_str(), advertised_record_type);
  
  if (advertised_record_type == 0x83) {
    // SHUNT/Battery Monitor - measures NET battery current (all in/out combined)
    Serial.println("=== BATTERY MONITOR/SHUNT (0x83) - NET current measurement ===");
    
    // Look for voltage patterns in little-endian format
    if (length >= 4) {
      Serial.println("Scanning all positions for voltage patterns:");
      
      // Try various positions for voltage (should be ~1350 raw for 13.50V)
      for (int pos = 0; pos <= (int)length - 2; pos++) {
        uint16_t raw_val = (uint16_t)(rawData[pos] | (rawData[pos+1] << 8));
        float voltage = raw_val * 0.01f;
        
        // Log all potential voltage values for analysis
        Serial.printf("Pos %2d: 0x%04X = %.2fV", pos, raw_val, voltage);
        
        // Try alternative scaling factors for voltages outside normal range
        float voltage_alt1 = raw_val / 100.0f;    // Alternative scaling
        float voltage_alt2 = raw_val / 1000.0f;   // Alternative scaling  
        float voltage_alt3 = raw_val * 0.1f;      // Alternative scaling
        
        // Check multiple voltage possibilities
        if (voltage >= 10.0f && voltage <= 16.0f) {
          Serial.printf(" <- VOLTAGE (0.01 scale)!");
          data.battery_voltage = voltage;
        } else if (voltage_alt1 >= 10.0f && voltage_alt1 <= 16.0f) {
          Serial.printf(" <- VOLTAGE (/100 scale: %.2fV)!", voltage_alt1);
          data.battery_voltage = voltage_alt1;
        } else if (voltage_alt2 >= 10.0f && voltage_alt2 <= 16.0f) {
          Serial.printf(" <- VOLTAGE (/1000 scale: %.3fV)!", voltage_alt2);
          data.battery_voltage = voltage_alt2;
        } else if (voltage_alt3 >= 10.0f && voltage_alt3 <= 16.0f) {
          Serial.printf(" <- VOLTAGE (*0.1 scale: %.1fV)!", voltage_alt3);
          data.battery_voltage = voltage_alt3;
        }
        
        if (data.battery_voltage > 0) {
          
          // Try to find current near the voltage
          if (pos + 2 < length) {
            int16_t current_raw = (int16_t)(rawData[pos+2] | (rawData[pos+3] << 8));
            
            // Try different current scaling factors
            float current_001 = current_raw * 0.001f; // 1mA scale
            float current_01 = current_raw * 0.01f;   // 10mA scale  
            float current_1 = current_raw * 0.1f;     // 100mA scale
            
            Serial.printf("\n    Current at pos %d: raw=0x%04X, scales: %.3fA/%.2fA/%.1fA", 
                         pos+2, current_raw, current_001, current_01, current_1);
            
            // Pick the most reasonable current value
            if (abs(current_001) <= 2.0f) {
              data.battery_current = current_001;
              Serial.printf(" -> Using 0.001A scale: %.3fA", current_001);
            } else if (abs(current_01) <= 10.0f) {
              data.battery_current = current_01;
              Serial.printf(" -> Using 0.01A scale: %.2fA", current_01);
            } else if (abs(current_1) <= 50.0f) {
              data.battery_current = current_1;
              Serial.printf(" -> Using 0.1A scale: %.1fA", current_1);
            }
          }
        }
        Serial.println();
      }
    }
    
  } else if (advertised_record_type == 0x60) {
    // SOLAR device - should show 0A if not charging
    Serial.println("=== SOLAR CHARGER (0x60) - Should be 0A if not charging ===");
    
    if (length >= 4) {
      Serial.println("Scanning SOLAR device for voltage patterns:");
      
      // Try various positions for voltage
      for (int pos = 0; pos <= (int)length - 2; pos++) {
        uint16_t raw_val = (uint16_t)(rawData[pos] | (rawData[pos+1] << 8));
        float voltage = raw_val * 0.01f;
        
        Serial.printf("Pos %2d: 0x%04X = %.2fV", pos, raw_val, voltage);
        
        // Try alternative scaling factors
        float voltage_alt1 = raw_val / 100.0f;    // Alternative scaling
        float voltage_alt2 = raw_val / 1000.0f;   // Alternative scaling  
        float voltage_alt3 = raw_val * 0.1f;      // Alternative scaling
        
        if (voltage >= 10.0f && voltage <= 16.0f) {
          Serial.printf(" <- VOLTAGE (0.01 scale)!");
          data.battery_voltage = voltage;
        } else if (voltage_alt1 >= 10.0f && voltage_alt1 <= 16.0f) {
          Serial.printf(" <- VOLTAGE (/100 scale: %.2fV)!", voltage_alt1);
          data.battery_voltage = voltage_alt1;
        } else if (voltage_alt2 >= 10.0f && voltage_alt2 <= 16.0f) {
          Serial.printf(" <- VOLTAGE (/1000 scale: %.3fV)!", voltage_alt2);
          data.battery_voltage = voltage_alt2;
        } else if (voltage_alt3 >= 10.0f && voltage_alt3 <= 16.0f) {
          Serial.printf(" <- VOLTAGE (*0.1 scale: %.1fV)!", voltage_alt3);
          data.battery_voltage = voltage_alt3;
        }
        
        if (data.battery_voltage > 0) {
          
          // Look for current and power
          if (pos + 2 < length) {
            int16_t current_raw = (int16_t)(rawData[pos+2] | (rawData[pos+3] << 8));
            float current_1 = current_raw * 0.1f;
            float current_01 = current_raw * 0.01f;
            
            Serial.printf("\n    Current at pos %d: raw=0x%04X, 0.1A=%.1fA, 0.01A=%.2fA", 
                         pos+2, current_raw, current_1, current_01);
            
            if (abs(current_1) <= 10.0f) {
              data.battery_current = current_1;
              Serial.printf(" -> Using %.1fA", current_1);
            } else if (abs(current_01) <= 2.0f) {
              data.battery_current = current_01;
              Serial.printf(" -> Using %.2fA", current_01);
            }
          }
          
          // Look for power data
          if (pos + 4 < length) {
            uint16_t power_raw = (uint16_t)(rawData[pos+4] | (rawData[pos+5] << 8));
            Serial.printf("\n    Power at pos %d: raw=0x%04X = %dW", pos+4, power_raw, power_raw);
            if (power_raw <= 1000) { // Reasonable power range
              data.pv_power = power_raw;
              Serial.printf(" -> Using %dW", power_raw);
            }
          }
        }
        Serial.println();
      }
    }
    
  } else if (advertised_record_type == 0x2E) {
    // AC CHARGER - This is the key device you're looking for!
    Serial.println("=== AC CHARGER (0x2E) - This should show AC charging! ===");
    
    if (length >= 14) {
      Serial.println("Sufficient data - scanning for AC charger patterns:");
      
      // Scan all positions for realistic values
      for (int pos = 0; pos <= (int)length - 2; pos++) {
        uint16_t raw_val = (uint16_t)(rawData[pos] | (rawData[pos+1] << 8));
        
        // Try different voltage scaling factors
        float voltage_01 = raw_val * 0.01f;      // Standard scaling
        float voltage_100 = raw_val / 100.0f;    // /100 scaling
        float voltage_1000 = raw_val / 1000.0f;  // /1000 scaling
        
        Serial.printf("Pos %2d: 0x%04X = 0.01:%.2fV, /100:%.2fV, /1000:%.3fV", 
                     pos, raw_val, voltage_01, voltage_100, voltage_1000);
        
        // Check for realistic battery voltage
        if (voltage_01 >= 10.0f && voltage_01 <= 20.0f) {
          Serial.printf(" <- AC VOLTAGE (0.01 scale)!");
          data.battery_voltage = voltage_01;
        } else if (voltage_100 >= 10.0f && voltage_100 <= 20.0f) {
          Serial.printf(" <- AC VOLTAGE (/100 scale)!");
          data.battery_voltage = voltage_100;
        } else if (voltage_1000 >= 10.0f && voltage_1000 <= 20.0f) {
          Serial.printf(" <- AC VOLTAGE (/1000 scale)!");
          data.battery_voltage = voltage_1000;
        }
        
        // If we found voltage, look for current nearby
        if (data.battery_voltage > 0 && pos + 2 < length) {
          uint16_t curr_raw = (uint16_t)(rawData[pos+2] | (rawData[pos+3] << 8));
          int16_t curr_signed = (int16_t)curr_raw;
          
          // Try different current scaling factors
          float current_01 = curr_signed * 0.01f;   // 10mA scale
          float current_001 = curr_signed * 0.001f; // 1mA scale
          float current_1 = curr_signed * 0.1f;     // 100mA scale
          
          Serial.printf("\n    AC Current at pos %d: raw=0x%04X, 0.01A=%.2fA, 0.001A=%.3fA, 0.1A=%.1fA", 
                       pos+2, curr_raw, current_01, current_001, current_1);
          
          // Look for positive charging current (AC should be charging)
          if (current_01 > 0.1f && current_01 <= 50.0f) {
            Serial.printf(" -> AC CHARGING: %.2fA", current_01);
            data.battery_current = current_01;
          } else if (current_001 > 0.1f && current_001 <= 50.0f) {
            Serial.printf(" -> AC CHARGING: %.3fA", current_001);
            data.battery_current = current_001;
          } else if (current_1 > 0.1f && current_1 <= 50.0f) {
            Serial.printf(" -> AC CHARGING: %.1fA", current_1);
            data.battery_current = current_1;
          }
        }
        Serial.println();
      }
    } else {
      Serial.printf("AC Charger data short: %d bytes, analyzing anyway...\n", length);
      
      // Even with short data, try to extract something
      for (int i = 0; i < length - 1; i++) {
        uint16_t raw_val = (uint16_t)(rawData[i] | (rawData[i+1] << 8));
        float voltage = raw_val / 1000.0f;
        Serial.printf("Short[%d]: 0x%04X = %.3fV\n", i, raw_val, voltage);
        
        if (voltage >= 10.0f && voltage <= 20.0f) {
          Serial.printf("  -> Possible AC voltage: %.3fV\n", voltage);
          data.battery_voltage = voltage;
        }
      }
    }
    
  } else {
    // Try the original hoberman struct for standard Solar Charger (0x01)
    Serial.println("=== Using hoberman VictronPanelData structure ===");
    
    if (length < sizeof(VictronPanelData)) {
      Serial.printf("Decrypted data too short: %d bytes, need at least %d\n", length, sizeof(VictronPanelData));
      return;
    }
    
    const VictronPanelData* victronData = (const VictronPanelData*)rawData;
    
    // Data corruption check
    uint8_t unusedBits = victronData->outputCurrentHi & 0xfe;
    if (unusedBits != 0xfe) {
      Serial.printf("Data corruption detected: unusedBits=0x%02X (expected 0xfe)\n", unusedBits);
      return;
    }
    
    // Apply scaling factors
    data.battery_voltage = float(victronData->batteryVoltage) * 0.01f;
    data.battery_current = float(victronData->batteryCurrent) * 0.1f;
    data.pv_power = victronData->inputPower;
    data.yield_today = float(victronData->todayYield) * 0.01f * 1000.0f;
  }
  
  // Final results
  Serial.printf("=== Parsed Results ===\n");
  Serial.printf("Battery Voltage: %.2fV\n", data.battery_voltage);
  Serial.printf("Battery Current: %.3fA\n", data.battery_current);
  Serial.printf("PV Power: %.0fW\n", data.pv_power);
  Serial.printf("Yield Today: %.0fWh\n", data.yield_today);
  Serial.println("======================");
}

String VictronBLE::getDeviceTypeName(uint8_t record_type) {
  switch (record_type) {
    case 0x01: return "Solar Charger";         // SOLAR_CHARGER
    case 0x02: return "Battery Monitor";       // BATTERY_MONITOR (SmartShunt)
    case 0x03: return "Inverter";             // INVERTER
    case 0x04: return "DC-DC Converter";      // DCDC_CONVERTER  
    case 0x05: return "Smart Lithium";        // SMART_LITHIUM
    case 0x06: return "Inverter RS";          // INVERTER_RS
    case 0x08: return "AC Charger";           // AC_CHARGER
    case 0x09: return "Smart Battery Protect"; // SMART_BATTERY_PROTECT
    case 0x0A: return "Lynx Smart BMS";       // LYNX_SMART_BMS
    case 0x0B: return "Multi RS";             // MULTI_RS
    case 0x0C: return "VE.Bus";               // VE_BUS
    case 0x0D: return "DC Energy Meter";      // DC_ENERGY_METER
    case 0x0F: return "Orion XS";             // ORION_XS
    case 0x2E: return "AC Charger 0x2E";      // AC_CHARGER (alternative record type)
    case 0x60: return "Solar Charger 0x60";   // SOLAR_CHARGER (seen in data)
    case 0x83: return "Battery Monitor 0x83"; // BATTERY_MONITOR (seen in data) 
    default: return String("Unknown Device 0x") + String(record_type, HEX);
  }
}

void VictronBLE::printAllDevices() {
  Serial.println("\n=== All Victron Devices ===");
  for (auto& device_pair : devices) {
    VictronData& device = device_pair.second;
    Serial.printf("Device: %s (%s)\n", device.device_name.c_str(), device.device_mac.c_str());
    Serial.printf("  Type: %s (Record: 0x%02X)\n", device.device_type.c_str(), device.record_type);
    Serial.printf("  Battery Voltage: %.2f V\n", device.battery_voltage);
    Serial.printf("  Battery Current: %.1f A\n", device.battery_current);
    Serial.printf("  State of Charge: %d%%\n", device.state_of_charge);
    Serial.printf("  Last Update: %lu ms ago\n", millis() - device.last_update);
    Serial.println();
  }
}