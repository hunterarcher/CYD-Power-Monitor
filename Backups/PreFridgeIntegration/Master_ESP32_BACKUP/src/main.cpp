#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "VictronData.h"

// ============ CONFIGURATION ============
// WiFi credentials for web server
const char* WIFI_SSID = "Rocket";        // << CHANGE THIS
const char* WIFI_PASSWORD = "Ed1nburgh2015!"; // << CHANGE THIS

// Web server on port 80
WebServer server(80);

// ============ DATA STORAGE ============
VictronPacket latestData;
unsigned long lastReceived = 0;
uint32_t packetsReceived = 0;
uint32_t packetsMissed = 0;

// ============ ESP-NOW CALLBACK ============
void onDataReceive(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
    Serial.println("\n[ESP-NOW] Data received!");
    Serial.printf("  From MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
    Serial.printf("  Length: %d bytes\n", len);

    if (len == sizeof(VictronPacket)) {
        VictronPacket* packet = (VictronPacket*)data;

        // Check for missed packets
        if (packetsReceived > 0) {
            uint32_t expected = latestData.packetId + 1;
            if (packet->packetId != expected) {
                packetsMissed += (packet->packetId - expected);
                Serial.printf("  ⚠️  Missed %d packets\n", packet->packetId - expected);
            }
        }

        // Store data
        memcpy(&latestData, data, sizeof(VictronPacket));
        lastReceived = millis();
        packetsReceived++;

        Serial.printf("  ✓ Packet #%d stored\n", latestData.packetId);
        Serial.printf("    BMV: %.2fV, %.3fA, %.1f%% SOC\n",
                     latestData.bmv.voltage, latestData.bmv.current, latestData.bmv.soc);
        Serial.printf("    MPPT: %.2fV, %dW, %s\n",
                     latestData.mppt.batteryVoltage, (int)latestData.mppt.solarPower,
                     getStateName(latestData.mppt.state).c_str());
        Serial.printf("    IP22: %.2fV, %.2fA, %s\n",
                     latestData.ip22.batteryVoltage, latestData.ip22.batteryCurrent,
                     getStateName(latestData.ip22.state).c_str());
    } else {
        Serial.printf("  ✗ Wrong packet size! Expected %d, got %d\n", sizeof(VictronPacket), len);
    }
}

// ============ WEB SERVER HANDLERS ============

void handleRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>";
    html += "<title>Power Monitor</title>";
    html += "<style>";
    html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; background: #0a0a0a; color: #fff; overflow-x: hidden; }";
    html += ".header { background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%); padding: 12px; text-align: center; box-shadow: 0 2px 8px rgba(0,0,0,0.4); }";
    html += ".header h1 { font-size: 1.3em; font-weight: 600; margin: 0; }";
    html += ".grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; padding: 8px; }";
    html += ".card { background: linear-gradient(145deg, #1a1a1a, #252525); border-radius: 12px; padding: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.5); border: 1px solid #333; }";
    html += ".card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; padding-bottom: 8px; border-bottom: 1px solid #333; }";
    html += ".card-title { font-size: 0.85em; font-weight: 600; color: #aaa; text-transform: uppercase; letter-spacing: 0.5px; }";
    html += ".status-dot { width: 8px; height: 8px; border-radius: 50%; }";
    html += ".status-on { background: #4CAF50; box-shadow: 0 0 8px #4CAF50; }";
    html += ".status-off { background: #666; }";
    html += ".metrics { display: flex; flex-direction: column; gap: 6px; }";
    html += ".metric-row { display: flex; justify-content: space-between; align-items: baseline; }";
    html += ".metric-label { font-size: 0.7em; color: #888; }";
    html += ".metric-value { font-size: 1.1em; font-weight: 700; color: #4CAF50; }";
    html += ".metric-unit { font-size: 0.75em; color: #aaa; margin-left: 2px; }";
    html += ".large-value { font-size: 1.8em; text-align: center; margin: 8px 0; }";
    html += ".footer { background: #1a1a1a; padding: 10px; text-align: center; font-size: 0.7em; color: #666; margin-top: 8px; }";
    html += "@media (max-width: 400px) { .grid { gap: 6px; padding: 6px; } .card { padding: 10px; } }";
    html += "</style>";
    html += "<script>setInterval(function(){location.reload();}, 5000);</script>";
    html += "</head><body>";

    html += "<div class='header'><h1>⚡ Power Monitor</h1></div>";

    // Check if data is recent (within last 60 seconds)
    bool dataRecent = (millis() - lastReceived) < 60000;

    // Battery Monitor - BMV-712
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<span class='card-title'>🔋 Battery</span>";
    html += "<div class='status-dot " + String((dataRecent && latestData.bmv.valid) ? "status-on" : "status-off") + "'></div>";
    html += "</div>";
    if (dataRecent && latestData.bmv.valid) {
        html += "<div class='large-value'>" + String(latestData.bmv.soc, 0) + "<span class='metric-unit'>%</span></div>";
        html += "<div class='metrics'>";
        html += "<div class='metric-row'><span class='metric-label'>Voltage</span><span class='metric-value'>" + String(latestData.bmv.voltage, 2) + "<span class='metric-unit'>V</span></span></div>";
        html += "<div class='metric-row'><span class='metric-label'>Current</span><span class='metric-value'>" + String(latestData.bmv.current, 2) + "<span class='metric-unit'>A</span></span></div>";
        html += "</div>";
    } else {
        html += "<div class='large-value' style='color:#666;'>--</div>";
    }
    html += "</div>";

    // Solar - MPPT
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<span class='card-title'>☀️ Solar</span>";
    html += "<div class='status-dot " + String((dataRecent && latestData.mppt.valid) ? "status-on" : "status-off") + "'></div>";
    html += "</div>";
    if (dataRecent && latestData.mppt.valid) {
        html += "<div class='large-value'>" + String((int)latestData.mppt.solarPower) + "<span class='metric-unit'>W</span></div>";
        html += "<div class='metrics'>";
        html += "<div class='metric-row'><span class='metric-label'>Voltage</span><span class='metric-value'>" + String(latestData.mppt.batteryVoltage, 2) + "<span class='metric-unit'>V</span></span></div>";
        html += "<div class='metric-row'><span class='metric-label'>Current</span><span class='metric-value'>" + String(latestData.mppt.batteryCurrent, 2) + "<span class='metric-unit'>A</span></span></div>";
        html += "</div>";
    } else {
        html += "<div class='large-value' style='color:#666;'>--</div>";
    }
    html += "</div>";

    // AC Charger - IP22
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<span class='card-title'>⚡ AC Charger</span>";
    html += "<div class='status-dot " + String((dataRecent && latestData.ip22.valid) ? "status-on" : "status-off") + "'></div>";
    html += "</div>";
    if (dataRecent && latestData.ip22.valid) {
        html += "<div class='large-value'>" + String(latestData.ip22.power, 0) + "<span class='metric-unit'>W</span></div>";
        html += "<div class='metrics'>";
        html += "<div class='metric-row'><span class='metric-label'>Voltage</span><span class='metric-value'>" + String(latestData.ip22.batteryVoltage, 2) + "<span class='metric-unit'>V</span></span></div>";
        html += "<div class='metric-row'><span class='metric-label'>Current</span><span class='metric-value'>" + String(latestData.ip22.batteryCurrent, 2) + "<span class='metric-unit'>A</span></span></div>";
        html += "</div>";
    } else {
        html += "<div class='large-value' style='color:#666;'>--</div>";
    }
    html += "</div>";

    // EcoFlow Delta 2 Max
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<span class='card-title'>🔌 EcoFlow</span>";
    html += "<div class='status-dot " + String((dataRecent && latestData.ecoflow.valid) ? "status-on" : "status-off") + "'></div>";
    html += "</div>";
    if (dataRecent && latestData.ecoflow.valid) {
        html += "<div class='large-value'>" + String(latestData.ecoflow.batteryPercent) + "<span class='metric-unit'>%</span></div>";
        html += "<div class='metrics'>";
        html += "<div class='metric-row'><span class='metric-label'>Serial</span><span class='metric-value' style='font-size:0.9em;'>" + String(latestData.ecoflow.serialNumber) + "</span></div>";
        html += "<div class='metric-row'><span class='metric-label'>Signal</span><span class='metric-value'>" + String(latestData.ecoflow.rssi) + "<span class='metric-unit'>dBm</span></span></div>";
        html += "</div>";
    } else {
        html += "<div class='large-value' style='color:#666;'>--</div>";
    }
    html += "</div>";

    html += "</div>"; // end grid

    // Footer
    html += "<div class='footer'>";
    html += "Last: " + String((millis() - lastReceived) / 1000) + "s ago | ";
    html += "RX: " + String(packetsReceived) + " | ";
    html += "Missed: " + String(packetsMissed);
    html += "</div>";

    html += "</body></html>";

    server.send(200, "text/html", html);
}

