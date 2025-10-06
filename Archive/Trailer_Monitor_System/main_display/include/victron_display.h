#ifndef VICTRON_DISPLAY_H
#define VICTRON_DISPLAY_H

#include <WiFi.h>
#include <esp_now.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3D

// Structure to receive Victron data from sensors (must match sensor side)
struct VictronData {
    uint8_t deviceType;     // 1=BMV-712, 2=MPPT, 3=IP22
    char deviceMAC[18];     // MAC address string
    float voltage;          // Battery/PV voltage
    float current;          // Battery/PV current  
    float power;           // Calculated power
    float soc;             // State of charge (BMV only)
    int rssi;              // BLE signal strength
    unsigned long timestamp; // When data was received
};

class VictronDisplay {
private:
    AsyncWebServer server;
    Adafruit_SSD1306 display;
    
    // Data storage
    VictronData bmvData;
    VictronData mpptData;
    VictronData ip22Data;
    
    bool bmvValid = false;
    bool mpptValid = false; 
    bool ip22Valid = false;
    
    unsigned long lastBMV = 0;
    unsigned long lastMPPT = 0;
    unsigned long lastIP22 = 0;
    
    const unsigned long DATA_TIMEOUT = 30000; // 30 seconds
    
    // Web interface methods
    void setupWebServer();
    String getStatusJSON();
    String generateWebPage();
    
    // Display methods
    void updateDisplay();
    void displaySummary();
    void displayDeviceData(const VictronData& data, const String& name);
    
    // Data validation
    bool isDataValid(unsigned long lastUpdate);
    void checkDataTimeouts();

public:
    VictronDisplay();
    bool begin(const char* ssid = nullptr, const char* password = nullptr);
    void loop();
    void onDataReceived(const VictronData& data);
    
    // Static callback for ESP-NOW
    static void dataReceivedCallback(const uint8_t* mac, const uint8_t* incomingData, int len);
    
    // Singleton pattern for callback access
    static VictronDisplay* instance;
};

#endif