#ifndef VICTRON_BLE_H// victron_ble.h - Victron BLE Advertising Protocol Implementation

#define VICTRON_BLE_H#ifndef VICTRON_BLE_H

#define VICTRON_BLE_H

#include <Arduino.h>

#include "BLEDevice.h"#include <Arduino.h>

#include "BLEScan.h"#include <BLEDevice.h>

#include "BLEAdvertisedDevice.h"#include <BLEUtils.h>

#include <array>#include <BLEScan.h>

#include <BLEAdvertisedDevice.h>

// Victron BLE constants from HACS integration#include <map>

static const uint16_t VICTRON_MANUFACTURER_ID = 0x02E1;#include "mbedtls/aes.h"



enum class VICTRON_MANUFACTURER_RECORD_TYPE : uint8_t {// Victron Energy manufacturer ID

  PRODUCT_ADVERTISEMENT = 0x10,#define VICTRON_MANUFACTURER_ID    0x02E1

};#define VICTRON_SCAN_TIME          5   // seconds



enum class VICTRON_PRODUCT_ID : uint16_t {struct VictronData {

  BMV_712_SMART = 0xA381,  float battery_voltage = 0.0;

  SMARTSHUNT_500A_50MV = 0xA389,  float battery_current = 0.0; 

  SmartSolar_MPPT_100_30_REV3 = 0xA076,  float consumed_ah = 0.0;

  SmartSolar_MPPT_100_50_REV3 = 0xA077,  int state_of_charge = 0;

  PHOENIX_INVERTER_SMART_IP43_CHARGER_12_50 = 0xA340,  int time_to_go = 0;

};  float instantaneous_power = 0.0;

  float pv_power = 0.0;  // For solar charger

enum class VICTRON_BLE_RECORD_TYPE : uint8_t {  float yield_today = 0.0; // For solar charger

  SOLAR_CHARGER = 0x01,  bool data_received = false;

  BATTERY_MONITOR = 0x02,  unsigned long last_update = 0;

  INVERTER = 0x03,  String device_name = "";

  DCDC_CONVERTER = 0x04,  String device_mac = "";

  SMART_LITHIUM = 0x05,  String encryption_key = "";

  INVERTER_RS = 0x06,  String device_type = "";

  AC_CHARGER = 0x08,  uint8_t record_type = 0;

  SMART_BATTERY_PROTECT = 0x09,  uint16_t data_counter = 0;

  LYNX_SMART_BMS = 0x0A,};

  MULTI_RS = 0x0B,

  VE_BUS = 0x0C,// Victron BLE manufacturer data structure (based on their protocol)

  DC_ENERGY_METER = 0x0D,struct VictronBLEManufacturerData {

  ORION_XS = 0x0F,  uint8_t manufacturer_record_type;    // 0x10 for product advertisement

};  uint8_t manufacturer_record_length;  // Length of manufacturer data

  uint16_t product_id;                 // Product identifier

enum class VE_REG_ALARM_REASON : uint16_t {  uint8_t record_type;                 // Data record type (battery monitor, solar charger, etc.)

  NO_ALARM = 0x00,  uint8_t data_counter_lsb;            // Data counter LSB 

  LOW_VOLTAGE = 0x01,  uint8_t data_counter_msb;            // Data counter MSB

  HIGH_VOLTAGE = 0x02,  uint8_t encryption_key_0;            // First byte of encryption key

  LOW_SOC = 0x04,  // Encrypted data follows after this header...

  LOW_STARTER_VOLTAGE = 0x08,} __attribute__((packed));

  HIGH_STARTER_VOLTAGE = 0x10,

  LOW_TEMPERATURE = 0x20,// Decrypted data structure (based on working hoberman implementation)

  HIGH_TEMPERATURE = 0x40,// This matches the layout of the decrypted payload data

