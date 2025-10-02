// config.h
#ifndef CONFIG_H
#define CONFIG_H

// Module enables
#define ENABLE_FRIDGE 0
#define ENABLE_VICTRON_MINIMAL 1      // ENABLED: Optimized minimal BLE version
#define ENABLE_VICTRON_BLE 0          // OLD: Disabled failed bindkey approach
#define ENABLE_ECOFLOW 0

// WiFi Configuration
#define WIFI_SSID "Rocket"
#define WIFI_PASSWORD "Ed1nburgh2015!"

// Victron Instant Readout settings (NEW - Working approach)
#define VICTRON_SCAN_INTERVAL 30000   // 30 seconds between BLE scans
#define VICTRON_DATA_TIMEOUT 120000   // 2 minutes data timeout

// Victron Device MAC Addresses (from your working HA setup)
#define BMV712_MAC_ADDRESS "c0:3b:98:39:e6:fe"   // BMV-712 Smart (Battery Monitor)
#define MPPT_MAC_ADDRESS "e8:86:01:5d:79:38"     // SmartSolar MPPT (Solar Controller)
#define IP22_MAC_ADDRESS "c7:a2:c2:61:9f:c4"     // Blue Smart IP22 (AC Charger)

// Web Server Configuration
#define WEB_SERVER_PORT 80

#endif