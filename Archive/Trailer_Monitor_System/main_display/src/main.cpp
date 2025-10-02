#include "victron_display.h"

// Static instance for callback access
VictronDisplay* VictronDisplay::instance = nullptr;

// WiFi credentials - update these for your network
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

VictronDisplay victronDisplay;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Victron Main Display ESP32 ===");
    Serial.println("Initializing ESP-NOW receiver + Web server...");
    
    if (victronDisplay.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("✓ Main display ready!");
        Serial.println("✓ Web interface available at: http://" + WiFi.localIP().toString());
    } else {
        Serial.println("✗ Failed to initialize main display");
        while(1) delay(1000);
    }
}

void loop() {
    victronDisplay.loop();
    delay(100);
}

// ========================= VictronDisplay Implementation =========================

VictronDisplay::VictronDisplay() : server(80), display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
    instance = this;  // Set singleton instance
}

bool VictronDisplay::begin(const char* ssid, const char* password) {
    // Initialize I2C display
    Serial.println("Initializing OLED display...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("✗ SSD1306 allocation failed");
        // Continue without display
    } else {
        Serial.println("✓ OLED display initialized");
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("Victron Monitor");
        display.println("Initializing...");
        display.display();
    }
    
    // Initialize WiFi
    if (ssid && password) {
        Serial.println("Connecting to WiFi...");
        WiFi.begin(ssid, password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.println("✓ WiFi connected!");
            Serial.println("IP address: " + WiFi.localIP().toString());
        } else {
            Serial.println();
            Serial.println("✗ WiFi connection failed, continuing in AP mode");
            WiFi.softAP("VictronDisplay", "12345678");
            Serial.println("AP IP: " + WiFi.softAPIP().toString());
        }
    } else {
        // Start in AP mode
        WiFi.softAP("VictronDisplay", "12345678");
        Serial.println("✓ Started AP mode");
        Serial.println("AP IP: " + WiFi.softAPIP().toString());
    }
    
    // Initialize ESP-NOW
    Serial.println("Initializing ESP-NOW receiver...");
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ ESP-NOW init failed");
        return false;
    }
    
    // Register receive callback
    esp_now_register_recv_cb(dataReceivedCallback);
    Serial.println("✓ ESP-NOW receiver initialized");
    
    // Setup web server
    setupWebServer();
    
    return true;
}

void VictronDisplay::loop() {
    checkDataTimeouts();
    updateDisplay();
    delay(1000);  // Update display every second
}

void VictronDisplay::dataReceivedCallback(const uint8_t* mac, const uint8_t* incomingData, int len) {
    if (instance && len == sizeof(VictronData)) {
        VictronData receivedData;
        memcpy(&receivedData, incomingData, sizeof(receivedData));
        instance->onDataReceived(receivedData);
    }
}

void VictronDisplay::onDataReceived(const VictronData& data) {
    Serial.printf("Received data from device type %d: %.2fV, %.2fA\n", 
                  data.deviceType, data.voltage, data.current);
    
    switch(data.deviceType) {
        case 1:  // BMV-712
            bmvData = data;
            bmvValid = true;
            lastBMV = millis();
            break;
            
        case 2:  // MPPT
            mpptData = data;
            mpptValid = true;
            lastMPPT = millis();
            break;
            
        case 3:  // IP22
            ip22Data = data;
            ip22Valid = true;
            lastIP22 = millis();
            break;
    }
}

void VictronDisplay::setupWebServer() {
    Serial.println("Setting up web server...");
    
    // Main page
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", generateWebPage());
    });
    
    // JSON API endpoint
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", getStatusJSON());
    });
    
    // Start server
    server.begin();
    Serial.println("✓ Web server started");
}

