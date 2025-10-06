# Extra Manufacturer Data

**Last update:** 2022/12/14 13:25  
**Source:** Victron Energy - https://wiki.victronenergy.com/

---

## Overview

This page describes the data format of the extra manufacturer data that can be added to the Victron Manufacturer Data record type 0x10 - Product Advertisement.

The block of extra data is called a record and it starts with the following data:

| Start bit | Nr of bits | Meaning |
|-----------|------------|---------|
| 0 | 8 | Record type |
| 8 | 16 | Nonce/Data counter in LSB order |
| 24 | 8 | Byte 0 of the encryption key |

## Record Types

| Value | Record type |
|-------|-------------|
| 0x00 | Test record |
| 0x01 | Solar Charger |
| 0x02 | Battery Monitor |
| 0x03 | Inverter |
| 0x04 | DC/DC converter |
| 0x05 | SmartLithium |
| 0x06 | Inverter RS |
| 0x07 | GX-Device (Record layout TBD) |
| 0x08 | AC Charger (Record layout TBD) |
| 0x09 | Smart Battery Protect |
| 0x0A | (Lynx Smart) BMS |
| 0x0B | Multi RS |
| 0x0C | VE.Bus |
| 0x0D | DC Energy Meter |
| 0x0E..0xFF | Unassigned / Reserved for future extensions (Possibly with a second type byte) |

## Encryption Details

The nonce/data counter in the header is used in the encryption to make the result of the encryption a little more random. This way, somebody that does not have the encryption key has to wait almost a day (assuming a change every second) for the same nonce value before more information can be extracted. What can be extracted in that case is only which bits have changed, but not the final value.

For some things the exact value might be deducible after 1 message. For example, the MPPT state is likely to be off in the middle of the night. This would allow someone to determine the exact value of the state when the same nonce is received at a later moment. It however does not give any information on the encryption key or other fields of the record.

The first byte of the encryption key is included in the message. This can be used by the receiving side to check if the key that they have matches the key that was used for the encoding. When this byte does not match the key as stored on the receiving device, then the stored key should be discarded and the device should stop trying to decode the advertisement. 

The advertising device should change the key when the bluetooth pin code is changed. This keeps the knowledge of the correct key in sync with phones that are also bonded meaning that a phone that can connect can also decode the advertisement messages. The advertising device should limit the chance of re-using a value for the first byte, for example by simply incrementing the first byte when generating a new key instead of taking a random value. When a key is not stored, a complete random key should be generated.

### AES-CTR Encryption