  MID_VOLTAGE = 0x80,struct VictronPanelData {

  OVERLOAD = 0x100,  uint8_t deviceState;                 // Charger state (0=off, 3=bulk, 4=absorption, 5=float, 7=equalization)

  DC_RIPPLE = 0x200,  uint8_t errorCode;                   // Error code

  LOW_V_AC_OUT = 0x400,  int16_t batteryVoltage;              // Battery voltage in 0.01V increments (multiply by 0.01 for volts)

  HIGH_V_AC_OUT = 0x800,  int16_t batteryCurrent;              // Battery current in 0.1A increments (multiply by 0.1 for amps)

  SHORT_CIRCUIT = 0x1000,  uint16_t todayYield;                 // Today's energy yield in 0.01kWh increments

  BMS_LOCKOUT = 0x2000,  uint16_t inputPower;                 // PV input power in watts

};  uint8_t outputCurrentLo;             // Low 8 bits of output current (load current in 0.1A increments)

  uint8_t outputCurrentHi;             // High 1 bit of output current (must mask off unused bits: & 0x01)

enum class VE_REG_BMV_AUX_INPUT : uint8_t {  uint8_t unused[4];                   // Unused padding bytes

  VE_REG_DC_CHANNEL2_VOLTAGE = 0x0,} __attribute__((packed));

  VE_REG_BATTERY_MID_POINT_VOLTAGE = 0x1,

  VE_REG_BAT_TEMPERATURE = 0x2,// BLE scan callback class

  NONE = 0x3,class VictronScanCallback : public BLEAdvertisedDeviceCallbacks {

};private:

  class VictronBLE* victron_ble_instance;

enum class VE_REG_CHR_ERROR_CODE : uint8_t {public:

  NO_ERROR = 0,  VictronScanCallback(class VictronBLE* instance) : victron_ble_instance(instance) {}

  BATTERY_TEMPERATURE_TOO_HIGH = 2,  void onResult(BLEAdvertisedDevice advertisedDevice) override;

  BATTERY_VOLTAGE_TOO_HIGH = 3,};

  BATTERY_TEMPERATURE_SENSOR_MISWIRED = 5,

  BATTERY_TEMPERATURE_SENSOR_MISSING = 6,class VictronBLE {

  BATTERY_VOLTAGE_TOO_LOW = 7,private:

  BATTERY_RIPPLE_VOLTAGE_TOO_HIGH = 8,  BLEScan* pBLEScan;

  BATTERY_LOW_SOC = 11,  std::map<String, VictronData> devices;  // Store multiple devices by MAC address

  INPUT_VOLTAGE_TOO_HIGH = 33,  String targetAddress;

  INPUT_CURRENT_TOO_HIGH = 34,  bool deviceFound;

  INPUT_POWER_TOO_HIGH = 35,

  PV_INPUT_SHUTDOWN = 38,public:

  PV_INPUT_SHUTDOWN_DUE_TO_BATTERY_OVER_VOLTAGE = 39,  bool init();

  PV_INPUT_SHUTDOWN_DUE_TO_BATTERY_LOW_SOC = 65,  bool scanForDevices();

  PV_INPUT_SHUTDOWN_DUE_TO_REMOTE_INPUT = 66,  bool readData();  // This will scan for new advertising packets

  PV_INPUT_SHUTDOWN_DUE_TO_BMS = 67,  VictronData getData(); // Returns first available device data (backwards compatibility)

  NETWORK_MISCONFIGURED = 114,  std::map<String, VictronData>& getAllDevices() { return devices; } // Return all devices

  FACTORY_CALIBRATION_DATA_LOST = 116,  String findEncryptionKey(String deviceName, String macAddress);

  INVALID_INCOMPATIBLE_FIRMWARE = 117,  void handleAdvertisementData(BLEAdvertisedDevice advertisedDevice);

  USER_SETTINGS_INVALID = 119,  bool decryptVictronData(const uint8_t* encryptedData, size_t length, const String& encryptionKey, uint8_t data_counter_lsb, uint8_t data_counter_msb, uint8_t encryption_key_0, uint8_t* decrypted);

};  void parseDecryptedData(const uint8_t* rawData, size_t length, uint8_t record_type, VictronData& data);

  String getDeviceTypeName(uint8_t record_type);

enum class VE_REG_DEVICE_STATE : uint8_t {  void printAllDevices();

