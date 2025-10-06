#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Do not include BLE headers here; implementation file includes them.
// AES/CTR will be performed with mbedtls from the ESP32 IDF (mbedtls/aes.h)

// Victron BLE Constants
#define VICTRON_MFG_ID 0x02E1

// Target Victron device MAC addresses and encryption keys (CORRECT KEYS)
#define BMV712_MAC "C0:3B:98:39:E6:FE"    // KARSTEN MAXI SHUNT
#define BMV712_KEY "6cb52976b1b82ab4d6bc4d24ee356c1b"

#define MPPT_MAC   "E8:86:01:5D:79:38"    // KARSTEN MAXI SOLAR  
#define MPPT_KEY   "7f8689f768ae1cb7018411538ae5fa85"

#define IP22_MAC   "C7:A2:C2:61:9F:C4"    // KARSTEN MAXI AC
#define IP22_KEY   "c9014b769559cb6fdf5a8a1361edf68a"

// Main Display ESP32 MAC address (you'll need to set this)
extern uint8_t mainDisplayMAC[];

// BitReader class for parsing bit streams (matching victron_ble Python library)
class BitReader {
private:
  const uint8_t* _data;
  size_t _index;
  size_t _totalBits;

public:
  BitReader(const uint8_t* data, size_t dataLen) : _data(data), _index(0), _totalBits(dataLen * 8) {}

  int readBit() {
    int bit = (_data[_index >> 3] >> (_index & 7)) & 1;
    _index++;
    return bit;
  }

  bool canRead(int numBits) const {
    return (_index + numBits) <= _totalBits;
  }

  uint32_t readUnsignedInt(int numBits) {
    uint32_t value = 0;
    for (int position = 0; position < numBits; position++) {
      value |= (readBit() << position);
    }
    return value;
  }

  int32_t readSignedInt(int numBits) {
    uint32_t unsignedValue = readUnsignedInt(numBits);
    return toSignedInt(unsignedValue, numBits);
  }

  static int32_t toSignedInt(uint32_t value, int numBits) {
    return (value & (1 << (numBits - 1))) ? value - (1 << numBits) : value;
  }
};

// Data structure to send over ESP-NOW
struct VictronData {
    uint8_t deviceType;  // 1=BMV712, 2=MPPT, 3=IP22
    char deviceMAC[18];
    float voltage;
    float current;
    float power;
    float soc;
    int rssi;
    unsigned long timestamp;
};

class VictronBLE {
private:
  void* pBLEScan; // opaque pointer to avoid BLE headers in this public header
    bool bleInitialized = false;
  bool espNowPeerAdded = false;
    
    // ESP-NOW functions
    void initESPNOW();
    void sendData(const VictronData& data);
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    
    // Victron parsing functions (based on your ESPHome/HACS working setup)
  void parseVictronAdvertisement(const String& address, const std::string& manufacturerData, int rssi);
    uint8_t getDeviceType(const String& address);
    void parseInstantReadout(const uint8_t* data, size_t len, const String& address, VictronData& victronData);
    
    // AES decryption functions
    String getEncryptionKey(const String& address);
    bool decryptVictronData(const uint8_t* encryptedData, size_t dataLen, const String& key, uint16_t iv, uint8_t* decrypted);
    
    // BitReader-based parsing functions (matching victron_ble Python library)
    void parseBitStream(const uint8_t* data, size_t dataLen, uint16_t model_id, uint8_t mode, VictronData& victronData);
    void parseBatteryMonitor(const uint8_t* data, size_t dataLen, VictronData& victronData);
    void parseSolarCharger(const uint8_t* data, size_t dataLen, VictronData& victronData);
    void parseAcCharger(const uint8_t* data, size_t dataLen, VictronData& victronData);

public:
    VictronBLE();
    bool begin();
    void loop();
  // BLE callback is implemented in the .cpp to avoid BLE headers in this public header
  // The implementation will call parseVictronAdvertisement(...) when an advertised device is received.
};