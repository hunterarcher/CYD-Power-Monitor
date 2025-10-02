#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "VictronData.h"

// ============ CONFIGURATION ============
// Victron ESP32 MAC address (use STA MAC from Victron serial output)
uint8_t victronMAC[] = {0x78, 0x21, 0x84, 0x9C, 0x9B, 0x88};

// WiFi AP credentials
const char* AP_SSID = "PowerMonitor";
const char* AP_PASSWORD = "12345678";

// Web server on port 80
WebServer server(80);

// ============ DATA STORAGE ============
VictronPacket latestData;
unsigned long lastReceived = 0;
uint32_t packetsReceived = 0;
uint32_t packetsMissed = 0;

// ============ STATUS TRACKING ============
bool victronReady = false;  // True when Victron is ready to receive commands
unsigned long lastStatusUpdate = 0;

// ============ COMMAND QUEUE ============
uint32_t nextCommandId = 1;

// ============ ESP-NOW CALLBACKS ============

// Send callback
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.printf("[ESP-NOW] Send status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "✓ Success" : "✗ Failed");
}

// Receive callback for VictronPacket data
void onDataReceive(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
    if (len == sizeof(VictronPacket)) {
        VictronPacket* packet = (VictronPacket*)data;

        // Check for missed packets
        if (packetsReceived > 0) {
            uint32_t expected = latestData.packetId + 1;
            if (packet->packetId != expected) {
                packetsMissed += (packet->packetId - expected);
                Serial.printf("[ESP-NOW] ⚠️  Missed %d packets\n", packet->packetId - expected);
            }
        }

        // Store data
        memcpy(&latestData, data, sizeof(VictronPacket));
        lastReceived = millis();
        packetsReceived++;

        Serial.printf("[ESP-NOW] ✓ Packet #%d\n", latestData.packetId);
    } else if (len == sizeof(StatusMessage)) {
        StatusMessage* status = (StatusMessage*)data;
        victronReady = (status->type == STATUS_READY);
        lastStatusUpdate = millis();
        Serial.printf("[STATUS] Victron is now: %s\n",
                     victronReady ? "READY ✓" : "SCANNING");
    } else if (len == sizeof(CommandAck)) {
        CommandAck* ack = (CommandAck*)data;
        Serial.printf("[ACK] Command #%d | RX:%s | EXEC:%s | Err:%d\n",
                     ack->commandId,
                     ack->received ? "✓" : "✗",
                     ack->executed ? "✓" : "✗",
                     ack->errorCode);
    } else {
        Serial.printf("[ESP-NOW] ✗ Unknown packet size: %d bytes\n", len);
    }
}

// Send command to Victron ESP32
bool sendCommand(uint8_t device, uint8_t command, int16_t value1, int16_t value2) {
    if (!victronReady) {
        Serial.println("[COMMAND] ✗ Victron is SCANNING - cannot send command");
        Serial.printf("[COMMAND]    Last status update: %lu ms ago\n", millis() - lastStatusUpdate);
        return false;
    }

    ControlCommand cmd;
    cmd.commandId = nextCommandId++;
    cmd.device = device;
    cmd.command = command;
    cmd.value1 = value1;
    cmd.value2 = value2;
    cmd.timestamp = millis();

    Serial.printf("[COMMAND] ✓ Victron is READY - Sending command #%d (device=%d, cmd=%d, v1=%d, v2=%d)\n",
                 cmd.commandId, device, command, value1, value2);

    esp_err_t result = esp_now_send(victronMAC, (uint8_t*)&cmd, sizeof(cmd));
    Serial.printf("[COMMAND] esp_now_send result: %d (%s)\n",
                 result, result == ESP_OK ? "ESP_OK" : "ERROR");

    return (result == ESP_OK);
}

// ============ WEB SERVER HANDLERS ============

