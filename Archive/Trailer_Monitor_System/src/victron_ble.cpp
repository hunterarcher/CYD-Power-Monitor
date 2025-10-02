// Top-level compatibility shim: intentionally empty.
// The real VictronBLE implementation lives in the component:
// victron_sensor/src/main.cpp. This file only exists so older build layouts
// that expect a top-level translation unit do not fail the build.

#include "../victron_sensor/include/victron_ble.h"

// No definitions here. All functions are implemented in the victron_sensor
// component. This translation unit intentionally defines nothing to avoid
// duplicate or mismatched symbol definitions.