  OFF = 0,  

  LOW_POWER = 1,  // AES decryption helper

  FAULT = 2,  void hexStringToBytes(const String& hex, uint8_t* bytes, size_t maxLen);

  BULK = 3,};

  ABSORPTION = 4,

  FLOAT = 5,#endif
  STORAGE = 6,
  EQUALIZE = 7,
  PASSTHRU = 8,
  INVERTING = 9,
  POWER_ASSIST = 10,
  POWER_SUPPLY = 11,
  BULK_PROTECTION = 252,
};

struct __attribute__((packed)) VICTRON_BLE_MANUFACTURER_DATA {
  VICTRON_MANUFACTURER_RECORD_TYPE manufacturer_record_type;
  uint8_t manufacturer_record_length;
  VICTRON_PRODUCT_ID product_id;
};

struct __attribute__((packed)) VICTRON_BLE_RECORD_BASE {
  VICTRON_BLE_MANUFACTURER_DATA manufacturer_base;
  VICTRON_BLE_RECORD_TYPE record_type;
  uint8_t data_counter_lsb;
  uint8_t data_counter_msb;
  uint8_t encryption_key_0;
};

struct __attribute__((packed)) VICTRON_BLE_RECORD_BATTERY_MONITOR {
  uint16_t time_to_go;         // Minutes
  int16_t battery_voltage;     // 0.01V
  VE_REG_ALARM_REASON alarm_reason;
  union {
    int16_t aux_voltage;       // 0.01V
    int16_t mid_voltage;       // 0.01V  
    uint16_t temperature;      // K
  } aux_input;
  VE_REG_BMV_AUX_INPUT aux_input_type : 2;
  int32_t battery_current : 22;    // 0.001A
  uint32_t consumed_ah : 20;       // -0.1Ah (negative consumed)
  uint16_t state_of_charge : 10;   // 0.1%
};

struct __attribute__((packed)) VICTRON_BLE_RECORD_SOLAR_CHARGER {
  VE_REG_DEVICE_STATE device_state;
  VE_REG_CHR_ERROR_CODE charger_error_code;
  int16_t battery_voltage;     // 0.01V
  int16_t battery_current;     // 0.1A
  uint16_t yield_today;        // 0.01kWh
  uint16_t pv_power;           // 1W
  uint16_t load_current : 9;   // 0.1A (negative)
};

struct __attribute__((packed)) VICTRON_BLE_RECORD_AC_CHARGER {
  VE_REG_DEVICE_STATE device_state;
  VE_REG_CHR_ERROR_CODE charger_error_code;
  int16_t battery_voltage;     // 0.01V
  int16_t battery_current;     // 0.1A
  uint16_t battery_temperature; // K
};

struct VictronData {
  // Battery Monitor Data
  float battery_voltage = 0.0f;    // V
  float battery_current = 0.0f;    // A
  float battery_soc = 0.0f;        // %
  float consumed_ah = 0.0f;        // Ah
  float aux_voltage = 0.0f;        // V
  uint16_t time_to_go = 0;         // minutes
  VE_REG_ALARM_REASON alarm_reason = VE_REG_ALARM_REASON::NO_ALARM;
  VE_REG_BMV_AUX_INPUT aux_input_type = VE_REG_BMV_AUX_INPUT::NONE;
  unsigned long bmv_last_update = 0;
  
  // Solar Charger Data
  float solar_voltage = 0.0f;      // V (calculated from battery_voltage)
  float solar_current = 0.0f;      // A
  float solar_power = 0.0f;        // W
  float yield_today = 0.0f;        // kWh
  VE_REG_DEVICE_STATE solar_state = VE_REG_DEVICE_STATE::OFF;
  VE_REG_CHR_ERROR_CODE solar_error = VE_REG_CHR_ERROR_CODE::NO_ERROR;
  unsigned long solar_last_update = 0;
  