void handleRoot() {
    bool dataRecent = (millis() - lastReceived) < 60000;

    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>Power</title>";
    html += "<style>body{background:#000;color:#fff;font-family:Arial;margin:0;padding:10px}";
    html += "a{color:#4af;text-decoration:none;padding:8px 16px;background:#222;border-radius:4px;display:inline-block;margin:5px}";
    html += ".c{background:#111;margin:10px 0;padding:15px;border-radius:8px}";
    html += "h2{margin:0 0 10px;font-size:1.2em}";
    html += ".v{font-size:2em;color:#4f4}</style></head><body>";

    html += "<a href='/'>Main</a><a href='/fridge'>Fridge</a><hr>";

    // Battery
    html += "<div class='c'><h2>Battery</h2>";
    if (dataRecent && latestData.bmv.valid) {
        html += "<div class='v'>" + String(latestData.bmv.soc, 0) + "%</div>";
        html += String(latestData.bmv.voltage, 2) + "V | " + String(latestData.bmv.current, 2) + "A";
    } else {
        html += "<div class='v'>--</div>";
    }
    html += "</div>";

    // Solar
    html += "<div class='c'><h2>Solar</h2>";
    if (dataRecent && latestData.mppt.valid) {
        html += "<div class='v'>" + String((int)latestData.mppt.solarPower) + "W</div>";
        html += String(latestData.mppt.batteryVoltage, 2) + "V | " + String(latestData.mppt.batteryCurrent, 2) + "A";
    } else {
        html += "<div class='v'>--</div>";
    }
    html += "</div>";

    // AC Charger
    html += "<div class='c'><h2>AC Charger</h2>";
    if (dataRecent && latestData.ip22.valid) {
        html += "<div class='v'>" + String(latestData.ip22.power, 0) + "W</div>";
        html += String(latestData.ip22.batteryVoltage, 2) + "V | " + String(latestData.ip22.batteryCurrent, 2) + "A";
    } else {
        html += "<div class='v'>--</div>";
    }
    html += "</div>";

    // EcoFlow
    html += "<div class='c'><h2>EcoFlow</h2>";
    if (dataRecent && latestData.ecoflow.valid) {
        html += "<div class='v'>" + String(latestData.ecoflow.batteryPercent) + "%</div>";
        html += "RSSI: " + String(latestData.ecoflow.rssi) + " dBm";
    } else {
        html += "<div class='v'>--</div>";
    }
    html += "</div>";

    html += "<p><small>Packets: " + String(packetsReceived) + " | Last: " + String((millis()-lastReceived)/1000) + "s</small></p>";

    html += "<hr><a href='/test' style='background:#282;padding:15px 30px;font-size:1.2em'>TEST SEND COMMAND</a>";

    html += "</body></html>";

    server.send(200, "text/html", html);
}

