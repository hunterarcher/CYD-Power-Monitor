#include <Arduino.h>
#include "victron_parser.h"
#include <vector>
#include <Arduino.h>
#include "victron_parser.h"
#include <vector>

static std::vector<uint8_t> hex_to_vec(const char* hex) {
    std::vector<uint8_t> v;
    const char* p = hex;
    while (*p) {
        while (*p == ' ') ++p;
        char a = *p++; if (!a) break;
        char b = *p++; if (!b) break;
        auto to_n = [](char c)->int { if (c>='0' && c<='9') return c-'0'; if (c>='a' && c<='f') return 10 + c - 'a'; if (c>='A' && c<='F') return 10 + c - 'A'; return 0; };
        v.push_back((to_n(a)<<4) | to_n(b));
    }
    return v;
}

static void printParsed(const char* name, const VictronParsed& p) {
    Serial.printf("%s -> V=%.2f V, I=%.3f A, SOC=%.1f%%, rem=%d, Ah=%.1f\n", name, p.voltage, p.current, p.soc, p.remaining_mins, p.consumed_ah);
}

// Call this from your main sketch if you want to run the parser tests.
void runVictronParserTests() {
    Serial.begin(115200);
    delay(1000);

    // samples (decrypted payloads captured earlier)
    const char* s_bmv_a = "0F8D87895CA2095480A0F4FBAB5290FB3C";
    const char* s_bmv_b = "39926B591105B487316FF8AFF2B44E064D";
    const char* s_mppt_a = "10F832FA52E41B2C5091725AFB97";
    const char* s_ac_a = "8A7ED2951B951F80ACFE8888674455";

    auto v1 = hex_to_vec(s_bmv_a);
    auto p1 = parse_battery_monitor(v1);
    printParsed("bmv_a", p1);

    auto v2 = hex_to_vec(s_bmv_b);
    auto p2 = parse_battery_monitor(v2);
    printParsed("bmv_b", p2);

    auto v3 = hex_to_vec(s_mppt_a);
    auto p3 = parse_solar_charger(v3);
    printParsed("mppt_a", p3);

    auto v4 = hex_to_vec(s_ac_a);
    auto p4 = parse_dc_energy_meter(v4);
    printParsed("ac_a", p4);
}
