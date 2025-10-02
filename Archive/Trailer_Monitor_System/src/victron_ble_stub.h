#pragma once

#include <Arduino.h>

// Minimal stub to satisfy linkage if the component header isn't directly used.
// This file deliberately contains only trivial implementations; the real
// implementation is in the victron_sensor component.

class VictronBLE {
public:
    VictronBLE() {}
    bool begin() { return false; }
    // Older code expects setup() method name
    bool setup() { return begin(); }
    void loop() {}
    void startScan() {}
    void stopScan() {}

    bool isBMVOnline() const { return false; }
    float getBatteryVoltage() const { return 0.0f; }
    float getBatteryCurrent() const { return 0.0f; }
    float getBatterySOC() const { return 0.0f; }
    float getConsumedAh() const { return 0.0f; }
    float getAuxVoltage() const { return 0.0f; }
    int getTimeToGo() const { return 0; }
    bool hasLowVoltageAlarm() const { return false; }
    bool hasHighVoltageAlarm() const { return false; }
    bool hasLowSOCAlarm() const { return false; }

    bool isSolarOnline() const { return false; }
    float getSolarVoltage() const { return 0.0f; }
    float getSolarCurrent() const { return 0.0f; }
    float getSolarPower() const { return 0.0f; }
    String getSolarState() const { return String("NoData"); }
    String getSolarError() const { return String("NoError"); }

    bool isACOnline() const { return false; }
    float getACVoltage() const { return 0.0f; }
    float getACCurrent() const { return 0.0f; }
    float getACTemperature() const { return 0.0f; }
    String getACState() const { return String("NoData"); }
    String getACError() const { return String("NoError"); }
    // Additional helpers used by main.cpp
    float getYieldToday() const { return 0.0f; }
};