  // AC Charger Data  
  float ac_voltage = 0.0f;         // V (calculated from battery_voltage)
  float ac_current = 0.0f;         // A
  float ac_temperature = 0.0f;     // °C
  VE_REG_DEVICE_STATE ac_state = VE_REG_DEVICE_STATE::OFF;
  VE_REG_CHR_ERROR_CODE ac_error = VE_REG_CHR_ERROR_CODE::NO_ERROR;
  unsigned long ac_last_update = 0;
  
  // General
  bool valid = false;
};

class VictronBLE : public BLEAdvertisedDeviceCallbacks {
private:
  VictronData data;
  bool ble_initialized = false;
  BLEScan* pBLEScan = nullptr;
  
  // Known device MAC addresses from config
  const char* bmv712_mac = "c0:3b:98:39:e6:fe";
  const char* mppt_mac = "e8:86:01:5d:79:38";  
  const char* ip22_mac = "c7:a2:c2:61:9f:c4";
  
  // AES encryption keys (would need to be configured per device)
  std::array<uint8_t, 16> bmv712_bindkey = {};
  std::array<uint8_t, 16> mppt_bindkey = {};  
  std::array<uint8_t, 16> ip22_bindkey = {};
  
  // Data counters for duplicate detection
  uint16_t last_bmv_counter = 0;
  uint16_t last_mppt_counter = 0;
  uint16_t last_ip22_counter = 0;
  
  bool parse_victron_advertisement(const std::string& mac_address, 
                                   const uint8_t* manufacturer_data, 
                                   size_t data_len, 
                                   int rssi);
  bool decrypt_victron_data(const uint8_t* encrypted_data, 
                            uint8_t data_len,
                            const std::array<uint8_t, 16>& bindkey,
                            uint8_t counter_lsb, 
                            uint8_t counter_msb,
                            uint8_t* decrypted_data);
  void parse_battery_monitor_data(const VICTRON_BLE_RECORD_BATTERY_MONITOR* record);
  void parse_solar_charger_data(const VICTRON_BLE_RECORD_SOLAR_CHARGER* record);
  void parse_ac_charger_data(const VICTRON_BLE_RECORD_AC_CHARGER* record);
  
public:
  VictronBLE();
  ~VictronBLE();
  
  bool setup();
  void startScan();
  void stopScan();
  
  // BLE callback
  void onResult(BLEAdvertisedDevice advertisedDevice) override;
  
  // Data access methods
  const VictronData& getData() const { return data; }
  
  // Battery Monitor
  float getBatteryVoltage() const { return data.battery_voltage; }
  float getBatteryCurrent() const { return data.battery_current; }
  float getBatterySOC() const { return data.battery_soc; }
  float getConsumedAh() const { return data.consumed_ah; }
  float getAuxVoltage() const { return data.aux_voltage; }
  uint16_t getTimeToGo() const { return data.time_to_go; }
  bool hasLowVoltageAlarm() const { return (uint16_t)data.alarm_reason & (uint16_t)VE_REG_ALARM_REASON::LOW_VOLTAGE; }
  bool hasHighVoltageAlarm() const { return (uint16_t)data.alarm_reason & (uint16_t)VE_REG_ALARM_REASON::HIGH_VOLTAGE; }
  bool hasLowSOCAlarm() const { return (uint16_t)data.alarm_reason & (uint16_t)VE_REG_ALARM_REASON::LOW_SOC; }
  
  // Solar Charger
  float getSolarVoltage() const { return data.solar_voltage; }
  float getSolarCurrent() const { return data.solar_current; }  
  float getSolarPower() const { return data.solar_power; }
  float getYieldToday() const { return data.yield_today; }
  String getSolarState() const;
  String getSolarError() const;
  
  // AC Charger
  float getACVoltage() const { return data.ac_voltage; }
  float getACCurrent() const { return data.ac_current; }
  float getACTemperature() const { return data.ac_temperature; }
  String getACState() const;
  String getACError() const;
  
  // Device status
  bool isBMVOnline() const { return (millis() - data.bmv_last_update) < 120000; }
  bool isSolarOnline() const { return (millis() - data.solar_last_update) < 120000; }
  bool isACOnline() const { return (millis() - data.ac_last_update) < 120000; }
  bool isValid() const { return data.valid; }
};

#endif