The encryption is done using an AES-CTR operation ([AES-CTR on Wikipedia](https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#Counter_(CTR)))

In the case of the extra advertisement data, the nonce is 2 bytes wide and contains the nonce bytes of the record header. The counter field of the AES-CTR operation is a MSB counter that starts at 0. Records that contain 16 bytes of data or less can be encrypted/decrypted using 1 AES operation. At the moment, this is true for all records.

The maximum length of a record is 20 bytes for the nRF devices. This maybe less for a GX-Device (TBD!!).

### Record Layout Rules

The layout of a record is never changed, it can however be extended in the future. This means that an implementation should not perform an upper bound check on the length. As the length is not fixed and items can be added, a length check should be performed for all items when retrieving data from the record.

Unused bits in the last byte of a record should be set. This also means that when a record is extended, care should be taken with the NA value, such that when the new field does not cross a byte boundary, the receiver should be able to use the NA value to determine if the field is actually set or not.

---

## Test Record

This is used by software test applications.

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 30 | Uptime | 1 s | 0 .. 34 year | 0x3FFFFFFF | VE_REG_UPTIME |
| 62 | 7 | Temperature | 1 °C | -40 .. 86 °C | 0xFF | VE_REG_BAT_TEMPERATURE<br>Temperature = Record value - 40 |
| 69 | 91 | Unused | | | | |

---

## Solar Charger

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 40 | 8 | Charger Error | | 0 .. 0xFE | 0xFF | VE_REG_CHR_ERROR_CODE |
| 48 | 16 | Battery voltage | 0.01 V | -327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 64 | 16 | Battery current | 0.1A | -3276.8 .. 3276.6 A | 0x7FFF | VE_REG_DC_CHANNEL1_CURRENT<br>Also negative current because of a possibly connected load |
| 80 | 16 | Yield today | 0.01 kWh | 0 .. 655.34 kWh | 0xFFFF | VE_REG_CHR_TODAY_YIELD<br>655.34 kWh is 27.3 kW@24h |
| 96 | 16 | PV power | 1 W | 0 .. 65534 W | 0xFFFF | VE_REG_DC_INPUT_POWER (un32 @ 0.01W) |
| 112 | 9 | Load current | 0.1 A | 0 .. 51.0 A | 0x1FF | VE_REG_DC_OUTPUT_CURRENT |
| 121 | 39 | Unused | | | | |

---

## Battery Monitor

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 16 | TTG | 1min | 0 .. 45.5 days | 0xFFFF | VE_REG_TTG |
| 48 | 16 | Battery voltage | 0.01 V | -327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 64 | 16 | Alarm reason | | 0 .. 0xFFFF | - | VE_REG_ALARM_REASON |
| 80 | 16 | Aux voltage / Mid voltage / Temperature | 0.01 V / 0.01 V / 0.01 K | -327.68 .. 327.64 V / 0 .. 655.34 V / 0 .. 655.34 K | - | VE_REG_DC_CHANNEL2_VOLTAGE<br>VE_REG_BATTERY_MID_POINT_VOLTAGE<br>VE_REG_BAT_TEMPERATURE |
| 96 | 2 | Aux input | | 0 .. 3 | 0x3 | VE_REG_BMV_AUX_INPUT<br>0 ⇒ Aux voltage: VE_REG_DC_CHANNEL2_VOLTAGE<br>1 ⇒ Mid voltage: VE_REG_BATTERY_MID_POINT_VOLTAGE<br>2 ⇒ Temperature: VE_REG_BAT_TEMPERATURE<br>3 ⇒ none |
| 98 | 22 | Battery current | 0.001A | -4194 .. 4194 A | 0x3FFFFF | VE_REG_DC_CHANNEL1_CURRENT_MA |
| 120 | 20 | Consumed Ah | 0.1 Ah | -104,857 .. 0 Ah | 0xFFFFF | VE_REG_CAH<br>Consumed Ah = -Record value |
| 140 | 10 | SOC | 0.1% | 0 .. 100.0% | 0x3FF | VE_REG_SOC |
| 150 | 10 | Unused | | | | |

---

## Inverter

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 40 | 16 | Alarm Reason | | 0 .. 0xFFFF | - | VE_REG_ALARM_REASON |
| 56 | 16 | Battery voltage | 0.01 V | -327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 72 | 16 | AC Apparent power | 1 VA | 0 .. 65534 VA | 0xFFFF | VE_REG_AC_OUT_APPARENT_POWER |
| 88 | 15 | AC voltage | 0.01 V | 0 .. 327.66 V | 0x7FFF | VE_REG_AC_OUT_VOLTAGE |
| 103 | 11 | AC current | 0.1 A | 0 .. 204.6 A | 0x7FF | VE_REG_AC_OUT_CURRENT |
| 114 | 46 | Unused | | | | |

---

## DC/DC Converter

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 40 | 8 | Charger Error | | 0 .. 0xFE | 0xFF | VE_REG_CHR_ERROR_CODE |
| 48 | 16 | Input voltage | 0.01 V | 0 .. 655.34 V | 0xFFFF | VE_REG_DC_INPUT_VOLTAGE |
| 64 | 16 | Output voltage | 0.01 V | -327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 80 | 32 | Off reason | | 0 .. 0xFFFFFFFF | - | VE_REG_DEVICE_OFF_REASON_2 |
| 112 | 48 | Unused | | | | |

---

## SmartLithium

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 32 | BMS flags | | 0 .. 0xFFFFFFFF | | VE_REG_BMS_FLAGs |
| 64 | 16 | SmartLithium error | | 0 .. 0xFFFF | | VE_REG_SMART_LITHIUM_ERROR_FLAGS |
| 80 | 7 | Cell 1 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 87 | 7 | Cell 2 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 94 | 7 | Cell 3 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 101 | 7 | Cell 4 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 108 | 7 | Cell 5 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 115 | 7 | Cell 6 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 122 | 7 | Cell 7 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 129 | 7 | Cell 8 | 0.01V | 2.60 .. 3.86 V | 0xFF | VE_REG_BATTERY_CELL_VOLTAGE* |
| 136 | 12 | Battery voltage | 0.01 V | 0 .. 40.94 V | 0x0FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 148 | 4 | Balancer status | | 0 .. 15 | 0x0F | VE_REG_BALANCER_STATUS |
| 152 | 7 | Battery temperature | 1 °C | -40 .. 86 °C | 0x7F | VE_REG_BAT_TEMPERATURE<br>Temperature = Record value - 40 |
| 159 | 1 | Unused | | | | |

### Cell Voltage Encoding
- 0x00 (0) when cell voltage < 2.61V
- 0x01 (1) when cell voltage == 2.61V
- 0x7D (125) when cell voltage == 3.85V
- 0x7E (126) when cell voltage > 3.85
- 0x7F (127) when cell voltage is not available / unknown

---

## Inverter RS

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 40 | 8 | Charger Error | | 0 .. 0xFE | 0xFF | VE_REG_CHR_ERROR_CODE |
| 48 | 16 | Battery voltage | 0.01 V | -327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 64 | 16 | Battery current | 0.1A | -3276.8 .. 3276.6 A | 0x7FFF | VE_REG_DC_CHANNEL1_CURRENT |
| 80 | 16 | PV power | 1 W | 0 .. 65,534 W | 0xFFFF | VE_REG_DC_INPUT_POWER |
| 96 | 16 | Yield today | 0.01 kWh | 0 .. 655.34 kWh | 0xFFFF | VE_REG_CHR_TODAY_YIELD<br>655.34 kWh is 27.3 kW@24h |
| 112 | 16 | AC out power | 1 W | -32,768 .. 32,766 W | 0x7FFF | VE_REG_AC_OUT_REAL_POWER |
| 128 | 32 | Unused | | | | |

---

## GX-Device

**Record layout is still to be determined and might change.**

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 16 | Battery voltage | 0.01 V | 0 .. 655.34 V | 0xFFFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 48 | 20 | PV power | W | 0 .. 1 MW | 0xFFFFF | VE_REG_DC_INPUT_POWER |
| 68 | 7 | SOC | 1% | 0 .. 100% | 0x7F | VE_REG_SOC |
| 75 | 21 | Battery power | W | -1 .. 1 MW | 0x0FFFFF | VE_REG_DC_CHANNEL1_POWER |
| 96 | 21 | DC power | W | -1 .. 1 MW | 0x0FFFFF | TBD - AC in power<br>TBD - AC out power<br>TBD - Warnings/Alarms<br>TBD |
| 117 | 43 | Unused | | | | |

---

## AC Charger

**Record layout is still to be determined and might change.**

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 40 | 8 | Charger Error | | 0 .. 0xFE | 0xFF | VE_REG_CHR_ERROR_CODE |
| 48 | 13 | Battery voltage 1 | 0.01 V | 0 .. 81.90 V | 0x1FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 61 | 11 | Battery current 1 | 0.1A | 0 .. 204.6 A | 0x7FF | VE_REG_DC_CHANNEL1_CURRENT |
| 72 | 13 | Battery voltage 2 | 0.01 V | 0 .. 81.90 V | 0x1FFF | VE_REG_DC_CHANNEL2_VOLTAGE |
| 85 | 11 | Battery current 2 | 0.1A | 0 .. 204.6 A | 0x7FF | VE_REG_DC_CHANNEL2_CURRENT |
| 96 | 13 | Battery voltage 3 | 0.01 V | 0 .. 81.90 V | 0x1FFF | VE_REG_DC_CHANNEL3_VOLTAGE |
| 109 | 11 | Battery current 3 | 0.1A | 0 .. 204.6 A | 0x7FF | VE_REG_DC_CHANNEL3_CURRENT |
| 120 | 7 | Temperature | °C | -40 .. 86 °C | 0x7F | VE_REG_BAT_TEMPERATURE<br>Temperature = Record value - 40 |
| 127 | 9 | AC current | 0.1A | 0 .. 51.0 A | 0x1FF | VE_REG_AC_ACTIVE_INPUT_L1_CURRENT |
| 136 | 24 | Unused | | | | |

---

## Smart Battery Protect

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 8 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 16 | 8 | Output state | | 0 .. 0xFE | 0xFF | VE_REG_DC_OUTPUT_STATUS |
| 24 | 8 | Error code | | 0 .. 0xFE | 0xFF | VE_REG_CHR_ERROR_CODE |
| 32 | 16 | Alarm reason | | 0 .. 0xFFFF | - | VE_REG_ALARM_REASON |
| 48 | 16 | Warning reason | | 0 .. 0xFFFF | - | VE_REG_WARNING_REASON |
| 64 | 16 | Input voltage | 0.01 V | 327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 80 | 16 | Output voltage | 0.01 V | 0 .. 655.34 V | 0xFFFF | VE_REG_DC_OUTPUT_VOLTAGE |
| 96 | 32 | Off reason | | 0 .. 0xFFFFFFFF | - | VE_REG_DEVICE_OFF_REASON_2 |
| 128 | 32 | Unused | | | | |

---

## (Lynx Smart) BMS

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Error | | | 0x0 | VE_REG_BMS_ERROR |
| 40 | 16 | TTG | 1min | 0 .. 45.5 days | 0xFFFF | VE_REG_TTG |
| 56 | 16 | Battery voltage | 0.01 V | -327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 72 | 16 | Battery current | 0.1A | -3276.8 .. 3276.6 | 0x7FFF | VE_REG_DC_CHANNEL1_CURRENT |
| 88 | 16 | IO status | | | 0x0 | VE_REG_BMS_IO |
| 104 | 18 | Warnings/Alarms | | | 0x0 | VE_REG_BMS_WARNINGS_ALARMS |
| 122 | 10 | SOC | 0.1% | 0 .. 100.0% | 0x3FF | VE_REG_SOC |
| 132 | 20 | Consumed Ah | 0.1 Ah | -104,857 .. 0 Ah | 0xFFFFF | VE_REG_CAH<br>Consumed Ah = -Record value |
| 152 | 7 | Temperature | °C | -40 .. 86 °C | 0x7F | VE_REG_BAT_TEMPERATURE<br>Temperature = Record value - 40 |
| 159 | 1 | Unused | | | | |

---

## Multi RS

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 40 | 8 | Charger Error | | 0 .. 0xFE | 0xFF | VE_REG_CHR_ERROR_CODE |
| 48 | 16 | Battery current | 0.1A | -3276.8 .. 3276.6 A | 0x7FFF | VE_REG_DC_CHANNEL1_CURRENT |
| 64 | 14 | Battery voltage | 0.01 V | 0 .. 163.83V | 0x3FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 78 | 2 | Active AC in | | 0 .. 3 | 0x3 | VE_REG_AC_IN_ACTIVE<br>0 = AC in 1, 1 = AC in 2,<br>2 = Not connected, 3 = unknown |
| 80 | 16 | Active AC in power | 1 W | -32,768 .. 32,766 W | 0x7FFF | VE_REG_AC_IN_1_REAL_POWER or<br>VE_REG_AC_IN_2_REAL_POWER, depending<br>on VE_REG_AC_IN_ACTIVE |
| 96 | 16 | AC out power | 1 W | -32,768 .. 32,766 W | 0x7FFF | VE_REG_AC_OUT_REAL_POWER |
| 112 | 16 | PV power | 1 W | 0 .. 65534 W | 0xFFFF | VE_REG_DC_INPUT_POWER |
| 128 | 16 | Yield today | 0.01 kWh | 0 .. 655.34 kWh | 0xFFFF | VE_REG_CHR_TODAY_YIELD<br>655.34 kWh is 27.3 kW@24h |
| 144 | 16 | Unused | | | | |

---

## VE.Bus

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 8 | Device state | | 0 .. 0xFE | 0xFF | VE_REG_DEVICE_STATE |
| 40 | 8 | VE.Bus error | | 0 .. 0xFE | 0xFF | VE_REG_VEBUS_VEBUS_ERROR |
| 48 | 16 | Battery current | 0.1A | -3276.8 .. 3276.6 A | 0x7FFF | VE_REG_DC_CHANNEL1_CURRENT |
| 64 | 14 | Battery voltage | 0.01 V | 0 .. 163.83 V | 0x3FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 78 | 2 | Active AC in | | 0 .. 3 | 0x3 | VE_REG_AC_IN_ACTIVE<br>0 = AC in 1, 1 = AC in 2,<br>2 = Not connected, 3 = unknown |
| 80 | 19 | Active AC in power | 1 W | -262,144 .. 262,142 W | 0x3FFFF | VE_REG_AC_IN_1_REAL_POWER or<br>VE_REG_AC_IN_2_REAL_POWER,<br>depending on VE_REG_AC_IN_ACTIVE |
| 99 | 19 | AC out power | 1 W | -262,144 .. 262,142 W | 0x3FFFF | VE_REG_AC_OUT_REAL_POWER |
| 118 | 2 | Alarm | | 0 .. 2 | 3 | VE_REG_ALARM_NOTIFICATION (to be defined)<br>0 = no alarm, 1 = warning, 2 = alarm |
| 120 | 7 | Battery temperature | 1 °C | -40 .. 86 °C | 0x7F | VE_REG_BAT_TEMPERATURE<br>Temperature = Record value - 40 |
| 127 | 7 | SOC | 1 % | 0 .. 126 % | 0x7F | VE_REG_SOC |
| 134 | 26 | Unused | | | | |

---

## DC Energy Meter

| Start bit | Nr of bits | Meaning | Units | Range | NA value | Remark |
|-----------|------------|---------|-------|-------|----------|---------|
| 32 | 16 | BMV monitor mode | | –32768 .. 32767 | - | VE_REG_BMV_MONITOR_MODE |
| 48 | 16 | Battery voltage | 0.01 V | -327.68 .. 327.66 V | 0x7FFF | VE_REG_DC_CHANNEL1_VOLTAGE |
| 64 | 16 | Alarm reason | | 0 .. 0xFFFF | - | VE_REG_ALARM_REASON |
| 80 | 16 | Aux voltage / Temperature | 0.01 V / 0.01 K | -327.68 .. 327.64 V / 0 .. 655.34 K | - | VE_REG_DC_CHANNEL2_VOLTAGE<br>VE_REG_BAT_TEMPERATURE |
| 96 | 2 | Aux input | | 0 .. 3 | 0x3 | VE_REG_BMV_AUX_INPUT<br>0 ⇒ Aux voltage: VE_REG_DC_CHANNEL2_VOLTAGE<br>2 ⇒ Temperature: VE_REG_BAT_TEMPERATURE<br>3 ⇒ none |
| 98 | 22 | Battery current | 0.001A | -4194 .. 4194 A | 0x3FFFFF | VE_REG_DC_CHANNEL1_CURRENT_MA |
| 120 | 40 | Unused | | | | |

---

**Source:** Victron Energy Wiki  
**Permanent link:** https://wiki.victronenergy.com/rend/ble/extra_manufacturer_data  
**Last update:** 2022/12/14 13:25