String VictronDisplay::getStatusJSON() {
    DynamicJsonDocument doc(1024);
    
    doc["timestamp"] = millis();
    doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
    doc["wifi_ip"] = WiFi.localIP().toString();
    
    // BMV-712 data
    JsonObject bmv = doc.createNestedObject("bmv");
    bmv["valid"] = bmvValid && isDataValid(lastBMV);
    if (bmv["valid"]) {
        bmv["voltage"] = bmvData.voltage;
        bmv["current"] = bmvData.current;
        bmv["power"] = bmvData.power;
        bmv["soc"] = bmvData.soc;
        bmv["rssi"] = bmvData.rssi;
        bmv["age"] = (millis() - lastBMV) / 1000;
    }
    
    // MPPT data
    JsonObject mppt = doc.createNestedObject("mppt");
    mppt["valid"] = mpptValid && isDataValid(lastMPPT);
    if (mppt["valid"]) {
        mppt["voltage"] = mpptData.voltage;
        mppt["current"] = mpptData.current;
        mppt["power"] = mpptData.power;
        mppt["rssi"] = mpptData.rssi;
        mppt["age"] = (millis() - lastMPPT) / 1000;
    }
    
    // IP22 data
    JsonObject ip22 = doc.createNestedObject("ip22");
    ip22["valid"] = ip22Valid && isDataValid(lastIP22);
    if (ip22["valid"]) {
        ip22["voltage"] = ip22Data.voltage;
        ip22["current"] = ip22Data.current;
        ip22["power"] = ip22Data.power;
        ip22["rssi"] = ip22Data.rssi;
        ip22["age"] = (millis() - lastIP22) / 1000;
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String VictronDisplay::generateWebPage() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>Victron Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; }
        .card { background: white; padding: 20px; margin: 10px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .device { display: flex; justify-content: space-between; align-items: center; margin: 10px 0; }
        .status { padding: 5px 10px; border-radius: 4px; color: white; font-weight: bold; }
        .online { background: #4CAF50; }
        .offline { background: #f44336; }
        .value { font-size: 1.2em; font-weight: bold; }
        .unit { color: #666; font-size: 0.9em; }
        .refresh { background: #2196F3; color: white; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h1>🔋 Victron Monitor</h1>
            <button class="refresh" onclick="refreshData()">Refresh</button>
        </div>
        
        <div id="devices" class="grid">
            <div class="card">
                <h3>BMV-712 Battery Monitor</h3>
                <div id="bmv-status" class="status offline">Offline</div>
                <div id="bmv-data">Waiting for data...</div>
            </div>
            
            <div class="card">
                <h3>MPPT Solar Controller</h3>
                <div id="mppt-status" class="status offline">Offline</div>
                <div id="mppt-data">Waiting for data...</div>
            </div>
            
            <div class="card">
                <h3>IP22 Charger</h3>
                <div id="ip22-status" class="status offline">Offline</div>
                <div id="ip22-data">Waiting for data...</div>
            </div>
        </div>
    </div>

    <script>
        function updateDevice(name, data) {
            const status = document.getElementById(name + '-status');
            const dataDiv = document.getElementById(name + '-data');
            
            if (data.valid) {
                status.textContent = 'Online';
                status.className = 'status online';
                
                let html = '<div class="device">';
                html += '<span>Voltage:</span><span class="value">' + data.voltage.toFixed(2) + '<span class="unit">V</span></span>';
                html += '</div>';
                
                html += '<div class="device">';
                html += '<span>Current:</span><span class="value">' + data.current.toFixed(2) + '<span class="unit">A</span></span>';
                html += '</div>';
                
                html += '<div class="device">';
                html += '<span>Power:</span><span class="value">' + data.power.toFixed(1) + '<span class="unit">W</span></span>';
                html += '</div>';
                
                if (name === 'bmv' && data.soc !== undefined) {
                    html += '<div class="device">';
                    html += '<span>SOC:</span><span class="value">' + data.soc.toFixed(0) + '<span class="unit">%</span></span>';
                    html += '</div>';
                }
                
                html += '<div class="device">';
                html += '<span>Signal:</span><span class="value">' + data.rssi + '<span class="unit">dBm</span></span>';
                html += '</div>';
                
                html += '<div class="device">';
                html += '<span>Age:</span><span class="value">' + data.age + '<span class="unit">s</span></span>';
                html += '</div>';
                
                dataDiv.innerHTML = html;
            } else {
                status.textContent = 'Offline';
                status.className = 'status offline';
                dataDiv.innerHTML = 'No recent data';
            }
        }
        
        function refreshData() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    updateDevice('bmv', data.bmv);
                    updateDevice('mppt', data.mppt);
                    updateDevice('ip22', data.ip22);
                })
                .catch(error => console.error('Error:', error));
        }
        
        // Auto-refresh every 5 seconds
        setInterval(refreshData, 5000);
        
        // Initial load
        refreshData();
    </script>
</body>
</html>
    )";
}

void VictronDisplay::updateDisplay() {
    if (!display.getRotation() == 0) return;  // Display not available
    
    display.clearDisplay();
    display.setCursor(0, 0);
    
    display.println("Victron Monitor");
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    
    int y = 15;
    
    // BMV-712
    display.setCursor(0, y);
    if (bmvValid && isDataValid(lastBMV)) {
        display.printf("BMV: %.1fV %.1fA", bmvData.voltage, bmvData.current);
        y += 10;
        display.setCursor(0, y);
        display.printf("SOC: %.0f%% PWR:%.0fW", bmvData.soc, bmvData.power);
    } else {
        display.print("BMV: Offline");
    }
    y += 12;
    
    // MPPT
    display.setCursor(0, y);
    if (mpptValid && isDataValid(lastMPPT)) {
        display.printf("MPPT: %.1fV %.1fA", mpptData.voltage, mpptData.current);
        y += 10;
        display.setCursor(0, y);
        display.printf("Solar: %.0fW", mpptData.power);
    } else {
        display.print("MPPT: Offline");
    }
    y += 12;
    
    // IP22
    display.setCursor(0, y);
    if (ip22Valid && isDataValid(lastIP22)) {
        display.printf("IP22: %.1fV %.1fA", ip22Data.voltage, ip22Data.current);
    } else {
        display.print("IP22: Offline");
    }
    
    display.display();
}

bool VictronDisplay::isDataValid(unsigned long lastUpdate) {
    return (millis() - lastUpdate) < DATA_TIMEOUT;
}

void VictronDisplay::checkDataTimeouts() {
    if (bmvValid && !isDataValid(lastBMV)) {
        bmvValid = false;
        Serial.println("BMV data timeout");
    }
    
    if (mpptValid && !isDataValid(lastMPPT)) {
        mpptValid = false;
        Serial.println("MPPT data timeout");
    }
    
    if (ip22Valid && !isDataValid(lastIP22)) {
        ip22Valid = false;
        Serial.println("IP22 data timeout");
    }
}