void handleJson() {
    String json = "{";
    json += "\"bmv\":{";
    json += "\"voltage\":" + String(latestData.bmv.voltage, 2) + ",";
    json += "\"current\":" + String(latestData.bmv.current, 3) + ",";
    json += "\"soc\":" + String(latestData.bmv.soc, 1) + ",";
    json += "\"consumedAh\":" + String(latestData.bmv.consumedAh, 1) + ",";
    json += "\"auxVoltage\":" + String(latestData.bmv.auxVoltage, 2) + ",";
    json += "\"timeToGo\":" + String(latestData.bmv.timeToGo) + ",";
    json += "\"valid\":" + String(latestData.bmv.valid ? "true" : "false");
    json += "},";
    json += "\"mppt\":{";
    json += "\"voltage\":" + String(latestData.mppt.batteryVoltage, 2) + ",";
    json += "\"current\":" + String(latestData.mppt.batteryCurrent, 2) + ",";
    json += "\"power\":" + String(latestData.mppt.solarPower, 0) + ",";
    json += "\"yieldToday\":" + String(latestData.mppt.yieldToday, 2) + ",";
    json += "\"state\":\"" + getStateName(latestData.mppt.state) + "\",";
    json += "\"valid\":" + String(latestData.mppt.valid ? "true" : "false");
    json += "},";
    json += "\"ip22\":{";
    json += "\"voltage\":" + String(latestData.ip22.batteryVoltage, 2) + ",";
    json += "\"current\":" + String(latestData.ip22.batteryCurrent, 2) + ",";
    json += "\"loadCurrent\":" + String(latestData.ip22.loadCurrent, 2) + ",";
    json += "\"power\":" + String(latestData.ip22.power, 1) + ",";
    json += "\"state\":\"" + getStateName(latestData.ip22.state) + "\",";
    json += "\"valid\":" + String(latestData.ip22.valid ? "true" : "false");
    json += "},";
    json += "\"ecoflow\":{";
    json += "\"serialNumber\":\"" + String(latestData.ecoflow.serialNumber) + "\",";
    json += "\"batteryPercent\":" + String(latestData.ecoflow.batteryPercent) + ",";
    json += "\"macAddress\":\"" + String(latestData.ecoflow.macAddress) + "\",";
    json += "\"rssi\":" + String(latestData.ecoflow.rssi) + ",";
    json += "\"valid\":" + String(latestData.ecoflow.valid ? "true" : "false");
    json += "},";
    json += "\"system\":{";
    json += "\"packetsReceived\":" + String(packetsReceived) + ",";
    json += "\"packetsMissed\":" + String(packetsMissed) + ",";
    json += "\"lastUpdate\":" + String((millis() - lastReceived) / 1000);
    json += "}";
    json += "}";

    server.send(200, "application/json", json);
}

// ============ SETUP ============
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("   Victron Master ESP32 (ESP-NOW + Web)");
    Serial.println("========================================\n");

    // Connect to WiFi
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_AP_STA);  // Both AP and Station mode for ESP-NOW + WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi connected!");
        Serial.printf("   IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("   Open browser: http://%s\n\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n✗ WiFi connection failed!");
        Serial.println("   Will still work with ESP-NOW, but no web interface");
    }

    // Print MAC address for ESP-NOW pairing
    Serial.println("Master ESP32 MAC Address:");
    Serial.printf("   %s\n", WiFi.macAddress().c_str());
    Serial.println("   ^^ Use this MAC in Victron ESP32 sender\n");

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ ESP-NOW init failed!");
        return;
    }
    Serial.println("✓ ESP-NOW initialized");

    // Register receive callback
    esp_now_register_recv_cb(onDataReceive);
    Serial.println("✓ ESP-NOW receive callback registered\n");

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/json", handleJson);
    server.begin();
    Serial.println("✓ Web server started\n");

    Serial.println("Waiting for ESP-NOW data...\n");
}

// ============ LOOP ============
void loop() {
    server.handleClient();
    delay(10);
}
