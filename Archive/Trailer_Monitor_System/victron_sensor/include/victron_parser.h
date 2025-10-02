#pragma once
#include <cstdint>
#include <vector>

struct VictronParsed {
    double voltage = 0.0;
    double current = 0.0;
    double soc = -1.0;
    int remaining_mins = 0;
    double consumed_ah = 0.0;
};

// Parse functions expect the decrypted payload (bytes only)
VictronParsed parse_battery_monitor(const std::vector<uint8_t>& data);
VictronParsed parse_solar_charger(const std::vector<uint8_t>& data);
VictronParsed parse_dc_energy_meter(const std::vector<uint8_t>& data);