void handleFridge() {
    bool dataRecent = (millis() - lastReceived) < 60000;

    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>Fridge</title>";
    html += "<style>body{background:#000;color:#fff;font-family:Arial;margin:0;padding:10px;text-align:center}";
    html += "a{color:#4af;text-decoration:none;padding:8px 16px;background:#222;border-radius:4px;display:inline-block;margin:5px}";
    html += ".c{background:#111;margin:20px 0;padding:20px;border-radius:8px}";
    html += ".t{font-size:3em;color:#4f4;margin:10px 0}";
    html += ".btn{padding:15px 30px;font-size:2em;background:#248;color:#fff;border:none;border-radius:8px;margin:10px}";
    html += "</style></head><body>";

    html += "<a href='/'>Main</a><a href='/fridge'>Fridge</a><hr>";

    if (dataRecent && latestData.fridge.valid) {
        // Left
        html += "<div class='c'><h2>LEFT</h2>";
        html += "<div class='t'>" + String(latestData.fridge.left_actual) + "°C</div>";
        html += "<button class='btn' onclick=\"location.href='/fridge/cmd?zone=0&temp=" + String(latestData.fridge.left_setpoint + 1) + "'\">+</button>";
        html += "<span style='font-size:1.5em'> " + String(latestData.fridge.left_setpoint) + "°C </span>";
        html += "<button class='btn' onclick=\"location.href='/fridge/cmd?zone=0&temp=" + String(latestData.fridge.left_setpoint - 1) + "'\">-</button>";
        html += "</div>";

        // Right
        html += "<div class='c'><h2>RIGHT</h2>";
        html += "<div class='t'>" + String(latestData.fridge.right_actual) + "°C</div>";
        html += "<button class='btn' onclick=\"location.href='/fridge/cmd?zone=1&temp=" + String(latestData.fridge.right_setpoint + 1) + "'\">+</button>";
        html += "<span style='font-size:1.5em'> " + String(latestData.fridge.right_setpoint) + "°C </span>";
        html += "<button class='btn' onclick=\"location.href='/fridge/cmd?zone=1&temp=" + String(latestData.fridge.right_setpoint - 1) + "'\">-</button>";
        html += "</div>";

        html += "<p>ECO: " + String(latestData.fridge.eco_mode ? "ON" : "OFF");
        html += " | Battery: " + String(latestData.fridge.battery_protection);
        html += " | Signal: " + String(latestData.fridge.rssi) + "dBm</p>";
    } else {
        html += "<p>No fridge data</p>";
    }

    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleTest() {
    Serial.println("[WEB] TEST button pressed - sending test command");

    bool success = sendCommand(1, CMD_FRIDGE_SET_TEMP, 0, 4);  // Zone 0, Temp 4°C

    server.sendHeader("Location", "/");
    server.send(303);
}

void handleFridgeCommand() {
    if (!server.hasArg("zone") || !server.hasArg("temp")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int8_t zone = server.arg("zone").toInt();
    int8_t temp = server.arg("temp").toInt();

    Serial.printf("[WEB] Fridge command: Zone %d to %d°C\n", zone, temp);

    bool success = sendCommand(1, CMD_FRIDGE_SET_TEMP, zone, temp);

    server.sendHeader("Location", "/fridge");
    server.send(303);
}

void handleNotFound() {
    Serial.printf("[WEB] 404: %s\n", server.uri().c_str());
    server.send(404, "text/plain", "Not Found");
}

// ============ SETUP ============
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("   Victron Master ESP32 (ESP-NOW + Web)");
    Serial.println("========================================\n");

    // CRITICAL: Use AP_STA mode for ESP-NOW + Web Server
    WiFi.mode(WIFI_AP_STA);
    delay(100);

    // Create Access Point (no external WiFi connection)
    WiFi.softAP(AP_SSID, AP_PASSWORD, 1, 0, 4);  // channel 1, hidden=0, max_conn=4
    delay(500);

    // Print MAC addresses
    Serial.printf("Master STA MAC: %s\n", WiFi.macAddress().c_str());
    Serial.println("^^ Use this MAC in Victron ESP32 code!\n");

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    Serial.printf("Master AP MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    Serial.printf("AP Started: %s\n", AP_SSID);
    Serial.printf("AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.println();

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ ESP-NOW init failed!");
        return;
    }
    Serial.println("✓ ESP-NOW initialized");

    // Register callbacks
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceive);
    Serial.println("✓ ESP-NOW callbacks registered");

    // Add Victron ESP32 as peer (channel 1 = default AP channel)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, victronMAC, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.printf("✓ Added Victron as peer: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                     victronMAC[0], victronMAC[1], victronMAC[2], victronMAC[3], victronMAC[4], victronMAC[5]);
    } else {
        Serial.println("✗ Failed to add Victron as peer!\n");
    }

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/test", handleTest);
    server.on("/fridge", handleFridge);
    server.on("/fridge/cmd", handleFridgeCommand);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("✓ Web server started\n");

    Serial.println("========================================");
    Serial.println("Connect to WiFi: PowerMonitor / 12345678");
    Serial.println("Then browse to: http://192.168.4.1");
    Serial.println("========================================\n");
}

// ============ LOOP ============
void loop() {
    server.handleClient();
    yield();
}
