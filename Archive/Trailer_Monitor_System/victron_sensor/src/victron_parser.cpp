#include "victron_parser.h"
#include <cstdint>
#include <vector>
#include <cstring>

// helpers
static uint16_t read_u16_le(const uint8_t* p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static int16_t read_i16_le(const uint8_t* p) { return (int16_t)read_u16_le(p); }
// read 24-bit signed (Int24sl) used in Python code: upper 22 bits are signed milliamps and low 2 bits aux mode
static int32_t read_i24_le(const uint8_t* p) {
    uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    // Sign extend 24-bit
    if (v & 0x800000) v |= 0xFF000000;
    return (int32_t)v;
}

VictronParsed parse_battery_monitor(const std::vector<uint8_t>& data) {
    VictronParsed out;
    // BatteryMonitor.PACKET expects at least: remaining_mins(2) voltage(2) alarm(1) uk_1b(1) aux(2) current(3) consumed_ah(2) soc(2)
    if (data.size() < 15) return out;
    const uint8_t* p = data.data();
    out.remaining_mins = read_u16_le(p + 0);
    uint16_t raw_voltage = read_u16_le(p + 2);
    out.voltage = raw_voltage / 100.0;
    // skip alarm (1) and uk_1b(1)
    uint16_t aux = read_u16_le(p + 6);
    int32_t current24 = read_i24_le(p + 8);
    out.current = (current24 >> 2) / 1000.0;
    uint16_t consumed = read_u16_le(p + 11);
    out.consumed_ah = consumed / 10.0;
    uint16_t soc_raw = read_u16_le(p + 13);
    out.soc = ((soc_raw & 0x3FFF) >> 4) / 10.0;
    return out;
}

VictronParsed parse_solar_charger(const std::vector<uint8_t>& data) {
    VictronParsed out;
    // SolarCharger.PACKET: charge_state(2) battery_voltage(2) battery_charging_current(2) yield_today(2) solar_power(2) external_device_load(2)
    if (data.size() < 12) return out;
    const uint8_t* p = data.data();
    int16_t charge_state = read_i16_le(p + 0);
    uint16_t raw_voltage = read_u16_le(p + 2);
    out.voltage = raw_voltage / 100.0;
    uint16_t raw_current = read_u16_le(p + 4);
    out.current = raw_current / 10.0;
    return out;
}

VictronParsed parse_dc_energy_meter(const std::vector<uint8_t>& data) {
    VictronParsed out;
    // DcEnergyMeter.PACKET: meter_type(2) voltage(2) alarm(1) uk_1b(1) aux(2) current(3)
    if (data.size() < 10) return out;
    const uint8_t* p = data.data();
    int16_t meter_type = read_i16_le(p + 0);
    uint16_t raw_voltage = read_u16_le(p + 2);
    out.voltage = raw_voltage / 100.0;
    int32_t current24 = read_i24_le(p + 8 - 3); // adjust for position when less bytes
    // but Python parses current from offset 6, so try that when size sufficient
    if (data.size() >= 9) {
        current24 = read_i24_le(p + 6);
        out.current = (current24 >> 2) / 1000.0;
    }
    return out;
}
