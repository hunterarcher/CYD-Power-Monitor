#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <algorithm>
#include "VictronData.h"
#include "DynamicInventory.h"

// ============ GLOBAL INVENTORY ============
std::vector<DynamicCategory> inventory;

// ============ SMART DEFAULTS FOR TRAILER STATUS ============
bool shouldLiveInTrailer(const String& itemName, const String& categoryName) {
    String item = itemName;
    String category = categoryName;
    item.toLowerCase();
    category.toLowerCase();
    
    // Items that should always be TRIP items (bought fresh)
    if (item.indexOf("milk") >= 0 || item.indexOf("bread") >= 0 || item.indexOf("fresh") >= 0) return false;
    if (item.indexOf("meat") >= 0 || item.indexOf("fish") >= 0 || item.indexOf("chicken") >= 0) return false;
    if (item.indexOf("fruit") >= 0 || item.indexOf("vegetable") >= 0 || item.indexOf("lettuce") >= 0) return false;
    if (item.indexOf("yogurt") >= 0 || item.indexOf("cheese") >= 0 || item.indexOf("cream") >= 0) return false;
    if (item.indexOf("frozen") >= 0 || item.indexOf("ice") >= 0 || item.indexOf("beer") >= 0) return false;
    if (item.indexOf("juice") >= 0 || item.indexOf("soda") >= 0) return false;
    
    // Items that should always be TRAILER items (non-perishable supplies)
    if (item.indexOf("spice") >= 0 || item.indexOf("herb") >= 0 || item.indexOf("seasoning") >= 0) return true;
    if (item.indexOf("oil") >= 0 || item.indexOf("vinegar") >= 0 || item.indexOf("sauce") >= 0) return true;
    if (item.indexOf("salt") >= 0 || item.indexOf("pepper") >= 0 || item.indexOf("sugar") >= 0) return true;
    if (item.indexOf("flour") >= 0 || item.indexOf("rice") >= 0 || item.indexOf("pasta") >= 0) return true;
    if (item.indexOf("can") >= 0 || item.indexOf("canned") >= 0 || item.indexOf("jar") >= 0) return true;
    if (item.indexOf("battery") >= 0 || item.indexOf("batteries") >= 0) return true;
    if (item.indexOf("paper") >= 0 || item.indexOf("towel") >= 0 || item.indexOf("tissue") >= 0) return true;
    if (item.indexOf("soap") >= 0 || item.indexOf("detergent") >= 0 || item.indexOf("cleaner") >= 0) return true;
    if (item.indexOf("first aid") >= 0 || item.indexOf("medicine") >= 0 || item.indexOf("bandage") >= 0) return true;
    if (item.indexOf("tool") >= 0 || item.indexOf("flashlight") >= 0 || item.indexOf("lighter") >= 0) return true;
    
    // Category-based defaults
    if (category.indexOf("cleaning") >= 0) return true;
    if (category.indexOf("tool") >= 0) return true;
    if (category.indexOf("first aid") >= 0) return true;
    if (category.indexOf("maintenance") >= 0) return true;
    if (category.indexOf("food") >= 0 && category.indexOf("beverage") >= 0) return false; // Fresh food category
    
    // Default: trailer items (conservative approach - easier to change individual items)
    return true;
}

// ============ FORWARD DECLARATIONS ============
bool createBackup(String backupName);
void rotateBackups();
void cleanupOldBackups();
String getStorageInfo();
void handleInventoryDeleteBackup();
void handleInventoryAddCategory();
void handleInventoryRenameItem();
void handleInventoryEditConsumable();
void handleInventoryMoveItem();
void handleInventoryDeleteItem();
void handleCategoryRename();
void handleCategoryDelete();
String createStyledConfirmationPage(String title, String icon, String message, String buttonText, String buttonUrl, String buttonColor = "primary");

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
#define MASTER_QUEUE_SIZE 10
struct QueuedCommand {
    uint8_t device;
    uint8_t command;
    int16_t value1;
    int16_t value2;
};
QueuedCommand masterQueue[MASTER_QUEUE_SIZE];
uint8_t masterQueueHead = 0;
uint8_t masterQueueTail = 0;
uint8_t masterQueueCount = 0;
uint32_t nextCommandId = 1;
unsigned long lastSendAttempt = 0;

// ============ FUNCTION DECLARATIONS ============
bool saveInventoryToSPIFFS();
void loadInventoryFromSPIFFS();
void initializeDefaultInventory();
void sortCategoryItems(DynamicCategory& category);
void sortAllInventory();

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

// Queue a command for sending
bool queueCommand(uint8_t device, uint8_t command, int16_t value1, int16_t value2) {
    if (masterQueueCount >= MASTER_QUEUE_SIZE) {
        Serial.println("[QUEUE] ✗ Queue full! Cannot add command");
        return false;
    }

    masterQueue[masterQueueTail].device = device;
    masterQueue[masterQueueTail].command = command;
    masterQueue[masterQueueTail].value1 = value1;
    masterQueue[masterQueueTail].value2 = value2;

    masterQueueTail = (masterQueueTail + 1) % MASTER_QUEUE_SIZE;
    masterQueueCount++;

    Serial.printf("[QUEUE] ✓ Command queued (count: %d)\n", masterQueueCount);
    return true;
}

// Process queued commands (called from loop)
void processMasterQueue() {
    if (masterQueueCount == 0) return;
    if (!victronReady) return;

    // Throttle sends to once per 100ms
    if (millis() - lastSendAttempt < 100) return;
    lastSendAttempt = millis();

    // Send next command from queue
    QueuedCommand cmd = masterQueue[masterQueueHead];
    masterQueueHead = (masterQueueHead + 1) % MASTER_QUEUE_SIZE;
    masterQueueCount--;

    ControlCommand espCmd;
    espCmd.commandId = nextCommandId++;
    espCmd.device = cmd.device;
    espCmd.command = cmd.command;
    espCmd.value1 = cmd.value1;
    espCmd.value2 = cmd.value2;
    espCmd.timestamp = millis();

    Serial.printf("[SEND] Sending command #%d (device=%d, cmd=%d) | Queue remaining: %d\n",
                 espCmd.commandId, cmd.device, cmd.command, masterQueueCount);

    esp_now_send(victronMAC, (uint8_t*)&espCmd, sizeof(espCmd));
}

// ============ WEB SERVER HANDLERS ============

void handleRoot() {
    bool dataRecent = (millis() - lastReceived) < 60000;
    unsigned long now = millis();

    // Pre-allocate string for better performance
    String html;
    html.reserve(4096);  // Reserve ~4KB to reduce reallocations

    html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'>";
    // PWA and iOS web app support
    html += "<meta name='apple-mobile-web-app-capable' content='yes'>";
    html += "<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent'>";
    html += "<meta name='apple-mobile-web-app-title' content='Trailer'>";
    html += "<meta name='mobile-web-app-capable' content='yes'>";
    html += "<link rel='manifest' href='/manifest.json'>";
    html += "<link rel='apple-touch-icon' href='/icon-192.png'>";
    html += "<title>Trailer Dashboard</title>";

    // Beautiful gradient-enhanced layout (matching fridge page style)
    html += "<style>";
    html += "*{-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}";
    html += "body{background:#000;color:#fff;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;margin:0;padding:8px;line-height:1.3;font-size:15px}";

    // Cards with gradients and depth
    html += ".c{background:linear-gradient(135deg,#1a1a1a 0%,#111 100%);margin:8px 0;padding:10px;border-radius:12px;border-left:3px solid #333;box-shadow:0 4px 6px rgba(0,0,0,0.3);position:relative}";
    html += ".c.online{border-left-color:#4f4;box-shadow:0 4px 6px rgba(79,244,79,0.2)}";
    html += ".c.warn{border-left-color:#f80;box-shadow:0 4px 6px rgba(255,136,0,0.2)}";
    html += ".c.critical{border-left-color:#f22;box-shadow:0 4px 6px rgba(255,34,34,0.2)}";
    html += ".c.emergency{border-left-color:#a0f;box-shadow:0 4px 6px rgba(170,0,255,0.2)}";
    html += ".c.offline{border-left-color:#666;opacity:0.5}";

    // Headers with icons - slightly larger and better contrast
    html += "h2{margin:0 0 6px;font-size:1.05em;color:#ddd;display:flex;align-items:center;gap:8px;font-weight:600}";
    html += ".icon{font-size:1.3em;filter:drop-shadow(0 2px 4px rgba(0,0,0,0.5))}";

    // Main values with gradient backgrounds (compact size)
    html += ".v{font-size:1.5em;font-weight:bold;margin:4px 0;line-height:1;padding:5px 8px;border-radius:8px;display:inline-block;background:linear-gradient(135deg,#4f4,#2d2);color:#000;box-shadow:0 2px 4px rgba(79,244,79,0.3)}";
    html += ".v.warn{background:linear-gradient(135deg,#f80,#d60);color:#000;box-shadow:0 2px 4px rgba(255,136,0,0.3)}";
    html += ".v.critical{background:linear-gradient(135deg,#f22,#d00);color:#fff;box-shadow:0 2px 4px rgba(255,34,34,0.3)}";
    html += ".v.emergency{background:linear-gradient(135deg,#a0f,#80d);color:#fff;box-shadow:0 2px 4px rgba(170,0,255,0.3)}";

    // Side-by-side layout: big value left, grid right (larger text)
    html += ".content{display:flex;gap:15px;align-items:center}";
    html += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:6px 12px;font-size:0.95em;flex:1}";
    html += ".item{display:flex;flex-direction:column}";
    html += ".label{color:#999;font-size:0.8em;margin-bottom:2px;font-weight:500}";
    html += ".value{color:#fff;font-size:1.05em;font-weight:500}";

    // Value color coding
    html += ".value.voltage{color:#4af}";
    html += ".value.current{color:#fff}";
    html += ".value.power{color:#fa0}";
    html += ".value.temp{color:#0ff}";

    // Badges with animations
    html += ".badge{background:#444;padding:2px 6px;border-radius:6px;font-size:0.68em;color:#aaa;margin-left:5px}";
    html += ".badge.on{background:linear-gradient(135deg,#282,#161);color:#4f4;animation:pulse 2s infinite}";
    html += ".badge.charging{background:linear-gradient(135deg,#248,#136);color:#4af;animation:pulse 2s infinite}";
    html += "@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.7}}";

    // Tablet/iPad optimization (768px and wider)
    html += "@media(min-width:768px){";
    html += "body{padding:10px 15px;max-width:1200px;margin:0 auto;font-size:16px}";
    html += ".c{margin:8px 0;padding:12px;border-radius:12px;border-left-width:4px}";
    html += "h2{font-size:1.15em;margin-bottom:6px}";
    html += ".icon{font-size:1.3em}";
    html += ".v{font-size:2em;padding:6px 12px}";
    html += ".content{gap:10px}";
    html += ".grid{gap:6px 12px;font-size:1em}";
    html += ".label{font-size:0.8em}";
    html += ".value{font-size:1.1em}";
    html += ".badge{font-size:0.7em;padding:3px 8px}";
    html += "a{padding:14px!important;font-size:1.1em!important}";
    html += "}";
    html += "@media(min-width:768px) and (orientation:landscape){";
    html += "body{padding:8px 12px}";
    html += ".c{margin:6px 0;padding:10px}";
    html += "}";

    html += "</style>";
    html += "<script>";
    html += "if('serviceWorker' in navigator){";
    html += "navigator.serviceWorker.register('/sw.js').catch(e=>console.log('SW registration failed'));";
    html += "}";
    html += "</script>";
    html += "<meta http-equiv='refresh' content='5'>";  // Auto-refresh every 5 seconds
    html += "</head><body>";

    // Battery with warning levels
    html += "<div class='c ";
    if (dataRecent && latestData.bmv.valid) {
        float soc = latestData.bmv.soc;
        int ttg = latestData.bmv.timeToGo;
        if (soc <= 10 || (ttg > 0 && ttg <= 30)) html += "emergency";
        else if (soc <= 20 || (ttg > 0 && ttg <= 60)) html += "critical";
        else if (soc <= 50 || (ttg > 0 && ttg <= 120)) html += "warn";
        else html += "online";
    } else {
        html += "offline";
    }
    html += "'><h2><span class='icon'>\u{1F50B}</span>KARSTEN MAXI SHUNT<span class='badge'>BMV-712</span></h2>";
    if (dataRecent && latestData.bmv.valid) {
        html += "<div class='content'>";

        // Main value with warning color
        float soc = latestData.bmv.soc;
        int ttg = latestData.bmv.timeToGo;
        html += "<div class='v";
        if (soc <= 10 || (ttg > 0 && ttg <= 30)) html += " emergency";
        else if (soc <= 20 || (ttg > 0 && ttg <= 60)) html += " critical";
        else if (soc <= 50 || (ttg > 0 && ttg <= 120)) html += " warn";
        html += "'>" + String(latestData.bmv.soc, 0) + "%</div>";

        // 2-column grid with color-coded values
        html += "<div class='grid'>";
        html += "<div class='item'><div class='label'>Voltage</div><div class='value voltage'>" + String(latestData.bmv.voltage, 2) + "V</div></div>";
        html += "<div class='item'><div class='label'>Current</div><div class='value current'>" + String(latestData.bmv.current, 2) + "A</div></div>";

        if (latestData.bmv.timeToGo > 0 && latestData.bmv.timeToGo < 1440) {
            int hours = latestData.bmv.timeToGo / 60;
            int mins = latestData.bmv.timeToGo % 60;
            html += "<div class='item'><div class='label'>Time remaining</div><div class='value'>" + String(hours) + "h " + String(mins) + "m</div></div>";
        } else {
            html += "<div class='item'><div class='label'>Time remaining</div><div class='value'>--</div></div>";
        }

        html += "<div class='item'><div class='label'>Consumed</div><div class='value'>" + String(latestData.bmv.consumedAh, 1) + "Ah</div></div>";
        html += "</div></div>";
    } else {
        html += "<div class='v'>--</div>";
        html += "<div style='font-size:0.9em;color:#888'>Offline</div>";
    }
    html += "</div>";

    // Solar
    html += "<div class='c ";
    html += (dataRecent && latestData.mppt.valid) ? "online" : "offline";
    html += "'><h2><span class='icon'>\u{2600}\u{FE0F}</span>KARSTEN MAXI SOLAR<span class='badge'>MPPT 100/20</span>";
    if (dataRecent && latestData.mppt.valid) {
        // Add charging state badge in header
        if (latestData.mppt.state == 3) html += "<span class='badge charging'>Bulk</span>";
        else if (latestData.mppt.state == 4) html += "<span class='badge charging'>Absorption</span>";
        else if (latestData.mppt.state == 5) html += "<span class='badge on'>Float</span>";
        else if (latestData.mppt.state == 0) html += "<span class='badge'>Off</span>";
    }
    html += "</h2>";
    if (dataRecent && latestData.mppt.valid) {
        html += "<div class='content'>";
        html += "<div class='v'>" + String((int)latestData.mppt.solarPower) + "W</div>";

        // 2-column grid
        html += "<div class='grid'>";
        html += "<div class='item'><div class='label'>Voltage</div><div class='value voltage'>" + String(latestData.mppt.batteryVoltage, 2) + "V</div></div>";
        html += "<div class='item'><div class='label'>Current</div><div class='value current'>" + String(latestData.mppt.batteryCurrent, 2) + "A</div></div>";
        html += "<div class='item'><div class='label'>Yield today</div><div class='value power'>" + String(latestData.mppt.yieldToday, 2) + "kWh</div></div>";
        html += "</div></div>";
    } else {
        html += "<div class='v'>--</div>";
        html += "<div style='font-size:0.9em;color:#888'>Offline</div>";
    }
    html += "</div>";

    // AC Charger
    html += "<div class='c ";
    html += (dataRecent && latestData.ip22.valid) ? "online" : "offline";
    html += "'><h2><span class='icon'>\u{1F50C}</span>KARSTEN MAXI AC<span class='badge'>IP22 12|20</span>";
    if (dataRecent && latestData.ip22.valid) {
        // Add charging state badge in header
        if (latestData.ip22.state == 3) html += "<span class='badge charging'>Bulk</span>";
        else if (latestData.ip22.state == 4) html += "<span class='badge charging'>Absorption</span>";
        else if (latestData.ip22.state == 5) html += "<span class='badge on'>Float</span>";
        else if (latestData.ip22.state == 2) html += "<span class='badge on'>Storage</span>";
        else if (latestData.ip22.state == 0) html += "<span class='badge'>Off</span>";
    }
    html += "</h2>";
    if (dataRecent && latestData.ip22.valid) {
        html += "<div class='content'>";
        html += "<div class='v'>" + String(latestData.ip22.power, 0) + "W</div>";

        // 2-column grid
        html += "<div class='grid'>";
        html += "<div class='item'><div class='label'>Voltage</div><div class='value voltage'>" + String(latestData.ip22.batteryVoltage, 2) + "V</div></div>";
        html += "<div class='item'><div class='label'>Current</div><div class='value current'>" + String(latestData.ip22.batteryCurrent, 2) + "A</div></div>";
        if (latestData.ip22.temperature > 0) {
            html += "<div class='item'><div class='label'>Temp</div><div class='value temp'>" + String(latestData.ip22.temperature, 0) + "°C</div></div>";
        }
        html += "</div></div>";
    } else {
        html += "<div class='v'>--</div>";
        html += "<div style='font-size:0.9em;color:#888'>Offline</div>";
    }
    html += "</div>";

    // EcoFlow with warning levels
    html += "<div class='c ";
    if (dataRecent && latestData.ecoflow.valid) {
        int pct = latestData.ecoflow.batteryPercent;
        if (pct <= 10) html += "emergency";
        else if (pct <= 20) html += "critical";
        else if (pct <= 50) html += "warn";
        else html += "online";
    } else {
        html += "offline";
    }
    html += "'><h2><span class='icon'>\u{26A1}</span>EcoFlow<span class='badge'>DELTA Max 2</span></h2>";
    if (dataRecent && latestData.ecoflow.valid) {
        html += "<div class='content'>";

        // Main value with warning color
        int pct = latestData.ecoflow.batteryPercent;
        html += "<div class='v";
        if (pct <= 10) html += " emergency";
        else if (pct <= 20) html += " critical";
        else if (pct <= 50) html += " warn";
        html += "'>" + String(pct) + "%</div>";

        // 2-column grid
        html += "<div class='grid'>";
        html += "<div class='item'><div class='label'>RSSI</div><div class='value current'>" + String(latestData.ecoflow.rssi) + " dBm</div></div>";

        // Show serial number if available
        if (latestData.ecoflow.serialNumber[0] != 0) {
            html += "<div class='item'><div class='label'>Serial</div><div class='value' style='font-size:0.8em'>";
            for (int i = 0; i < 16 && latestData.ecoflow.serialNumber[i] != 0; i++) {
                html += (char)latestData.ecoflow.serialNumber[i];
            }
            html += "</div></div>";
        }
        html += "</div></div>";
    } else {
        html += "<div class='v'>--</div>";
        html += "<div style='font-size:0.9em;color:#888'>Offline</div>";
    }
    html += "</div>";

    // Fridge with temperature spike warnings
    bool hasFridgeData = dataRecent && latestData.fridge.valid && latestData.fridge.connected;
    html += "<div class='c ";
    if (hasFridgeData) {
        // Check for temperature spikes (door open, sun, etc)
        int leftDiff = abs(latestData.fridge.left_actual - latestData.fridge.left_setpoint);
        int rightDiff = abs(latestData.fridge.right_actual - latestData.fridge.right_setpoint);
        int maxDiff = max(leftDiff, rightDiff);

        if (maxDiff >= 10) html += "critical";  // >10°C = critical
        else if (maxDiff >= 5) html += "warn";   // >5°C = warning
        else html += "online";
    } else {
        html += "offline";
    }
    html += "' onclick=\"window.location.href='/fridge'\" style='cursor:pointer;transition:transform 0.2s'>";
    html += "<h2><span class='icon'>\u{2744}\u{FE0F}</span>Fridge<span class='badge'>Flex Adventure 95L K.I.D</span> <span style='margin-left:auto;font-size:1.2em'>→</span></h2>";
    if (hasFridgeData) {
        // 4 blocks in a row: LEFT | RIGHT | ECO | BATTERY
        html += "<div class='fridge-blocks' style='display:flex;gap:8px;justify-content:space-between'>";

        // LEFT temp block
        int leftDiff = abs(latestData.fridge.left_actual - latestData.fridge.left_setpoint);
        html += "<div style='flex:1;text-align:center;background:rgba(79,244,79,0.05);padding:8px;border-radius:8px'>";
        html += "<div class='label' style='font-size:0.75em;margin-bottom:4px'>LEFT</div>";
        html += "<div class='temp-value' style='font-size:1.4em;font-weight:bold;margin:2px 0;padding:4px 8px;border-radius:6px;display:inline-block;";
        if (leftDiff >= 10) html += "background:linear-gradient(135deg,#f22,#d00);color:#fff";
        else if (leftDiff >= 5) html += "background:linear-gradient(135deg,#f80,#d60);color:#000";
        else html += "background:linear-gradient(135deg,#4af,#28d);color:#000";
        html += "'>" + String(latestData.fridge.left_actual) + "°C</div>";
        html += "<div class='temp-setpoint' style='font-size:0.75em;color:#aaa;margin-top:4px'>→ " + String(latestData.fridge.left_setpoint) + "°C</div>";
        html += "</div>";

        // RIGHT temp block
        int rightDiff = abs(latestData.fridge.right_actual - latestData.fridge.right_setpoint);
        html += "<div style='flex:1;text-align:center;background:rgba(68,170,255,0.05);padding:8px;border-radius:8px'>";
        html += "<div class='label' style='font-size:0.75em;margin-bottom:4px'>RIGHT</div>";
        html += "<div class='temp-value' style='font-size:1.4em;font-weight:bold;margin:2px 0;padding:4px 8px;border-radius:6px;display:inline-block;";
        if (rightDiff >= 10) html += "background:linear-gradient(135deg,#f22,#d00);color:#fff";
        else if (rightDiff >= 5) html += "background:linear-gradient(135deg,#f80,#d60);color:#000";
        else html += "background:linear-gradient(135deg,#0ff,#0dd);color:#000";
        html += "'>" + String(latestData.fridge.right_actual) + "°C</div>";
        html += "<div class='temp-setpoint' style='font-size:0.75em;color:#aaa;margin-top:4px'>→ " + String(latestData.fridge.right_setpoint) + "°C</div>";
        html += "</div>";

        // ECO block
        html += "<div style='flex:1;text-align:center;padding:8px;border-radius:8px;";
        html += latestData.fridge.eco_mode ? "background:linear-gradient(135deg,#282,#161)" : "background:#f8f8f8";
        html += "'>";
        html += "<div class='icon-block' style='font-size:1.8em;margin-bottom:2px'>\u{1F343}</div>";
        html += "<div class='icon-label' style='font-size:0.85em;font-weight:600;";
        html += latestData.fridge.eco_mode ? "color:#4f4" : "color:#333";
        html += "'>ECO</div>";
        html += "<div class='icon-status' style='font-size:0.7em;";
        html += latestData.fridge.eco_mode ? "color:#4f4" : "color:#666";
        html += "'>" + String(latestData.fridge.eco_mode ? "ON" : "OFF") + "</div>";
        html += "</div>";

        // BATTERY block
        html += "<div style='flex:1;text-align:center;background:#222;padding:8px;border-radius:8px'>";
        html += "<div class='icon-block' style='font-size:1.8em;margin-bottom:2px'>\u{1F50B}</div>";
        html += "<div class='icon-label' style='font-size:0.85em;font-weight:600;color:#4af'>BAT</div>";
        html += "<div class='icon-status' style='font-size:0.7em;color:#aaa'>";
        if (latestData.fridge.battery_protection == 0) html += "L";
        else if (latestData.fridge.battery_protection == 1) html += "M";
        else html += "H";
        html += "</div>";
        html += "</div>";

        html += "</div>";
    } else {
        html += "<div class='v'>--</div>";
        html += "<div style='font-size:0.9em;color:#888'>Offline</div>";
    }
    html += "</div>";

    // Navigation buttons
    html += "<div style='margin-top:15px;display:flex;gap:8px'>";
    html += "<div onclick=\"window.location.href='/fridge'\" style='flex:1;padding:12px;background:linear-gradient(135deg,#667eea,#764ba2);border-radius:10px;text-align:center;color:#fff;font-weight:600;cursor:pointer'>\u{1F9CA} Fridge</div>";
    html += "<div onclick=\"window.location.href='/inventory'\" style='flex:1;padding:12px;background:linear-gradient(135deg,#667eea,#764ba2);border-radius:10px;text-align:center;color:#fff;font-weight:600;cursor:pointer'>\u{1F69A} Inventory</div>";
    html += "</div>";

    // Footer
    html += "<div style='margin-top:10px;padding:6px;background:#111;border-radius:4px;font-size:0.7em;color:#666;text-align:center'>";
    unsigned long secondsAgo = (millis() - lastReceived) / 1000;
    html += "Pkts: " + String(packetsReceived) + " | ";
    if (secondsAgo < 60) {
        html += String(secondsAgo) + "s";
    } else {
        html += String(secondsAgo / 60) + "m";
    }
    html += " | Up: ";
    unsigned long uptimeSeconds = millis() / 1000;
    unsigned long days = uptimeSeconds / 86400;
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    if (days > 0) html += String(days) + "d ";
    html += String(hours) + "h";
    html += "</div>";

    html += "</body></html>";

    server.send(200, "text/html", html);
}

void handleFridge() {
    bool dataRecent = (millis() - lastReceived) < 60000;
    bool hasFridgeData = dataRecent && latestData.fridge.valid && latestData.fridge.connected;

    // Get current state
    int leftActual = hasFridgeData ? latestData.fridge.left_actual : 0;
    int rightActual = hasFridgeData ? latestData.fridge.right_actual : 0;
    int leftSet = hasFridgeData ? latestData.fridge.left_setpoint : 4;
    int rightSet = hasFridgeData ? latestData.fridge.right_setpoint : -18;
    bool ecoMode = hasFridgeData ? latestData.fridge.eco_mode : false;
    uint8_t batProt = hasFridgeData ? latestData.fridge.battery_protection : 2;

    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'>";
    html += "<meta name='apple-mobile-web-app-capable' content='yes'>";
    html += "<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent'>";
    html += "<meta name='apple-mobile-web-app-title' content='Trailer'>";
    html += "<meta name='mobile-web-app-capable' content='yes'>";
    html += "<link rel='manifest' href='/manifest.json'>";
    html += "<link rel='apple-touch-icon' href='/icon-192.png'>";
    html += "<title>Fridge Control</title>";
    html += "<style>";
    html += "*{-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}";
    html += "body{background:#000;color:#fff;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;margin:0;padding:10px}";
    html += "a{color:#4af;text-decoration:none;padding:8px 16px;background:#222;border-radius:4px;display:inline-block;margin:5px}";

    // Zone selector cards - more compact
    html += ".zone{display:flex;gap:10px;margin:15px 0}";
    html += ".zcard{flex:1;padding:15px;border-radius:10px;text-align:center;cursor:pointer;border:2px solid #444}";
    html += ".zcard.sel{background:#1a1a1a;color:#fff;border-color:#4af}";
    html += ".zcard.unsel{background:#f8f8f8;color:#333;border-color:#ddd}";
    html += ".ztemp{font-size:2em;font-weight:bold;margin:4px 0}";
    html += ".zlabel{font-size:0.85em;color:#888;margin-bottom:6px}";

    // Slider - thinner and longer
    html += ".slider-container{margin:20px 0}";
    html += ".slider-wrapper{position:relative;margin:15px 30px}";
    html += ".slider{width:100%;height:12px;border-radius:6px;background:linear-gradient(to right,#00bfff,#00ff7f,#ffff00,#ffa500,#ff4500);-webkit-appearance:none;outline:none}";
    html += ".slider::-webkit-slider-thumb{-webkit-appearance:none;width:40px;height:40px;border-radius:6px;background:#1a1a1a;border:3px solid #4af;cursor:pointer;position:relative}";
    html += ".slider::-moz-range-thumb{width:40px;height:40px;border-radius:6px;background:#1a1a1a;border:3px solid #4af;cursor:pointer}";
    html += ".temp-display{text-align:center;font-size:1.8em;margin:10px 0;font-weight:bold}";
    html += ".temp-labels{display:flex;justify-content:space-between;font-size:0.85em;color:#888;margin-top:5px}";

    // Toggle buttons
    html += ".toggle-row{display:flex;gap:10px;margin:25px 0;justify-content:center}";
    html += ".tbtn{padding:15px 25px;border-radius:12px;border:2px solid #444;cursor:pointer;text-align:center;min-width:80px}";
    html += ".tbtn.on{background:#1a1a1a;color:#fff;border-color:#4af}";
    html += ".tbtn.off{background:#f8f8f8;color:#333;border-color:#ddd}";
    html += ".icon{font-size:1.5em;margin-bottom:5px}";
    html += ".tlabel{font-size:0.8em}";

    // Tablet/iPad optimization (768px and wider)
    html += "@media(min-width:768px){";
    html += "body{padding:15px 20px;max-width:900px;margin:0 auto}";
    html += "a{padding:10px 18px;font-size:1em}";
    html += ".zcard{padding:18px}";
    html += ".ztemp{font-size:2.8em}";
    html += ".zlabel{font-size:1em}";
    html += ".slider{height:45px}";
    html += ".slider::-webkit-slider-thumb{width:70px;height:70px}";
    html += ".slider::-moz-range-thumb{width:70px;height:70px}";
    html += ".temp-display{font-size:2.2em;margin:15px 0}";
    html += ".temp-labels{font-size:1em}";
    html += ".tbtn{padding:16px 28px;font-size:1em}";
    html += ".icon{font-size:1.6em}";
    html += "}";

    html += "</style>";
    html += "<script>";
    html += "var currentZone='left';";
    html += "var leftTemp=" + String(leftSet) + ";";
    html += "var rightTemp=" + String(rightSet) + ";";
    html += "var ecoState=" + String(ecoMode ? "1" : "0") + ";";
    html += "var batLevel=" + String(batProt) + ";";

    // Zone selection
    html += "function selectZone(zone){currentZone=zone;updateDisplay();}";

    // Temperature slider
    html += "function updateTemp(val){if(currentZone=='left'){leftTemp=parseInt(val);}else{rightTemp=parseInt(val);}updateDisplay();}";
    html += "function adjustTemp(delta){var newTemp;if(currentZone=='left'){newTemp=Math.max(-20,Math.min(20,leftTemp+delta));leftTemp=newTemp;}else{newTemp=Math.max(-20,Math.min(20,rightTemp+delta));rightTemp=newTemp;}updateDisplay();}";
    html += "function updateDisplay(){document.getElementById('leftCard').className=currentZone=='left'?'zcard sel':'zcard unsel';";
    html += "document.getElementById('rightCard').className=currentZone=='right'?'zcard sel':'zcard unsel';";
    html += "var temp=currentZone=='left'?leftTemp:rightTemp;document.getElementById('slider').value=temp;document.getElementById('tempDisp').innerText=temp+'°C';}";
    html += "function setTemp(){var zone=currentZone=='left'?0:1;var temp=currentZone=='left'?leftTemp:rightTemp;";
    html += "showStatus('Setting temperature...');setTimeout(function(){window.location.href='/fridge/cmd?zone='+zone+'&temp='+temp;},100);}";

    // ECO toggle (visual only, needs SET)
    html += "function toggleEco(){ecoState=ecoState=='1'?'0':'1';document.getElementById('ecoBtn').className='tbtn '+(ecoState=='1'?'on':'off');}";
    html += "function setEco(){showStatus('Setting ECO mode...');setTimeout(function(){window.location.href='/fridge/eco?state='+ecoState;},100);}";

    // Battery protection (visual only, needs SET)
    html += "function selectBat(level){batLevel=level;document.getElementById('batL').className='tbtn '+(level==0?'on':'off');";
    html += "document.getElementById('batM').className='tbtn '+(level==1?'on':'off');";
    html += "document.getElementById('batH').className='tbtn '+(level==2?'on':'off');}";
    html += "function setBat(){showStatus('Setting battery protection...');setTimeout(function(){window.location.href='/fridge/battery?level='+batLevel;},100);}";

    // Status message
    html += "function showStatus(msg,color){var s=document.getElementById('status');if(s){s.innerText=msg;s.style.backgroundColor=color||'#248';s.style.display='block';}}";
    html += "function refresh(){location.reload();}";

    // Smart polling after command sent
    html += "var checkCount=0;";
    html += "var expectedTemp=" + String(leftSet) + ";";
    html += "var expectedEco=" + String(ecoMode ? "1" : "0") + ";";
    html += "var expectedBat=" + String(batProt) + ";";
    html += "var checkType='';";

    html += "function checkUpdate(){";
    html += "checkCount++;";
    html += "if(checkCount>12){showStatus('Update timeout - please refresh manually','#f44');return;}";
    html += "fetch('/fridge/status').then(r=>r.json()).then(data=>{";
    html += "var updated=false;";
    html += "if(checkType=='temp'){";
    html += "var currentTemp=currentZone=='left'?data.leftSet:data.rightSet;";
    html += "if(currentTemp==expectedTemp){updated=true;showStatus('Temperature updated!','#4a4');}";
    html += "}else if(checkType=='eco'){";
    html += "if((data.eco?'1':'0')==expectedEco){updated=true;showStatus('ECO mode updated!','#4a4');}";
    html += "}else if(checkType=='bat'){";
    html += "if(data.bat==expectedBat){updated=true;showStatus('Battery protection updated!','#4a4');}";
    html += "}";
    html += "if(updated){setTimeout(function(){window.location.href='/fridge';},1500);}";
    html += "else{showStatus('Waiting for update... ('+checkCount+'/12)','#248');setTimeout(checkUpdate,2000);}";
    html += "}).catch(e=>{setTimeout(checkUpdate,2000);});";
    html += "}";

    // Check if we just sent a command
    html += "var params=new URLSearchParams(window.location.search);";
    html += "if(params.has('zone')){checkType='temp';expectedTemp=parseInt(params.get('temp'));showStatus('Setting temperature...');setTimeout(checkUpdate,3000);}";
    html += "else if(params.has('state')){checkType='eco';expectedEco=params.get('state');showStatus('Setting ECO mode...');setTimeout(checkUpdate,3000);}";
    html += "else if(params.has('level')){checkType='bat';expectedBat=parseInt(params.get('level'));showStatus('Setting battery protection...');setTimeout(checkUpdate,3000);}";
    html += "</script></head><body>";

    html += "<button style='padding:8px 16px;background:#4af;color:#000;border:none;border-radius:8px;cursor:pointer;font-weight:bold;margin-bottom:10px' onclick='refresh()'>🔄 Refresh</button><hr>";

    // Status message
    html += "<div id='status' style='display:none;padding:12px;margin:10px 0;background:#248;color:#fff;border-radius:8px;text-align:center;font-weight:bold'></div>";

    // Zone selector cards
    html += "<div class='zone'>";
    html += "<div id='leftCard' class='zcard sel' onclick='selectZone(\"left\")'>";
    html += "<div class='zlabel'>Current temp</div>";
    html += "<div style='font-size:1.2em'>Left " + String(leftActual) + "°C</div>";
    html += "</div>";
    html += "<div id='rightCard' class='zcard unsel' onclick='selectZone(\"right\")'>";
    html += "<div class='zlabel'>Current temp</div>";
    html += "<div style='font-size:1.2em'>Right " + String(rightActual) + "°C</div>";
    html += "</div>";
    html += "</div>";

    // Temperature slider with increment buttons
    html += "<div class='slider-container'>";
    html += "<div class='temp-display' id='tempDisp'>" + String(leftSet) + "°C</div>";

    // Quick adjust buttons
    html += "<div style='display:flex;gap:8px;justify-content:center;margin:10px 0'>";
    html += "<button onclick='adjustTemp(-5)' style='flex:1;padding:12px 20px;background:#4af;color:#000;border:none;border-radius:12px;cursor:pointer;font-weight:bold'>-5</button>";
    html += "<button onclick='adjustTemp(-1)' style='flex:1;padding:12px 20px;background:#4af;color:#000;border:none;border-radius:12px;cursor:pointer;font-weight:bold'>-1</button>";
    html += "<button onclick='adjustTemp(1)' style='flex:1;padding:12px 20px;background:#4af;color:#000;border:none;border-radius:12px;cursor:pointer;font-weight:bold'>+1</button>";
    html += "<button onclick='adjustTemp(5)' style='flex:1;padding:12px 20px;background:#4af;color:#000;border:none;border-radius:12px;cursor:pointer;font-weight:bold'>+5</button>";
    html += "</div>";

    html += "<div class='slider-wrapper'>";
    html += "<input type='range' min='-20' max='20' step='1' value='" + String(leftSet) + "' class='slider' id='slider' oninput='updateTemp(this.value)'>";
    html += "<div class='temp-labels'><span>-20°C</span><span>20°C</span></div>";
    html += "</div>";
    html += "<button style='width:90%;padding:12px;font-size:1.1em;background:#4af;color:#000;border:none;border-radius:12px;margin:15px 5%;cursor:pointer;font-weight:bold' onclick='setTemp()'>SET TEMPERATURE</button>";
    html += "</div>";

    // ECO control with SET button
    html += "<div style='text-align:center;margin:20px 0'>";
    html += "<div style='font-size:0.9em;color:#888;margin-bottom:10px'>ECO MODE</div>";
    html += "<div class='toggle-row' style='justify-content:center;margin-bottom:10px'>";
    html += "<div id='ecoBtn' class='tbtn " + String(ecoMode ? "on" : "off") + "' onclick='toggleEco()' style='min-width:120px'>";
    html += "<div class='icon'>\u{1F343}</div>";  // Leaf emoji
    html += "<div class='tlabel'>ECO</div>";
    html += "</div>";
    html += "</div>";
    html += "<button style='padding:12px 40px;font-size:1.1em;background:#4af;color:#000;border:none;border-radius:12px;cursor:pointer;font-weight:bold' onclick='setEco()'>SET ECO</button>";
    html += "</div>";

    // Status
    if (hasFridgeData) {
        html += "<div style='margin:20px 0;padding:15px;background:#111;border-radius:8px;font-size:0.9em;text-align:center;color:#888'>";
        html += "Last update: " + String((millis() - lastReceived) / 1000) + "s ago";
        html += "</div>";
    } else {
        html += "<p style='color:#f66;font-size:1.1em;text-align:center'>⚠ Fridge offline</p>";
    }

    // Navigation buttons
    html += "<div style='display:flex;gap:8px;margin:20px 0;padding:0 20px'>";
    html += "<button style='flex:1;padding:12px 20px;font-size:1.1em;background:#4af;color:#000;border:none;border-radius:12px;cursor:pointer;font-weight:bold' onclick='window.location.href=\"/\"'>\u{1F3E0} Main Dashboard</button>";
    html += "<button style='flex:1;padding:12px 20px;font-size:1.1em;background:#4af;color:#000;border:none;border-radius:12px;cursor:pointer;font-weight:bold' onclick='window.location.href=\"/inventory\"'>\u{1F4C4} Inventory</button>";
    html += "</div>";

    html += "</body></html>";
    server.sendHeader("Content-Type", "text/html; charset=UTF-8");
    server.send(200, "text/html; charset=UTF-8", html);
}

// ============ PWA SUPPORT HANDLERS ============

void handleManifest() {
    String manifest = R"({
  "name": "Trailer Dashboard",
  "short_name": "Trailer",
  "description": "Complete trailer monitoring and control system",
  "start_url": "/",
  "display": "standalone",
  "orientation": "landscape",
  "background_color": "#000000",
  "theme_color": "#44aaff",
  "icons": [
    {
      "src": "/icon-192.png",
      "sizes": "192x192",
      "type": "image/png",
      "purpose": "any maskable"
    },
    {
      "src": "/icon-512.png", 
      "sizes": "512x512",
      "type": "image/png",
      "purpose": "any maskable"
    }
  ]
})";
    
    server.send(200, "application/json", manifest);
}

void handleServiceWorker() {
    String sw = R"(
// Trailer Dashboard Service Worker
const CACHE_NAME = 'trailer-dashboard-v1';
const urlsToCache = [
  '/',
  '/fridge',
  '/inventory'
];

self.addEventListener('install', function(event) {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(function(cache) {
        return cache.addAll(urlsToCache);
      })
  );
});

self.addEventListener('fetch', function(event) {
  event.respondWith(
    caches.match(event.request)
      .then(function(response) {
        // Return cached version or fetch from network
        return response || fetch(event.request);
      }
    )
  );
});
)";
    
    server.send(200, "application/javascript", sw);
}

void handleIcon192() {
    // Simple PNG icon data (192x192 trailer icon - base64 encoded minimal icon)
    const char* iconData = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg==";
    
    // For now, send a placeholder response - you would store actual icon in SPIFFS
    server.sendHeader("Cache-Control", "max-age=86400"); // Cache for 1 day
    server.send(200, "image/png", "PNG icon placeholder - implement actual icon");
}

void handleIcon512() {
    // Simple PNG icon data (512x512 trailer icon - base64 encoded minimal icon)  
    const char* iconData = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg==";
    
    // For now, send a placeholder response - you would store actual icon in SPIFFS
    server.sendHeader("Cache-Control", "max-age=86400"); // Cache for 1 day
    server.send(200, "image/png", "PNG icon placeholder - implement actual icon");
}

void handleTest() {
    Serial.println("[WEB] TEST button pressed - queueing test command");

    bool success = queueCommand(1, CMD_FRIDGE_SET_TEMP, 0, 4);  // Zone 0, Temp 4°C

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

    Serial.printf("[WEB] Fridge command: Zone %d to %d°C - queueing\n", zone, temp);

    // Use correct command based on zone: 0=LEFT, 1=RIGHT
    uint8_t cmd = (zone == 0) ? CMD_FRIDGE_SET_LEFT_TEMP : CMD_FRIDGE_SET_RIGHT_TEMP;

    // value1 = zone (0 or 1), value2 = temperature
    bool success = queueCommand(1, cmd, zone, temp);

    server.sendHeader("Location", "/fridge?zone=" + String(zone) + "&temp=" + String(temp));
    server.send(303);
}

void handleFridgeEco() {
    // Accept both "eco" and "state" parameters for compatibility
    if (!server.hasArg("eco") && !server.hasArg("state")) {
        server.send(400, "text/plain", "Missing parameter");
        return;
    }

    int8_t eco = server.hasArg("state") ? server.arg("state").toInt() : server.arg("eco").toInt();
    Serial.printf("[WEB] Fridge ECO mode: %s - queueing\n", eco ? "ON" : "OFF");

    queueCommand(1, CMD_FRIDGE_SET_ECO, eco, 0);

    server.sendHeader("Location", "/fridge?state=" + String(eco));
    server.send(303);
}

void handleFridgeBattery() {
    if (!server.hasArg("level")) {
        server.send(400, "text/plain", "Missing parameter");
        return;
    }

    int8_t level = server.arg("level").toInt();
    Serial.printf("[WEB] Fridge battery protection: %d - queueing\n", level);

    queueCommand(1, CMD_FRIDGE_SET_BATTERY, level, 0);

    server.sendHeader("Location", "/fridge?level=" + String(level));
    server.send(303);
}

void handleFridgeStatus() {
    bool dataRecent = (millis() - lastReceived) < 60000;
    bool hasFridgeData = dataRecent && latestData.fridge.valid && latestData.fridge.connected;

    String json = "{";
    if (hasFridgeData) {
        json += "\"leftActual\":" + String(latestData.fridge.left_actual) + ",";
        json += "\"rightActual\":" + String(latestData.fridge.right_actual) + ",";
        json += "\"leftSet\":" + String(latestData.fridge.left_setpoint) + ",";
        json += "\"rightSet\":" + String(latestData.fridge.right_setpoint) + ",";
        json += "\"eco\":" + String(latestData.fridge.eco_mode ? "true" : "false") + ",";
        json += "\"bat\":" + String(latestData.fridge.battery_protection) + ",";
        json += "\"connected\":true";
    } else {
        json += "\"connected\":false";
    }
    json += "}";

    server.sendHeader("Content-Type", "application/json");
    server.send(200, "application/json", json);
}

// ============ INVENTORY HANDLERS ============

void handleTabContent() {
    int tabNum = server.arg("tab").toInt();
    String html = "";
    
    if (tabNum == 1) {
        // Consumables Tab Content
        html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:15px'>";
        html += "<h2 style='margin:0'>\u{1F374} Consumable Items</h2>";
        html += "<div>";
        html += "<button onclick='collapseAll()' title='Collapse All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;margin-right:3px;cursor:pointer;font-size:12px'>▼</button>";
        html += "<button onclick='expandAll()' title='Expand All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;cursor:pointer;font-size:12px'>▶</button>";
        html += "</div></div>";
        
        // Summary tiles
        int okCount = 0, lowCount = 0, outCount = 0;
        for (size_t i = 0; i < inventory.size(); i++) {
            if (!inventory[i].isConsumable) continue;
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                switch (inventory[i].consumables[j].status) {
                    case STATUS_FULL:
                    case STATUS_OK: okCount++; break;
                    case STATUS_LOW: lowCount++; break;
                    case STATUS_OUT: outCount++; break;
                }
            }
        }
        
        html += "<div class='summary-grid' style='grid-template-columns:repeat(3,1fr)'>";
        html += "<div class='summary-card' onclick='filterStatus(0)' style='cursor:pointer'><div class='label'>OK Stock</div><div class='value' style='color:#22c55e'>" + String(okCount) + "</div></div>";
        html += "<div class='summary-card' onclick='filterStatus(1)' style='cursor:pointer'><div class='label'>Low Stock</div><div class='value' style='color:#f59e0b'>" + String(lowCount) + "</div></div>";
        html += "<div class='summary-card' onclick='filterStatus(2)' style='cursor:pointer'><div class='label'>Out of Stock</div><div class='value' style='color:#ef4444'>" + String(outCount) + "</div></div>";
        html += "</div>";
        
        // Enhanced Filtering System
        html += "<div style='background:rgba(255,255,255,0.1);padding:15px;border-radius:10px;margin:15px 0'>";
        html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:10px'>";
        html += "<div style='font-weight:bold;font-size:14px'>🔍 Advanced Filters</div>";
        html += "<button onclick='clearAllFilters()' style='background:#666;color:white;border:none;padding:4px 8px;border-radius:4px;cursor:pointer;font-size:11px'>Clear All</button>";
        html += "</div>";
        
        // Location Filters
        html += "<div style='margin-bottom:10px'>";
        html += "<div style='font-size:12px;opacity:0.8;margin-bottom:5px'>LOCATION:</div>";
        html += "<button onclick='applyLocationFilter(\"all\")' id='loc-all' class='filter-btn active' style='background:#667eea;color:white;border:none;padding:6px 10px;margin:2px;border-radius:5px;cursor:pointer;font-size:11px'>All Items</button>";
        html += "<button onclick='applyLocationFilter(\"trailer\")' id='loc-trailer' class='filter-btn' style='background:#4af;color:black;border:none;padding:6px 10px;margin:2px;border-radius:5px;cursor:pointer;font-size:11px'>🚚 Lives in Trailer</button>";
        html += "<button onclick='applyLocationFilter(\"trip\")' id='loc-trip' class='filter-btn' style='background:#ff6b35;color:white;border:none;padding:6px 10px;margin:2px;border-radius:5px;cursor:pointer;font-size:11px'>🛒 Buy Each Trip</button>";
        html += "</div>";
        
        // Status Filters
        html += "<div>";
        html += "<div style='font-size:12px;opacity:0.8;margin-bottom:5px'>STATUS:</div>";
        html += "<button onclick='applyStatusFilter(\"all\")' id='status-all' class='filter-btn active' style='background:#667eea;color:white;border:none;padding:6px 10px;margin:2px;border-radius:5px;cursor:pointer;font-size:11px'>All Status</button>";
        html += "<button onclick='applyStatusFilter(\"ok\")' id='status-ok' class='filter-btn' style='background:#22c55e;color:white;border:none;padding:6px 10px;margin:2px;border-radius:5px;cursor:pointer;font-size:11px'>✅ OK Stock</button>";
        html += "<button onclick='applyStatusFilter(\"low\")' id='status-low' class='filter-btn' style='background:#f59e0b;color:white;border:none;padding:6px 10px;margin:2px;border-radius:5px;cursor:pointer;font-size:11px'>⚠️ Low Stock</button>";
        html += "<button onclick='applyStatusFilter(\"out\")' id='status-out' class='filter-btn' style='background:#ef4444;color:white;border:none;padding:6px 10px;margin:2px;border-radius:5px;cursor:pointer;font-size:11px'>❌ Out of Stock</button>";
        html += "</div>";
        html += "</div>";
        
        for (size_t i = 0; i < inventory.size(); i++) {
            if (!inventory[i].isConsumable) continue;
            
            html += "<div class='inv-category' data-category-index='" + String(i) + "'>";
            html += "<div class='cat-header'>";
            html += "<div class='cat-title' onclick='toggleCat(" + String(i) + ")'>" + inventory[i].icon + " " + inventory[i].name + "<div class='cat-count'>" + String(inventory[i].consumables.size()) + "</div></div>";
            html += "<div class='cat-controls'>";
            html += "<span class='expand-icon' onclick='toggleCat(" + String(i) + ")'>\u{25BC}</span>";
            html += "<button onclick='showCategoryOptions(" + String(i) + ",\"" + inventory[i].name + "\")' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:6px;cursor:pointer;font-size:12px' title='Category Options'>⋮</button>";
            html += "</div></div>";
            
            html += "<div id='cat" + String(i) + "' class='items-container expanded'>";
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                String trailerIcon = inventory[i].consumables[j].livesInTrailer ? "🚚" : "🛒";
                html += "<div class='item consumable-item' data-status='" + String(inventory[i].consumables[j].status) + "' data-trailer='" + String(inventory[i].consumables[j].livesInTrailer ? "1" : "0") + "' data-item-index='" + String(j) + "'>";
                html += "<span class='item-name'>" + trailerIcon + " " + inventory[i].consumables[j].name + "</span>";
                html += "<div class='status-btns'>";
                html += String("<button class='status-btn ok") + (inventory[i].consumables[j].status <= STATUS_OK ? " active" : "") + "' onclick='setStatus(" + String(i) + "," + String(j) + "," + String(STATUS_OK) + ")'>OK</button>";
                html += String("<button class='status-btn low") + (inventory[i].consumables[j].status == STATUS_LOW ? " active" : "") + "' onclick='setStatus(" + String(i) + "," + String(j) + "," + String(STATUS_LOW) + ")'>Low</button>";
                html += String("<button class='status-btn out") + (inventory[i].consumables[j].status == STATUS_OUT ? " active" : "") + "' onclick='setStatus(" + String(i) + "," + String(j) + "," + String(STATUS_OUT) + ")'>Out</button>";
                html += "<button class='status-btn' style='background:#6b7280;margin-left:8px' onclick='editItem(" + String(i) + "," + String(j) + ",true,\"" + inventory[i].consumables[j].name + "\"," + String(inventory[i].consumables[j].livesInTrailer ? "true" : "false") + ")' title='Edit'>✏️</button>";
                html += "</div></div>";
            }
            html += "</div></div>";
        }

        html += "<div class='action-btns'>";
        html += "<button class='action-btn secondary' onclick='clearAllConsumables()'>\u{1F5D1} Clear All</button>";
        html += "<button class='action-btn secondary' onclick='showAddCategoryDialog(true)'>\u{1F4C1} Add Category</button>";
        html += "<button class='action-btn primary' onclick='showAddConsumableModal()' style='background:#22c55e'>\u{2795} Add Item</button>";
        html += "<button class='action-btn secondary' onclick='resetAll()'>🔄 Reset All to Full</button>";
        html += "<button class='action-btn secondary' onclick='window.location.href=\"/inventory\"'>\u{1F3E0} Main Dashboard</button>";
        html += "</div>";
    } 
    else if (tabNum == 2) {
        // Trailer Tab Content 
        html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:15px'>";
        html += "<h2 style='margin:0'>\u{1F69A} Trailer Equipment</h2>";
        html += "<div>";
        html += "<button onclick='collapseAll()' title='Collapse All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;margin-right:3px;cursor:pointer;font-size:12px'>▼</button>";
        html += "<button onclick='expandAll()' title='Expand All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;cursor:pointer;font-size:12px'>▶</button>";
        html += "</div></div>";
        
        // Summary tiles for Trailer equipment
        int totalItems = 0, checkedItems = 0, packedItems = 0;
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_TRAILER) continue;
            totalItems += inventory[i].equipment.size();
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                if (inventory[i].equipment[j].checked) checkedItems++;
                if (inventory[i].equipment[j].packed) packedItems++;
            }
        }
        
        html += "<div class='summary-grid' style='grid-template-columns:repeat(3,1fr)'>";
        html += "<div class='summary-card' style='cursor:pointer'><div class='label'>Total Items</div><div class='value' style='color:#3b82f6'>" + String(totalItems) + "</div></div>";
        html += "<div class='summary-card' onclick='filterEquipment(0,false)' style='cursor:pointer'><div class='label'>Need Check</div><div class='value' style='color:#f59e0b'>" + String(totalItems - checkedItems) + "</div></div>";
        html += "<div class='summary-card' onclick='filterEquipment(1,false)' style='cursor:pointer'><div class='label'>Need Pack</div><div class='value' style='color:#ef4444'>" + String(totalItems - packedItems) + "</div></div>";
        html += "</div>";
        
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_TRAILER) continue;
            
            html += "<div class='c'>";
            html += "<div class='cat-header'>";
            html += "<div class='cat-title' onclick='toggleCat(" + String(i) + ")'>" + inventory[i].icon + " " + inventory[i].name + "<div class='cat-count'>" + String(inventory[i].equipment.size()) + "</div></div>";
            html += "<div class='cat-controls'>";
            html += "<span class='expand-icon' onclick='toggleCat(" + String(i) + ")'>\u{25BC}</span>";
            html += "<button onclick='showCategoryOptions(" + String(i) + ",\"" + inventory[i].name + "\")' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:6px;cursor:pointer;font-size:12px' title='Category Options'>⋮</button>";
            html += "</div></div>";
            
            html += "<div id='cat" + String(i) + "' class='items-container expanded'>";
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                html += "<div class='item'><span class='item-name'>" + inventory[i].equipment[j].name + "</span>";
                html += "<div class='status-btns'>";
                html += String("<button class='status-btn checked") + (inventory[i].equipment[j].checked ? " active" : "") + "' onclick='toggleEquipmentStatus(" + String(i) + "," + String(j) + ",\"checked\",2)'>Checked</button>";
                html += String("<button class='status-btn packed") + (inventory[i].equipment[j].packed ? " active" : "") + "' onclick='toggleEquipmentStatus(" + String(i) + "," + String(j) + ",\"packed\",2)'>Packed</button>";
                html += "</div>";
                html += "<button class='status-btn' style='background:#6b7280;margin-left:8px' onclick='showItemOptions(" + String(i) + "," + String(j) + ",\"" + inventory[i].equipment[j].name + "\",false,false)' title='Options'>⋮</button>";
                html += "</div>";
            }
            html += "</div></div>";
        }

        html += "<div class='action-btns'>";
        html += "<button class='action-btn secondary' onclick='clearAllTrailer()'>\u{1F5D1} Clear All</button>";
        html += "<button class='action-btn secondary' onclick='showAddCategoryDialog(false)'>\u{1F4C1} Add Category</button>";
        html += "<button class='action-btn primary' onclick='showAddEquipmentModal()' style='background:#22c55e'>\u{2795} Add Item</button>";
        html += "<button class='action-btn secondary' onclick='window.location.href=\"/inventory\"'>\u{1F3E0} Main Dashboard</button>";
        html += "</div>";
    }
    else if (tabNum == 3) {
        // Essentials Tab Content
        html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:15px'>";
        html += "<h2 style='margin:0'>\u{2B50} Essential Equipment</h2>";
        html += "<div>";
        html += "<button onclick='collapseAll()' title='Collapse All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;margin-right:3px;cursor:pointer;font-size:12px'>▼</button>";
        html += "<button onclick='expandAll()' title='Expand All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;cursor:pointer;font-size:12px'>▶</button>";
        html += "</div></div>";
        
        // Summary tiles for Essentials equipment
        int totalItems = 0, checkedItems = 0, packedItems = 0;
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_ESSENTIALS) continue;
            totalItems += inventory[i].equipment.size();
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                if (inventory[i].equipment[j].checked) checkedItems++;
                if (inventory[i].equipment[j].packed) packedItems++;
            }
        }
        
        html += "<div class='summary-grid' style='grid-template-columns:repeat(3,1fr)'>";
        html += "<div class='summary-card' style='cursor:pointer'><div class='label'>Total Items</div><div class='value' style='color:#3b82f6'>" + String(totalItems) + "</div></div>";
        html += "<div class='summary-card' onclick='filterEquipment(0,false)' style='cursor:pointer'><div class='label'>Need Check</div><div class='value' style='color:#f59e0b'>" + String(totalItems - checkedItems) + "</div></div>";
        html += "<div class='summary-card' onclick='filterEquipment(1,false)' style='cursor:pointer'><div class='label'>Need Pack</div><div class='value' style='color:#ef4444'>" + String(totalItems - packedItems) + "</div></div>";
        html += "</div>";
        
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_ESSENTIALS) continue;
            
            html += "<div class='c'>";
            html += "<div class='cat-header'>";
            html += "<div class='cat-title' onclick='toggleCat(" + String(i) + ")'>" + inventory[i].icon + " " + inventory[i].name + "<div class='cat-count'>" + String(inventory[i].equipment.size()) + "</div></div>";
            html += "<div class='cat-controls'>";
            html += "<span class='expand-icon' onclick='toggleCat(" + String(i) + ")'>\u{25BC}</span>";
            html += "<button onclick='showCategoryOptions(" + String(i) + ",\"" + inventory[i].name + "\")' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:6px;cursor:pointer;font-size:12px' title='Category Options'>⋮</button>";
            html += "</div></div>";
            
            html += "<div id='cat" + String(i) + "' class='items-container expanded'>";
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                html += "<div class='item'><span class='item-name'>" + inventory[i].equipment[j].name + "</span>";
                html += "<div class='status-btns'>";
                html += String("<button class='status-btn checked") + (inventory[i].equipment[j].checked ? " active" : "") + "' onclick='toggleEquipmentStatus(" + String(i) + "," + String(j) + ",\"checked\",3)'>Checked</button>";
                html += String("<button class='status-btn packed") + (inventory[i].equipment[j].packed ? " active" : "") + "' onclick='toggleEquipmentStatus(" + String(i) + "," + String(j) + ",\"packed\",3)'>Packed</button>";
                html += "</div>";
                html += "<button class='status-btn' style='background:#6b7280;margin-left:8px' onclick='showItemOptions(" + String(i) + "," + String(j) + ",\"" + inventory[i].equipment[j].name + "\",false,false)' title='Options'>⋮</button>";
                html += "</div>";
            }
            html += "</div></div>";
        }

        html += "<div class='action-btns'>";
        html += "<button class='action-btn primary' onclick='clearAllEssentials()'>🔄 Clear All</button>";
        html += "<button class='action-btn secondary' onclick='showAddCategoryDialog(false)'>\u{1F4C1} Add Category</button>";
        html += "<button class='action-btn primary' onclick='showAddEquipmentModal()' style='background:#22c55e'>\u{2795} Add Item</button>";
        html += "<button class='action-btn secondary' onclick='window.location.href=\"/inventory\"'>\u{1F3E0} Main Dashboard</button>";
        html += "</div>";
    }
    else if (tabNum == 4) {
        // Optional Tab Content
        html += "<div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:15px'>";
        html += "<h2 style='margin:0'>\u{1F4E6} Optional Equipment</h2>";
        html += "<div>";
        html += "<button onclick='collapseAll()' title='Collapse All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;margin-right:3px;cursor:pointer;font-size:12px'>▼</button>";
        html += "<button onclick='expandAll()' title='Expand All' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:4px;cursor:pointer;font-size:12px'>▶</button>";
        html += "</div></div>";
        
        // Summary tiles for Optional equipment
        int totalItems = 0, checkedItems = 0, packedItems = 0, takingItems = 0;
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_OPTIONAL) continue;
            totalItems += inventory[i].equipment.size();
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                if (inventory[i].equipment[j].checked) checkedItems++;
                if (inventory[i].equipment[j].packed) packedItems++;
                if (inventory[i].equipment[j].taking) takingItems++;
            }
        }
        
        html += "<div class='summary-grid'>";
        html += "<div class='summary-card' style='cursor:pointer'><div class='label'>Total Items</div><div class='value' style='color:#3b82f6'>" + String(totalItems) + "</div></div>";
        html += "<div class='summary-card' onclick='filterEquipment(2,false)' style='cursor:pointer'><div class='label'>Taking</div><div class='value' style='color:#8b5cf6'>" + String(takingItems) + "</div></div>";
        html += "<div class='summary-card' onclick='filterEquipment(0,false)' style='cursor:pointer'><div class='label'>Need Check</div><div class='value' style='color:#f59e0b'>" + String(takingItems - checkedItems) + "</div></div>";
        html += "<div class='summary-card' onclick='filterEquipment(1,false)' style='cursor:pointer'><div class='label'>Need Pack</div><div class='value' style='color:#ef4444'>" + String(takingItems - packedItems) + "</div></div>";
        html += "</div>";
        
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_OPTIONAL) continue;
            
            html += "<div class='c'>";
            html += "<div class='cat-header'>";
            html += "<div class='cat-title' onclick='toggleCat(" + String(i) + ")'>" + inventory[i].icon + " " + inventory[i].name + "<div class='cat-count'>" + String(inventory[i].equipment.size()) + "</div></div>";
            html += "<div class='cat-controls'>";
            html += "<span class='expand-icon' onclick='toggleCat(" + String(i) + ")'>\u{25BC}</span>";
            html += "<button onclick='showCategoryOptions(" + String(i) + ",\"" + inventory[i].name + "\")' style='background:#6b7280;color:white;border:none;padding:6px 8px;border-radius:6px;cursor:pointer;font-size:12px' title='Category Options'>⋮</button>";
            html += "</div></div>";
            
            html += "<div id='cat" + String(i) + "' class='items-container expanded'>";
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                html += "<div class='item'><span class='item-name'>" + inventory[i].equipment[j].name + "</span>";
                html += "<div class='status-btns'>";
                html += String("<button class='status-btn taking") + (inventory[i].equipment[j].taking ? " active" : "") + "' onclick='toggleEquipmentStatus(" + String(i) + "," + String(j) + ",\"taking\",4)'>Taking</button>";
                String disabledClass = inventory[i].equipment[j].taking ? "" : " disabled";
                html += String("<button class='status-btn checked") + (inventory[i].equipment[j].checked && inventory[i].equipment[j].taking ? " active" : "") + disabledClass + "' onclick='toggleEquipmentStatus(" + String(i) + "," + String(j) + ",\"checked\",4)'>Checked</button>";
                html += String("<button class='status-btn packed") + (inventory[i].equipment[j].packed && inventory[i].equipment[j].taking ? " active" : "") + disabledClass + "' onclick='toggleEquipmentStatus(" + String(i) + "," + String(j) + ",\"packed\",4)'>Packed</button>";
                html += "</div>";
                html += "<button class='status-btn' style='background:#6b7280;margin-left:8px' onclick='showItemOptions(" + String(i) + "," + String(j) + ",\"" + inventory[i].equipment[j].name + "\",false,false)' title='Options'>⋮</button>";
                html += "</div>";
            }
            html += "</div></div>";
        }

        html += "<div class='action-btns'>";
        html += "<button class='action-btn primary' onclick='clearAllOptional()'>🔄 Clear All</button>";
        html += "<button class='action-btn secondary' onclick='showAddCategoryDialog(false)'>\u{1F4C1} Add Category</button>";
        html += "<button class='action-btn primary' onclick='showAddEquipmentModal()' style='background:#22c55e'>\u{2795} Add Item</button>";
        html += "<button class='action-btn secondary' onclick='window.location.href=\"/inventory\"'>\u{1F3E0} Main Dashboard</button>";
        html += "</div>";
    }
    else if (tabNum == 5) {
        // Shopping Tab Content - Restore full functionality
        String shoppingItems = "";
        int shoppingCount = 0;
        
        for (size_t i = 0; i < inventory.size(); i++) {
            if (!inventory[i].isConsumable) continue;
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                if (inventory[i].consumables[j].status == STATUS_LOW || inventory[i].consumables[j].status == STATUS_OUT) {
                    String statusClass = (inventory[i].consumables[j].status == STATUS_LOW) ? "low" : "out";
                    String badgeText = (inventory[i].consumables[j].status == STATUS_LOW) ? "LOW" : "OUT";
                    String trailerAttribute = inventory[i].consumables[j].livesInTrailer ? "true" : "false";
                    shoppingItems += "<div class='shop-item " + statusClass + "' data-cat='" + String(i) + "' data-item='" + String(j) + "' data-status='" + String(inventory[i].consumables[j].status) + "' data-trailer='" + trailerAttribute + "'>";
                    shoppingItems += "<input type='checkbox' class='shop-check' style='width:18px;height:18px;margin-right:10px;cursor:pointer'>";
                    String trailerIcon = inventory[i].consumables[j].livesInTrailer ? "\u{1F69A}" : "\u{1F6D2}";
                    shoppingItems += "<span style='margin-right:8px;font-size:14px'>" + trailerIcon + "</span>";
                    shoppingItems += "<div style='flex:1'>";
                    shoppingItems += "<div style='font-weight:bold;margin-bottom:2px'>" + inventory[i].consumables[j].name + "</div>";
                    shoppingItems += "<div style='font-size:13px;opacity:0.8'>" + inventory[i].name + "</div></div>";
                    shoppingItems += "<span class='badge " + statusClass + "'>" + badgeText + "</span></div>";
                    shoppingCount++;
                }
            }
        }

        // Count LOW and OUT separately  
        int lowShopCount = 0, outShopCount = 0;
        for (size_t i = 0; i < inventory.size(); i++) {
            if (!inventory[i].isConsumable) continue;
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                if (inventory[i].consumables[j].status == STATUS_LOW) lowShopCount++;
                else if (inventory[i].consumables[j].status == STATUS_OUT) outShopCount++;
            }
        }
        
        html += "<div class='c'>";
        
        // Add filters ABOVE the header when there are shopping items
        if (shoppingCount > 0) {
            html += "<div style='margin-bottom:12px;padding:10px;background:rgba(255,255,255,0.05);border-radius:8px'>";
            html += "<div style='font-size:12px;opacity:0.8;margin-bottom:8px;font-weight:bold'>🛒 SHOPPING FILTERS:</div>";
            html += "<div style='margin-bottom:10px'>";
            html += "<div style='font-size:11px;opacity:0.7;margin-bottom:6px'>📍 LOCATION:</div>";
            html += "<div style='display:flex;gap:8px;flex-wrap:wrap;margin-bottom:10px'>";
            html += "<button onclick='applyShoppingLocationFilter(\"all\")' id='shop-loc-all' class='filter-btn active' style='background:#667eea;color:white;border:none;padding:8px 12px;margin:2px;border-radius:6px;cursor:pointer;font-size:12px;font-weight:500'>All Items</button>";
            html += "<button onclick='applyShoppingLocationFilter(\"trailer\")' id='shop-loc-trailer' class='filter-btn' style='background:#4af;color:black;border:none;padding:8px 12px;margin:2px;border-radius:6px;cursor:pointer;font-size:12px;font-weight:500'>🚚 Lives in Trailer</button>";
            html += "<button onclick='applyShoppingLocationFilter(\"trip\")' id='shop-loc-trip' class='filter-btn' style='background:#ff6b35;color:white;border:none;padding:8px 12px;margin:2px;border-radius:6px;cursor:pointer;font-size:12px;font-weight:500'>🛒 Buy Each Trip</button>";
            html += "</div>";
            html += "<div style='font-size:11px;opacity:0.7;margin-bottom:6px'>🎯 STATUS:</div>";
            html += "<div style='display:flex;gap:8px;flex-wrap:wrap'>";
            html += "<button onclick='applyShoppingStatusFilter(\"all\")' id='shop-status-all' class='filter-btn active' style='background:#667eea;color:white;border:none;padding:8px 12px;margin:2px;border-radius:6px;cursor:pointer;font-size:12px;font-weight:500'>All Status</button>";
            html += "<button onclick='applyShoppingStatusFilter(\"low\")' id='shop-status-low' class='filter-btn' style='background:#f59e0b;color:white;border:none;padding:8px 12px;margin:2px;border-radius:6px;cursor:pointer;font-size:12px;font-weight:500'>⚠️ Low Stock</button>";
            html += "<button onclick='applyShoppingStatusFilter(\"out\")' id='shop-status-out' class='filter-btn' style='background:#ef4444;color:white;border:none;padding:8px 12px;margin:2px;border-radius:6px;cursor:pointer;font-size:12px;font-weight:500'>❌ Out of Stock</button>";
            html += "</div></div></div>";
        }
        
        html += "<div class='cat-header' onclick='toggleCat(99)'>";
        html += "<div class='cat-title'>\u{1F6D2} Items to Purchase";
        html += "<span class='cat-count' id='shop-count'>";
        if (outShopCount > 0) {
            html += "<span style='color:#ef4444'>" + String(outShopCount) + " OUT</span>";
            if (lowShopCount > 0) html += " + ";
        }
        if (lowShopCount > 0) {
            html += "<span style='color:#f59e0b'>" + String(lowShopCount) + " LOW</span>";
        }
        if (shoppingCount == 0) {
            html += "0 items";
        }
        html += "</span></div>";
        html += "<span class='expand-icon'>\u{25BC}</span></div>";

        if (shoppingCount > 0) {
            html += "<div id='cat99' class='items-container expanded'>";
            html += "<div style='margin-bottom:12px;padding:8px;background:rgba(255,255,255,0.03);border-radius:6px'>";
            html += "<div style='font-size:11px;opacity:0.7;margin-bottom:8px;font-weight:bold'>🎯 SELECTION ACTIONS:</div>";
            html += "<div style='display:flex;gap:8px;flex-wrap:wrap'>";
            html += "<button onclick=\"selectItems('all')\" style='padding:8px 14px;background:rgba(255,255,255,0.2);border:none;border-radius:6px;color:#fff;font-size:12px;font-weight:500;cursor:pointer'>Select All Visible</button>";
            html += "<button onclick=\"selectItems('out')\" style='padding:8px 14px;background:rgba(239,68,68,0.3);border:none;border-radius:6px;color:#fff;font-size:12px;font-weight:500;cursor:pointer'>Select Out Only</button>";
            html += "<button onclick=\"selectItems('low')\" style='padding:8px 14px;background:rgba(245,158,11,0.3);border:none;border-radius:6px;color:#fff;font-size:12px;font-weight:500;cursor:pointer'>Select Low Only</button>";
            html += "<button onclick=\"selectItems('none')\" style='padding:8px 14px;background:rgba(255,255,255,0.1);border:none;border-radius:6px;color:#fff;font-size:12px;font-weight:500;cursor:pointer'>Deselect All</button>";
            html += "</div></div>";
            html += shoppingItems + "</div>";
        } else {
            html += "<div id='cat99' class='items-container expanded'><div class='empty'><h3>\u{2713} All stocked up!</h3><p>No items need purchasing</p></div></div>";
        }

        html += "</div>";

        html += "<div class='action-btns'>";
        if (shoppingCount > 0) {
            html += "<button class='action-btn primary' onclick='markRestocked()'>✅ Mark Selected as Restocked</button>";
        }
        html += "<button class='action-btn secondary' onclick='copyFilteredList()'>📋 Copy Visible List</button>";
        html += "<button class='action-btn secondary' onclick='downloadList()'>⬇️ Download</button>";
        html += "<button class='action-btn secondary' onclick='window.location.href=\"/inventory\"'>\u{1F3E0} Main Dashboard</button>";
        html += "</div";
    }
    
    server.send(200, "text/html", html);
}

void handleInventory() {
    // Check if this is a request for a specific tab's content only
    if (server.hasArg("tab")) {
        handleTabContent();
        return;
    }
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no,viewport-fit=cover'>";
    html += "<meta name='apple-mobile-web-app-capable' content='yes'>";
    html += "<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent'>";
    html += "<meta name='mobile-web-app-capable' content='yes'>";
    html += "<title>Inventory</title>";
    html += "<style>";
    html += "*{margin:0;padding:0;box-sizing:border-box;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}";
    html += "body{background:#000;color:#fff;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;padding:8px;line-height:1.3;font-size:15px}";
    html += ".header{text-align:center;padding:10px;background:linear-gradient(135deg,#1a1a1a,#111);border-radius:8px;margin-bottom:12px}";
    html += ".header h1{font-size:1.3em;margin:0}";
    html += ".tabs{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:15px}";
    html += ".tab{padding:12px 8px;background:rgba(255,255,255,0.1);border:none;border-radius:8px;color:#fff;font-size:15px;cursor:pointer;text-align:center;transition:all 0.2s;font-weight:500;line-height:1.3;display:flex;flex-direction:column;align-items:center;gap:4px}";
    html += ".tab.active{background:linear-gradient(135deg,#667eea,#764ba2);box-shadow:0 4px 15px rgba(102,126,234,0.4)}";
    html += ".content{display:none}";
    html += ".content.active{display:block}";
    html += ".c{background:linear-gradient(135deg,#1a1a1a,#111);margin:8px 0;padding:12px;border-radius:12px;border-left:3px solid #333;box-shadow:0 4px 6px rgba(0,0,0,0.3)}";
    html += ".cat-header{display:flex;align-items:center;padding:12px;background:rgba(255,255,255,0.08);border-radius:10px;margin-bottom:10px;border-left:3px solid rgba(255,255,255,0.2)}";
    html += ".cat-title{font-size:1.1em;font-weight:bold;display:flex;align-items:center;gap:10px;cursor:pointer;flex:1}";
    html += ".cat-count{background:rgba(255,255,255,0.15);padding:4px 12px;border-radius:15px;font-size:0.85em;margin-left:10px;color:#fff}";
    html += ".cat-controls{display:flex;align-items:center;gap:10px;margin-left:auto}";
    html += ".expand-icon{font-size:14px;color:rgba(255,255,255,0.7);cursor:pointer}";
    html += ".item{display:flex;justify-content:space-between;align-items:center;padding:10px;background:rgba(255,255,255,0.05);border-radius:8px;margin-bottom:6px}";
    html += ".item-name{flex:1;font-size:14px}";
    html += ".status-btns{display:flex;gap:4px}";
    html += ".status-btn{padding:5px 12px;border:none;border-radius:6px;font-size:12px;font-weight:600;cursor:pointer;opacity:0.5}";
    html += ".status-btn.active{opacity:1;box-shadow:0 0 8px rgba(255,255,255,0.3)}";
    html += ".status-btn.ok{background:#22c55e;color:#fff}";
    html += ".status-btn.low{background:#f59e0b;color:#fff}";
    html += ".status-btn.out{background:#ef4444;color:#fff}";
    html += ".status-btn.checked{background:#3b82f6;color:#fff}";
    html += ".status-btn.packed{background:#22c55e;color:#fff}";
    html += ".status-btn.taking{background:#8b5cf6;color:#fff}";
    html += ".status-btn.disabled{background:#6b7280;opacity:0.4;cursor:not-allowed}";
    html += ".filter-btn{opacity:0.7;transition:all 0.2s}";
    html += ".filter-btn.active{opacity:1;transform:scale(0.95);box-shadow:inset 0 2px 4px rgba(0,0,0,0.3)}";
    html += ".checkbox-group{display:flex;gap:12px}";
    html += ".checkbox-label{display:flex;align-items:center;gap:6px;font-size:13px;padding:6px 10px;border-radius:6px;background:rgba(255,255,255,0.05);transition:all 0.2s;cursor:pointer;opacity:0.6}";
    html += ".checkbox-label:has(input:checked){opacity:1;background:rgba(255,255,255,0.15);box-shadow:0 0 8px rgba(255,255,255,0.2)}";
    html += ".checkbox-label input{width:16px;height:16px;cursor:pointer;accent-color:#667eea}";
    html += ".items-container{max-height:0;overflow:hidden;transition:max-height 0.3s}";
    html += ".items-container.expanded{max-height:3000px}";
    html += ".expand-icon{transition:transform 0.3s}";
    html += ".expanded .expand-icon{transform:rotate(180deg)}";
    html += ".action-btns{display:flex;gap:8px;margin-top:15px;flex-wrap:wrap}";
    html += ".action-btn{flex:1;padding:12px;border:none;border-radius:10px;font-size:14px;font-weight:600;cursor:pointer;min-width:120px}";
    html += ".action-btn.primary{background:linear-gradient(135deg,#667eea,#764ba2);color:#fff}";
    html += ".action-btn.secondary{background:rgba(255,255,255,0.2);color:#fff}";
    html += ".summary{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:15px}";
    html += ".summary-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin:15px 0}";
    html += ".summary-card{background:rgba(255,255,255,0.1);padding:15px;border-radius:10px;text-align:center}";
    html += ".summary-card .value{font-size:2em;font-weight:bold;margin:5px 0}";
    html += ".summary-card .label{font-size:0.85em;opacity:0.8}";
    
    // Main dashboard card styles
    html += ".main-cards{display:flex;flex-direction:column;gap:16px;margin-bottom:20px}";
    html += ".main-card{background:rgba(255,255,255,0.1);border-radius:12px;padding:16px;cursor:pointer;transition:all 0.2s;display:flex;align-items:center;gap:16px}";
    html += ".main-card:hover{background:rgba(255,255,255,0.15);transform:translateY(-2px);box-shadow:0 8px 25px rgba(0,0,0,0.3)}";
    html += ".main-card-left{flex-shrink:0;display:flex;flex-direction:column;align-items:center;gap:8px;min-width:80px}";
    html += ".main-card-icon{font-size:32px;line-height:1}";
    html += ".main-card-title{font-size:14px;font-weight:600;text-align:center;color:rgba(255,255,255,0.9)}";
    html += ".main-card-right{flex:1;display:grid;grid-template-columns:repeat(3,1fr);gap:12px}";
    html += ".main-card-tile{background:rgba(255,255,255,0.1);border-radius:8px;padding:12px;text-align:center;min-height:50px;display:flex;flex-direction:column;justify-content:center}";
    html += ".main-card-tile.ok .tile-value{color:#22c55e}";
    html += ".main-card-tile.low .tile-value{color:#f59e0b}";
    html += ".main-card-tile.out .tile-value{color:#ef4444}";
    html += ".main-card-tile.total .tile-value{color:#3b82f6}";
    html += ".main-card-tile.check .tile-value{color:#f59e0b}";
    html += ".main-card-tile.pack .tile-value{color:#ef4444}";
    html += ".main-card-tile.taking .tile-value{color:#8b5cf6}";
    html += ".tile-label{font-size:11px;color:rgba(255,255,255,0.7);margin-bottom:4px;font-weight:500}";
    html += ".tile-value{font-size:18px;font-weight:bold;line-height:1}";
    html += ".shop-item{display:flex;justify-content:space-between;align-items:center;padding:12px;background:rgba(255,255,255,0.05);border-radius:8px;margin-bottom:8px;border-left:4px solid}";
    html += ".shop-item.low{border-left-color:#f59e0b}";
    html += ".shop-item.out{border-left-color:#ef4444}";
    html += ".badge{padding:4px 10px;border-radius:10px;font-size:11px;font-weight:600}";
    html += ".badge.low{background:#f59e0b;color:#000}";
    html += ".badge.out{background:#ef4444;color:#fff}";
    html += ".empty{text-align:center;padding:30px;opacity:0.6}";

    // Tablet/iPad optimization (768px and wider)
    html += "@media(min-width:768px){";
    html += "body{padding:12px 15px;max-width:1000px;margin:0 auto;font-size:16px}";
    html += ".header{padding:15px}";
    html += ".header h1{font-size:1.6em}";
    html += ".tabs{grid-template-columns:repeat(6,1fr);gap:10px;margin-bottom:12px}";
    html += ".tab{padding:14px 12px;font-size:1em;gap:5px}";
    html += ".c{margin:10px 0;padding:12px}";
    html += ".cat-header{padding:14px}";
    html += ".cat-title{font-size:1.15em;gap:12px}";
    html += ".cat-count{padding:5px 14px;font-size:0.9em;margin-left:18px}";
    html += ".expand-icon{font-size:16px}";
    html += ".item{padding:10px;margin-bottom:6px}";
    html += ".item-name{font-size:1em}";
    html += ".status-btn{padding:6px 12px;font-size:0.85em}";
    html += ".checkbox-label{font-size:0.95em;padding:8px 12px}";
    html += ".checkbox-label input{width:18px;height:18px}";
    html += ".action-btn{padding:12px;font-size:1em;min-width:140px}";
    html += ".summary-card{padding:12px}";
    html += ".summary-card .value{font-size:2.2em}";
    html += ".summary-card .label{font-size:0.9em}";
    html += ".shop-item{padding:12px;margin-bottom:8px}";
    html += ".badge{padding:4px 10px;font-size:0.75em}";
    html += ".main-card-right{grid-template-columns:1fr;gap:8px}";
    html += ".main-card-tile{padding:8px}";
    html += ".tile-value{font-size:16px}";
    html += "}";

    html += "</style></head><body>";

    // Header
    html += "<div class='header'>";
    html += "<h1>\u{1F69A} Inventory Tracker</h1></div>";

    // Tabs - restored to top for all pages
    html += "<div class='tabs'>";
    html += "<button class='tab active' onclick='showTab(0)'><div>\u{1F3E0}</div><div>Main</div></button>";
    html += "<button class='tab' onclick='showTab(1)'><div>\u{1F374}</div><div>Consumables</div></button>";
    html += "<button class='tab' onclick='showTab(2)'><div>\u{1F69A}</div><div>Trailer</div></button>";
    html += "<button class='tab' onclick='showTab(3)'><div>\u{2B50}</div><div>Essentials</div></button>";
    html += "<button class='tab' onclick='showTab(4)'><div>\u{1F4E6}</div><div>Optional</div></button>";
    html += "<button class='tab' onclick='showTab(5)'><div>\u{1F6D2}</div><div>Shopping</div></button>";
    html += "</div>";

    // Main Dashboard Tab
    html += "<div class='content active' id='tab0'>";
    
    // Calculate summary data for each tab
    // Consumables Summary
    int okCount = 0, lowCount = 0, outCount = 0;
    int trailerOk = 0, trailerLow = 0, trailerOut = 0;
    int tripOk = 0, tripLow = 0, tripOut = 0;
    
    for (size_t i = 0; i < inventory.size(); i++) {
        if (!inventory[i].isConsumable) continue;
        for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
            switch (inventory[i].consumables[j].status) {
                case STATUS_OK: 
                    okCount++; 
                    if (inventory[i].consumables[j].livesInTrailer) trailerOk++; else tripOk++;
                    break;
                case STATUS_LOW: 
                    lowCount++; 
                    if (inventory[i].consumables[j].livesInTrailer) trailerLow++; else tripLow++;
                    break;
                case STATUS_OUT: 
                    outCount++; 
                    if (inventory[i].consumables[j].livesInTrailer) trailerOut++; else tripOut++;
                    break;
            }
        }
    }
    
    // Equipment Summary by Category
    int trailerTotal = 0, trailerChecked = 0, trailerPacked = 0;
    int essentialsTotal = 0, essentialsChecked = 0, essentialsPacked = 0;
    int optionalTotal = 0, optionalChecked = 0, optionalPacked = 0;
    int optionalTaking = 0, optionalTakingChecked = 0, optionalTakingPacked = 0;
    
    for (size_t i = 0; i < inventory.size(); i++) {
        if (inventory[i].isConsumable) continue;
        
        for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
            if (inventory[i].subcategory == SUBCATEGORY_TRAILER) {
                trailerTotal++;
                if (inventory[i].equipment[j].checked) trailerChecked++;
                if (inventory[i].equipment[j].packed) trailerPacked++;
            } else if (inventory[i].subcategory == SUBCATEGORY_ESSENTIALS) {
                essentialsTotal++;
                if (inventory[i].equipment[j].checked) essentialsChecked++;
                if (inventory[i].equipment[j].packed) essentialsPacked++;
            } else if (inventory[i].subcategory == SUBCATEGORY_OPTIONAL) {
                optionalTotal++;
                if (inventory[i].equipment[j].checked) optionalChecked++;
                if (inventory[i].equipment[j].packed) optionalPacked++;
                
                // Count taking items only for optional category
                if (inventory[i].equipment[j].taking) {
                    optionalTaking++;
                    if (inventory[i].equipment[j].checked) optionalTakingChecked++;
                    if (inventory[i].equipment[j].packed) optionalTakingPacked++;
                }
            }
        }
    }
    
    // Main Dashboard Cards
    html += "<div class='main-cards'>";
    
    // Consumables - Trailer Card
    html += "<div class='main-card' onclick='showTab(1)'>";
    html += "<div class='main-card-left'>";
    html += "<div class='main-card-icon'>\u{1F69A}</div>";
    html += "<div class='main-card-title'>Consumables</div>";
    html += "<div style='font-size:12px;opacity:0.7;margin-top:4px'>Trailer</div>";
    html += "</div>";
    html += "<div class='main-card-right'>";
    html += "<div class='main-card-tile ok'><div class='tile-label'>OK Stock</div><div class='tile-value'>" + String(trailerOk) + "</div></div>";
    html += "<div class='main-card-tile low'><div class='tile-label'>Low Stock</div><div class='tile-value'>" + String(trailerLow) + "</div></div>";
    html += "<div class='main-card-tile out'><div class='tile-label'>Out of Stock</div><div class='tile-value'>" + String(trailerOut) + "</div></div>";
    html += "</div></div>";
    
    // Consumables - Trip Card  
    html += "<div class='main-card' onclick='showTab(1)'>";
    html += "<div class='main-card-left'>";
    html += "<div class='main-card-icon'>\u{1F6D2}</div>";
    html += "<div class='main-card-title'>Consumables</div>";
    html += "<div style='font-size:12px;opacity:0.7;margin-top:4px'>Trip</div>";
    html += "</div>";
    html += "<div class='main-card-right'>";
    html += "<div class='main-card-tile ok'><div class='tile-label'>OK Stock</div><div class='tile-value'>" + String(tripOk) + "</div></div>";
    html += "<div class='main-card-tile low'><div class='tile-label'>Low Stock</div><div class='tile-value'>" + String(tripLow) + "</div></div>";
    html += "<div class='main-card-tile out'><div class='tile-label'>Out of Stock</div><div class='tile-value'>" + String(tripOut) + "</div></div>";
    html += "</div></div>";
    
    // Trailer Card
    html += "<div class='main-card' onclick='showTab(2)'>";
    html += "<div class='main-card-left'>";
    html += "<div class='main-card-icon'>\u{1F3D5}</div>";
    html += "<div class='main-card-title'>Trailer</div>";
    html += "</div>";
    html += "<div class='main-card-right'>";
    html += "<div class='main-card-tile total'><div class='tile-label'>Total Items</div><div class='tile-value'>" + String(trailerTotal) + "</div></div>";
    html += "<div class='main-card-tile check'><div class='tile-label'>Need Check</div><div class='tile-value'>" + String(trailerTotal - trailerChecked) + "</div></div>";
    html += "<div class='main-card-tile pack'><div class='tile-label'>Need Pack</div><div class='tile-value'>" + String(trailerTotal - trailerPacked) + "</div></div>";
    html += "</div></div>";
    
    // Essentials Card
    html += "<div class='main-card' onclick='showTab(3)'>";
    html += "<div class='main-card-left'>";
    html += "<div class='main-card-icon'>\u{2B50}</div>";
    html += "<div class='main-card-title'>Essentials</div>";
    html += "</div>";
    html += "<div class='main-card-right'>";
    html += "<div class='main-card-tile total'><div class='tile-label'>Total Items</div><div class='tile-value'>" + String(essentialsTotal) + "</div></div>";
    html += "<div class='main-card-tile check'><div class='tile-label'>Need Check</div><div class='tile-value'>" + String(essentialsTotal - essentialsChecked) + "</div></div>";
    html += "<div class='main-card-tile pack'><div class='tile-label'>Need Pack</div><div class='tile-value'>" + String(essentialsTotal - essentialsPacked) + "</div></div>";
    html += "</div></div>";
    
    // Optional Card (with Taking workflow)
    html += "<div class='main-card' onclick='showTab(4)'>";
    html += "<div class='main-card-left'>";
    html += "<div class='main-card-icon'>\u{1F4E6}</div>";
    html += "<div class='main-card-title'>Optional</div>";
    html += "</div>";
    html += "<div class='main-card-right'>";
    html += "<div class='main-card-tile taking'><div class='tile-label'>Taking</div><div class='tile-value'>" + String(optionalTaking) + "</div></div>";
    html += "<div class='main-card-tile check'><div class='tile-label'>Need Check</div><div class='tile-value'>" + String(optionalTaking - optionalTakingChecked) + "</div></div>";
    html += "<div class='main-card-tile pack'><div class='tile-label'>Need Pack</div><div class='tile-value'>" + String(optionalTaking - optionalTakingPacked) + "</div></div>";
    html += "</div></div>";
    
    html += "</div>"; // Close main-cards
    
    // Navigation Actions (moved below cards for better main dashboard focus)
    html += "<div class='action-btns'>";
    html += "<button class='action-btn primary' onclick='window.location.href=\"/\"'>\u{1F3E0} Home (Monitoring)</button>";
    html += "<button class='action-btn secondary' onclick='window.location.href=\"/inventory/backups\"'>\u{1F4C4} Backups</button>";
    html += "<button class='action-btn secondary' onclick='showTab(5)'>\u{1F6D2} Shopping List</button>";
    html += "</div>";
    
    html += "</div>";  // Close Main tab
    
    // Single content container for dynamic loading  
    html += "<div id='content' style='display:none'></div>";

    // JavaScript
    html += "<script>";
    html += "let editingCategory=-1;";
    html += "let editingIndex=-1;";
    html += "function showTab(n){";
    html += "document.querySelectorAll('.tab').forEach((t,i)=>t.classList.toggle('active',i==n));";
    html += "let mainTab=document.getElementById('tab0');";
    html += "let dynamicContent=document.getElementById('content');";
    html += "if(n==0){";
    html += "mainTab.style.display='block';";
    html += "dynamicContent.style.display='none';";
    html += "}else{";
    html += "mainTab.style.display='none';";
    html += "dynamicContent.style.display='block';";
    html += "fetch('/inventory?tab='+n).then(r=>r.text()).then(html=>{";
    html += "dynamicContent.innerHTML=html;";
    html += "if(n==5)refreshShoppingList();});";
    html += "}}";
    
    html += "function toggleCat(id){";
    html += "let el=document.getElementById('cat'+id);";
    html += "let hdr=el.previousElementSibling;";
    html += "let icon=hdr.querySelector('.expand-icon');";
    html += "if(el.classList.contains('expanded')){";
    html += "el.classList.remove('expanded');el.classList.add('collapsed');";
    html += "icon.innerHTML='\\u25B6';}";
    html += "else{el.classList.remove('collapsed');el.classList.add('expanded');";
    html += "icon.innerHTML='\\u25BC';}}";

    html += "function collapseAll(){";
    html += "document.querySelectorAll('.items-container').forEach(el=>{";
    html += "el.classList.remove('expanded');el.classList.add('collapsed');";
    html += "let hdr=el.previousElementSibling;";
    html += "let icon=hdr.querySelector('.expand-icon');";
    html += "if(icon)icon.innerHTML='\\u25B6';});}";

    html += "function expandAll(){";
    html += "document.querySelectorAll('.items-container').forEach(el=>{";
    html += "el.classList.remove('collapsed');el.classList.add('expanded');";
    html += "let hdr=el.previousElementSibling;";
    html += "let icon=hdr.querySelector('.expand-icon');";
    html += "if(icon)icon.innerHTML='\\u25BC';});}";

    html += "function setStatus(cat,item,status){";
    html += "fetch('/inventory/set?cat='+cat+'&item='+item+'&status='+status).then(r=>r.ok?updateUI(cat,item,status):alert('Failed'));}";

    html += "function updateUI(cat,item,status){";
    html += "let container=document.getElementById('cat'+cat);";
    html += "let items=container.querySelectorAll('.item');";
    html += "items[item].setAttribute('data-status',status);";
    html += "let btns=items[item].querySelectorAll('.status-btn');";
    html += "btns.forEach((b,i)=>b.classList.toggle('active',i==status));";
    html += "updateConsumableTiles();}";

    html += "function updateConsumableTiles(){";
    html += "updateSummary();updateFilteredTileCounts();applyAllFilters();}";

    html += "function updateSummary(){";
    html += "fetch('/inventory/stats').then(r=>r.json()).then(d=>{";
    html += "document.querySelectorAll('.summary-card .value')[0].textContent=d.full;";
    html += "document.querySelectorAll('.summary-card .value')[1].textContent=d.low;";
    html += "document.querySelectorAll('.summary-card .value')[2].textContent=d.out;";
    html += "});}";

    html += "function setCheck(cat,item,type,val){";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type='+type+'&val='+(val?1:0));}";

    html += "function toggleEquipmentStatus(cat,item,statusType,tab){";
    html += "let container=document.getElementById('cat'+cat);";
    html += "if(!container)return;";
    html += "let items=container.querySelectorAll('.item');";
    html += "if(!items[item])return;";
    html += "let btn=items[item].querySelector('.status-btn.'+statusType);";
    html += "if(!btn||btn.classList.contains('disabled'))return;";
    html += "let isActive=btn.classList.contains('active');";
    html += "let newVal=!isActive;";
    html += "btn.classList.toggle('active',newVal);";
    html += "if(statusType=='taking'){";
    html += "let checkedBtn=items[item].querySelector('.status-btn.checked');";
    html += "let packedBtn=items[item].querySelector('.status-btn.packed');";
    html += "if(newVal){";
    html += "checkedBtn.classList.remove('disabled');";
    html += "packedBtn.classList.remove('disabled');";
    html += "}else{";
    html += "checkedBtn.classList.add('disabled');";
    html += "checkedBtn.classList.remove('active');";
    html += "packedBtn.classList.add('disabled');";
    html += "packedBtn.classList.remove('active');";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type=0&val=0');";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type=1&val=0');";
    html += "}";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type=2&val='+(newVal?1:0));";
    html += "}else{";
    html += "let typeNum=(statusType=='checked')?0:1;";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type='+typeNum+'&val='+(newVal?1:0));";
    html += "}";
    html += "updateEquipmentTiles(tab);";
    html += "}";

    html += "function updateEquipmentTiles(tabNum){";
    html += "setTimeout(()=>showTab(tabNum),100);";
    html += "}";

    html += "function toggleTaking(cat,item,val){";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type=2&val='+(val?1:0)).then(()=>{";
    html += "if(!val){";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type=0&val=0');";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type=1&val=0');}";
    html += "setTimeout(()=>showTab(4),200);});}";

    html += "function clearAllEssentials(){";
    html += "if(!confirm('Clear all Taking, Checked, and Packed for Essentials?'))return;";
    html += "fetch('/inventory/clearall?type=essentials').then(r=>{";
    html += "if(r.ok){setTimeout(()=>showTab(3),100);}else{alert('Clear failed');}});}";

    html += "function clearAllOptional(){";
    html += "if(!confirm('Clear all Taking, Checked, and Packed for Optional items?'))return;";
    html += "fetch('/inventory/clearall?type=optional').then(r=>{";
    html += "if(r.ok){setTimeout(()=>showTab(4),100);}else{alert('Clear failed');}});}";

    html += "function clearAllTrailer(){";
    html += "if(!confirm('Clear all Checked and Packed for Trailer items?'))return;";
    html += "fetch('/inventory/clearall?type=trailer').then(r=>{";
    html += "if(r.ok){setTimeout(()=>showTab(2),100);}else{alert('Clear failed');}});}";

    html += "function clearAllConsumables(){";
    html += "let filteredCount=document.querySelectorAll('.inv-category .item:not([style*=\"display: none\"])').length;";
    html += "let totalCount=document.querySelectorAll('.inv-category .item').length;";
    html += "let filterText=filteredCount===totalCount?'all consumables':'visible filtered items ('+filteredCount+' of '+totalCount+')';";
    html += "if(!confirm('Set '+filterText+' to OUT status for stocktaking?'))return;";
    html += "let visibleItems=[];";
    html += "document.querySelectorAll('.inv-category .item:not([style*=\"display: none\"])').forEach(item=>{";
    html += "let catIndex=item.closest('.inv-category').getAttribute('data-category-index');";
    html += "let itemIndex=item.getAttribute('data-item-index');";
    html += "if(catIndex!==null&&itemIndex!==null)visibleItems.push({cat:catIndex,item:itemIndex});});";
    html += "if(visibleItems.length===0){alert('No visible items to clear');return;}";
    html += "let payload=JSON.stringify({items:visibleItems});";
    html += "console.log('Sending clear payload:',payload);";
    html += "fetch('/inventory/clearall?type=consumables',{method:'POST',headers:{'Content-Type':'application/json'},body:payload}).then(r=>{";
    html += "console.log('Clear response:',r.status,r.ok);";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);setTimeout(()=>showTab(tabIndex),100);}else{alert('Clear failed');}});}";

    html += "function saveInventory(){";
    html += "fetch('/inventory/save').then(r=>{if(r.ok){";
    html += "window.scrollTo({top:0,behavior:'smooth'});";
    html += "updateSummary();";
    html += "alert('\u{2713} Saved!');}else{alert('Save failed');}});}";

    html += "function setCheckWithUpdate(cat,item,type,val,tabNum){";
    html += "fetch('/inventory/check?cat='+cat+'&item='+item+'&type='+type+'&val='+(val?1:0)).then(()=>{";
    html += "setTimeout(()=>{updateSummary();updateFilteredTileCounts();applyAllFilters();},100);});}";

    html += "function showItemOptions(cat,item,name,isConsumable,currentTrailer){";
    html += "let action=prompt('Choose action for \"'+name+'\":\\n\\n1 - Edit Item\\n2 - Delete\\n3 - Move to different category\\n\\nEnter 1, 2, or 3:');";
    html += "if(action==='1'){";
    html += "editItem(cat,item,isConsumable,name,currentTrailer);";
    html += "}";
    html += "else if(action==='2'){";
    html += "if(confirm('Delete \"'+name+'\"?')){";
    html += "fetch('/inventory/delete?cat='+cat+'&item='+item).then(r=>{";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);setTimeout(()=>showTab(tabIndex),100);}else{alert('Delete failed');}});}}";
    html += "else if(action==='3'){";
    html += "let categories=[];";
    html += "document.querySelectorAll('.cat-title').forEach((el,idx)=>{";
    html += "categories.push(idx+' - '+el.textContent.trim());});";
    html += "let catList=categories.join('\\n');";
    html += "let newCat=prompt('Select target category:\\n\\n'+catList+'\\n\\nEnter category number:');";
    html += "if(newCat&&newCat!==String(cat)){";
    html += "fetch('/inventory/move?cat='+cat+'&item='+item+'&target='+newCat).then(r=>{";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);setTimeout(()=>showTab(tabIndex),100);}else{alert('Move failed');}});}}";
    html += "}";

    html += "function showCategoryOptions(cat,name){";
    html += "let action=prompt('Choose action for category \"'+name+'\":\\n\\n1 - Rename Category\\n2 - Delete Category\\n\\nEnter 1 or 2:');";
    html += "if(action==='1'){";
    html += "let newName=prompt('Enter new category name:',name);";
    html += "if(newName&&newName!==name){";
    html += "fetch('/inventory/renamecat?cat='+cat+'&name='+encodeURIComponent(newName)).then(r=>{";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);setTimeout(()=>showTab(tabIndex),100);}else{alert('Rename failed');}});}}";
    html += "else if(action==='2'){";
    html += "if(confirm('Delete category \"'+name+'\" and ALL its items?\\n\\nThis cannot be undone!')){";
    html += "fetch('/inventory/deletecat?cat='+cat).then(r=>{";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);setTimeout(()=>showTab(tabIndex),100);}else{alert('Delete failed');}});}}";
    html += "}";

    html += "function showAddItemDialog(subcategory){";
    html += "let name=prompt('Enter new item name:');";
    html += "if(!name)return;";
    html += "let categoryNum=prompt('Enter category number (0-' + (document.querySelectorAll('.c').length-1) + '):');";
    html += "if(categoryNum===null)return;";
    html += "let livesInTrailer='';";
    html += "if(subcategory==0){";
    html += "livesInTrailer=confirm('Does this item live in the trailer?\\n\\nYES = Stays in trailer (🚚)\\nNO = Buy each trip (🛒)')?'true':'false';";
    html += "}";
    html += "fetch('/inventory/additem?cat=' + categoryNum + '&name=' + encodeURIComponent(name) + '&subcategory=' + subcategory + '&livesInTrailer=' + livesInTrailer)";
    html += ".then(r=>{if(r.ok){loadTabContent(currentTab);}else{alert('Add failed');}});}";

    html += "let activeFilter=-1;";
    html += "let activeEquipmentFilter=-1;";
    html += "let activeTrailerFilter=-1;";
    html += "function filterStatus(status){";
    html += "if(activeFilter==status){activeFilter=-1;status=-1;}else{activeFilter=status;}";
    html += "document.querySelectorAll('.inv-category .item').forEach(item=>{";
    html += "let itemStatus=parseInt(item.getAttribute('data-status'));";
    html += "item.style.display=(status==-1||itemStatus==status)?'flex':'none';});";
    html += "updateFilteredTileCounts();}";

    html += "function filterTrailer(trailerStatus){";
    html += "if(activeTrailerFilter==trailerStatus){activeTrailerFilter=-1;trailerStatus=-1;}else{activeTrailerFilter=trailerStatus;}";
    html += "document.querySelectorAll('.inv-category .item').forEach(item=>{";
    html += "let itemTrailer=parseInt(item.getAttribute('data-trailer'));";
    html += "let shouldShow=true;";
    html += "if(trailerStatus==-1)shouldShow=true;";
    html += "else if(trailerStatus==1)shouldShow=(itemTrailer==1);";
    html += "else if(trailerStatus==0)shouldShow=(itemTrailer==0);";
    html += "item.style.display=shouldShow?'flex':'none';});";
    html += "updateFilteredTileCounts();}";

    html += "function updateFilteredTileCounts(){";
    html += "let okCount=0,lowCount=0,outCount=0;";
    html += "document.querySelectorAll('.inv-category .item').forEach(item=>{";
    html += "if(item.style.display!=='none'){";
    html += "let status=parseInt(item.getAttribute('data-status'));";
    html += "if(status<=0)okCount++;";
    html += "else if(status==1)lowCount++;";
    html += "else if(status==2)outCount++;}});";
    html += "let cards=document.querySelectorAll('.summary-card .value');";
    html += "if(cards[0])cards[0].textContent=okCount;";
    html += "if(cards[1])cards[1].textContent=lowCount;";
    html += "if(cards[2])cards[2].textContent=outCount;}";

    html += "let currentLocationFilter='all';";
    html += "let currentStatusFilter='all';";

    html += "function applyLocationFilter(filter){";
    html += "currentLocationFilter=filter;";
    html += "document.querySelectorAll('#loc-all,#loc-trailer,#loc-trip').forEach(btn=>btn.classList.remove('active'));";
    html += "document.getElementById('loc-'+filter).classList.add('active');";
    html += "applyAllFilters();}";

    html += "function applyStatusFilter(filter){";
    html += "currentStatusFilter=filter;";
    html += "document.querySelectorAll('#status-all,#status-ok,#status-low,#status-out').forEach(btn=>btn.classList.remove('active'));";
    html += "document.getElementById('status-'+filter).classList.add('active');";
    html += "applyAllFilters();}";

    html += "function applyAllFilters(){";
    html += "document.querySelectorAll('.inv-category .item').forEach(item=>{";
    html += "let shouldShow=true;";
    html += "if(currentLocationFilter!=='all'){";
    html += "let itemTrailer=parseInt(item.getAttribute('data-trailer'));";
    html += "if(currentLocationFilter==='trailer'&&itemTrailer!==1)shouldShow=false;";
    html += "if(currentLocationFilter==='trip'&&itemTrailer!==0)shouldShow=false;}";
    html += "if(currentStatusFilter!=='all'&&shouldShow){";
    html += "let itemStatus=parseInt(item.getAttribute('data-status'));";
    html += "if(currentStatusFilter==='ok'&&itemStatus>0)shouldShow=false;";
    html += "if(currentStatusFilter==='low'&&itemStatus!==1)shouldShow=false;";
    html += "if(currentStatusFilter==='out'&&itemStatus!==2)shouldShow=false;}";
    html += "item.style.display=shouldShow?'flex':'none';});";
    html += "updateFilteredTileCounts();}";

    html += "function clearAllFilters(){";
    html += "currentLocationFilter='all';";
    html += "currentStatusFilter='all';";
    html += "document.querySelectorAll('.filter-btn').forEach(btn=>btn.classList.remove('active'));";
    html += "document.getElementById('loc-all').classList.add('active');";
    html += "document.getElementById('status-all').classList.add('active');";
    html += "applyAllFilters();}";

    html += "function filterEquipment(type,showCompleted){";
    html += "if(activeEquipmentFilter==type){activeEquipmentFilter=-1;type=-1;}else{activeEquipmentFilter=type;}";
    html += "document.querySelectorAll('.items-container').forEach(container=>{";
    html += "container.querySelectorAll('.item').forEach(item=>{";
    html += "let shouldShow=true;";
    html += "if(type==-1){shouldShow=true;}";
    html += "else if(type==0){";
    html += "let checkedBtn=item.querySelector('.status-btn.checked');";
    html += "let takingBtn=item.querySelector('.status-btn.taking');";
    html += "if(takingBtn){shouldShow=takingBtn.classList.contains('active')&&!checkedBtn.classList.contains('active');}";
    html += "else{shouldShow=!checkedBtn.classList.contains('active');}}";
    html += "else if(type==1){";
    html += "let packedBtn=item.querySelector('.status-btn.packed');";
    html += "let takingBtn=item.querySelector('.status-btn.taking');";
    html += "if(takingBtn){shouldShow=takingBtn.classList.contains('active')&&!packedBtn.classList.contains('active');}";
    html += "else{shouldShow=!packedBtn.classList.contains('active');}}";
    html += "else if(type==2){";
    html += "let takingBtn=item.querySelector('.status-btn.taking');";
    html += "shouldShow=takingBtn?takingBtn.classList.contains('active'):false;}";
    html += "item.style.display=shouldShow?'flex':'none';});});}";

    html += "function resetAll(){";
    html += "let filteredCount=document.querySelectorAll('.inv-category .item:not([style*=\"display: none\"])').length;";
    html += "let totalCount=document.querySelectorAll('.inv-category .item').length;";
    html += "let filterText=filteredCount===totalCount?'ALL consumable items':'visible filtered items ('+filteredCount+' of '+totalCount+')';";
    html += "if(!confirm('Reset '+filterText+' to Full? This cannot be undone.'))return;";
    html += "let visibleItems=[];";
    html += "document.querySelectorAll('.inv-category .item:not([style*=\"display: none\"])').forEach(item=>{";
    html += "let catIndex=item.closest('.inv-category').getAttribute('data-category-index');";
    html += "let itemIndex=item.getAttribute('data-item-index');";
    html += "if(catIndex!==null&&itemIndex!==null)visibleItems.push({cat:catIndex,item:itemIndex});});";
    html += "if(visibleItems.length===0){alert('No visible items to reset');return;}";
    html += "let payload=JSON.stringify({items:visibleItems});";
    html += "console.log('Sending reset payload:',payload);";
    html += "fetch('/inventory/resetall',{method:'POST',headers:{'Content-Type':'application/json'},body:payload}).then(r=>{";
    html += "console.log('Reset response:',r.status,r.ok);";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);setTimeout(()=>showTab(tabIndex),100);}else{alert('Reset failed');}});}";

    html += "let shoppingLocationFilter='all';";
    html += "let shoppingStatusFilter='all';";
    
    html += "function applyShoppingLocationFilter(filter){";
    html += "shoppingLocationFilter=filter;";
    html += "document.querySelectorAll('[id^=\"shop-loc-\"]').forEach(btn=>btn.classList.remove('active'));";
    html += "document.getElementById('shop-loc-'+filter).classList.add('active');";
    html += "applyAllShoppingFilters();}";
    
    html += "function applyShoppingStatusFilter(filter){";
    html += "shoppingStatusFilter=filter;";
    html += "document.querySelectorAll('[id^=\"shop-status-\"]').forEach(btn=>btn.classList.remove('active'));";
    html += "document.getElementById('shop-status-'+filter).classList.add('active');";
    html += "applyAllShoppingFilters();}";
    
    html += "function applyAllShoppingFilters(){";
    html += "document.querySelectorAll('.shop-item').forEach(item=>{";
    html += "let trailerAttr=item.getAttribute('data-trailer');";
    html += "let statusAttr=item.getAttribute('data-status');";
    html += "console.log('Item status attr:',statusAttr,'trailer:',trailerAttr);";
    html += "let locationMatch=shoppingLocationFilter=='all'||(shoppingLocationFilter=='trailer'&&trailerAttr=='true')||(shoppingLocationFilter=='trip'&&trailerAttr=='false');";
    html += "let statusMatch=shoppingStatusFilter=='all'||(shoppingStatusFilter=='low'&&statusAttr=='1')||(shoppingStatusFilter=='out'&&statusAttr=='2');";
    html += "item.style.display=(locationMatch&&statusMatch)?'flex':'none';});";
    html += "updateShoppingTileCounts();}";
    
    html += "function updateShoppingTileCounts(){";
    html += "let visibleLow=0,visibleOut=0;";
    html += "document.querySelectorAll('.shop-item').forEach(item=>{";
    html += "if(item.style.display=='none')return;";
    html += "let status=parseInt(item.getAttribute('data-status'));";
    html += "if(status==1)visibleLow++;";
    html += "else if(status==2)visibleOut++;});";
    html += "let countEl=document.getElementById('shop-count');";
    html += "if(countEl){";
    html += "let html='';";
    html += "if(visibleOut>0){html+='<span style=\"color:#ef4444\">'+visibleOut+' OUT</span>';}";
    html += "if(visibleLow>0){if(html)html+=' + ';html+='<span style=\"color:#f59e0b\">'+visibleLow+' LOW</span>';}";
    html += "if(visibleOut==0&&visibleLow==0)html='0 items';";
    html += "countEl.innerHTML=html;}}";

    html += "function filterShoppingTrailer(mode){";
    html += "document.querySelectorAll('.shop-item').forEach(item=>{";
    html += "let trailerAttr=item.getAttribute('data-trailer');";
    html += "let shouldShow=mode=='all'||(mode=='trailer'&&trailerAttr=='true')||(mode=='trip'&&trailerAttr=='false');";
    html += "item.style.display=shouldShow?'flex':'none';});";
    html += "document.querySelectorAll('[id^=\"shopping-filter-\"]').forEach(btn=>btn.style.background='rgba(255,255,255,0.1)');";
    html += "document.getElementById('shopping-filter-'+mode).style.background='rgba(255,255,255,0.3)';";
    html += "updateShoppingTileCounts();}";

    html += "function selectItems(mode){";
    html += "let checkboxes=document.querySelectorAll('.shop-check');";
    html += "let visibleCount=0,selectedCount=0;";
    html += "checkboxes.forEach(cb=>{";
    html += "let item=cb.closest('.shop-item');";
    html += "if(!item)return;";
    html += "if(item.style.display=='none'){cb.checked=false;return;}";
    html += "visibleCount++;";
    html += "let status=parseInt(item.getAttribute('data-status'));";
    html += "let trailerAttr=item.getAttribute('data-trailer')||'false';";
    html += "let isTrailer=(trailerAttr=='true');";
    html += "console.log('SelectItems mode:'+mode+', status:'+status+', checking: low='+(status==1)+', out='+(status==2));";
    html += "let shouldSelect=false;";
    html += "if(mode=='all'){shouldSelect=true;}";
    html += "else if(mode=='none'){shouldSelect=false;}";
    html += "else if(mode=='out'){shouldSelect=(status==2);}";
    html += "else if(mode=='low'){shouldSelect=(status==1);}";
    html += "else if(mode=='trailer'){shouldSelect=isTrailer;}";
    html += "else if(mode=='trip'){shouldSelect=!isTrailer;}";
    html += "cb.checked=shouldSelect;";
    html += "if(shouldSelect)selectedCount++;});}";

    html += "function markRestocked(){";
    html += "let selected=[];";
    html += "document.querySelectorAll('.shop-check:checked').forEach(cb=>{";
    html += "let item=cb.closest('.shop-item');";
    html += "selected.push({cat:item.getAttribute('data-cat'),item:item.getAttribute('data-item')});});";
    html += "if(selected.length==0){alert('Please select items to restock');return;}";
    html += "if(!confirm('Mark '+selected.length+' selected item(s) as Full?'))return;";
    html += "fetch('/inventory/restock',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(selected)})";
    html += ".then(r=>{if(r.ok){let msg=document.createElement('div');msg.textContent='✅ '+selected.length+' item(s) restocked!';msg.style.cssText='position:fixed;top:20px;right:20px;background:#22c55e;color:white;padding:10px 15px;border-radius:5px;z-index:9999;font-weight:bold';document.body.appendChild(msg);setTimeout(()=>msg.remove(),3000);refreshShoppingList();}else{alert('Restock failed');}});}";

    html += "function copyList(){";
    html += "let items=[];";
    html += "document.querySelectorAll('.shop-item').forEach(el=>{";
    html += "let nameEl=el.querySelector('div>div');";
    html += "if(nameEl)items.push('\u{2022} '+nameEl.textContent);});";
    html += "if(items.length==0){alert('No items to copy');return;}";
    html += "let txt=items.join('\\n');";
    html += "if(navigator.clipboard&&navigator.clipboard.writeText){";
    html += "navigator.clipboard.writeText(txt).then(()=>{alert('\u{2713} Copied to clipboard!');},()=>{";
    html += "prompt('Copy this list:',txt);});}else{prompt('Copy this list:',txt);}}";

    html += "function downloadList(){";
    html += "window.location.href='/inventory/download';}";

    html += "function copyFilteredList(){";
    html += "let items=[];";
    html += "document.querySelectorAll('.shop-item').forEach(el=>{";
    html += "if(el.style.display=='none')return;";
    html += "let nameEl=el.querySelector('div>div');";
    html += "if(nameEl)items.push('\u{2022} '+nameEl.textContent);});";
    html += "if(items.length==0){alert('No visible items to copy');return;}";
    html += "let txt=items.join('\\n');";
    html += "if(navigator.clipboard&&navigator.clipboard.writeText){";
    html += "navigator.clipboard.writeText(txt).then(()=>{alert('\u{2713} Copied '+items.length+' filtered items to clipboard!');},()=>{";
    html += "prompt('Copy this filtered list:',txt);});}else{prompt('Copy this filtered list:',txt);}}";

    html += "function markAllFilteredRestocked(){";
    html += "let visibleItems=[];";
    html += "document.querySelectorAll('.shop-item').forEach(item=>{";
    html += "if(item.style.display=='none')return;";
    html += "visibleItems.push({cat:item.getAttribute('data-cat'),item:item.getAttribute('data-item')});});";
    html += "if(visibleItems.length==0){alert('No visible items to restock');return;}";
    html += "let filterText=shoppingLocationFilter=='all'?'':' ('+shoppingLocationFilter+' items)';";
    html += "filterText+=shoppingStatusFilter=='all'?'':' ('+shoppingStatusFilter+' status)';";
    html += "if(!confirm('Mark ALL '+visibleItems.length+' visible item(s)'+filterText+' as Full?'))return;";
    html += "fetch('/inventory/restock',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(visibleItems)})";
    html += ".then(r=>{if(r.ok){let msg=document.createElement('div');msg.textContent='✅ '+visibleItems.length+' filtered item(s) restocked!';msg.style.cssText='position:fixed;top:20px;right:20px;background:#22c55e;color:white;padding:10px 15px;border-radius:5px;z-index:9999;font-weight:bold';document.body.appendChild(msg);setTimeout(()=>msg.remove(),3000);refreshShoppingList();}else{alert('Bulk restock failed');}});}";

    html += "function addItem(cat){";
    html += "let name=prompt('Enter item name:');";
    html += "if(!name||name.trim()=='')return;";
    html += "fetch('/inventory/add?cat='+cat+'&name='+encodeURIComponent(name)).then(r=>{";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}else{alert('Failed to add item');}});}";

    html += "function editItem(cat,item,isConsumable,currentName,currentTrailer){";
    html += "if(isConsumable){";
    html += "editingCategory=cat;";
    html += "editingIndex=item;";
    html += "let modal=document.createElement('div');";
    html += "modal.id='editModal';";
    html += "modal.style='position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.8);display:flex;align-items:center;justify-content:center;z-index:1000';";
    html += "let modalContent=document.createElement('div');";
    html += "modalContent.style='background:#222;padding:20px;border-radius:10px;width:90%;max-width:400px;color:#fff';";
    html += "modalContent.innerHTML='<h3 style=\"margin:0 0 15px 0\">Edit Item</h3>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:15px\"><label style=\"display:block;margin-bottom:5px\">Item Name:</label>';";
    html += "modalContent.innerHTML+='<input type=\"text\" id=\"edit-name\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\"></div>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:20px\"><label style=\"display:block;margin-bottom:5px\">Location:</label>';";
    html += "modalContent.innerHTML+='<select id=\"edit-trailer\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\"></select></div>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:20px\"><label style=\"display:block;margin-bottom:5px\">Category:</label>';";
    html += "modalContent.innerHTML+='<select id=\"edit-category\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\"></select></div>';";
    html += "modalContent.innerHTML+='<div style=\"display:flex;gap:10px;justify-content:flex-end\">';";
    html += "modalContent.innerHTML+='<button onclick=\"closeEditModal()\" style=\"padding:8px 16px;background:#666;border:none;border-radius:4px;color:#fff;cursor:pointer\">Cancel</button>';";
    html += "modalContent.innerHTML+='<button onclick=\"deleteItemFromModal()\" style=\"padding:8px 16px;background:#ef4444;border:none;border-radius:4px;color:#fff;cursor:pointer\">Delete Item</button>';";
    html += "modalContent.innerHTML+='<button onclick=\"saveItemEdit()\" style=\"padding:8px 16px;background:#4af;border:none;border-radius:4px;color:#000;cursor:pointer;font-weight:bold\">Save Changes</button>';";
    html += "modalContent.innerHTML+='</div>';";
    html += "modal.appendChild(modalContent);";
    html += "modal.onclick=(e)=>{if(e.target===modal)closeEditModal();};";
    html += "document.body.appendChild(modal);";
    html += "let trailerSelect=document.getElementById('edit-trailer');";
    html += "trailerSelect.innerHTML='<option value=\"true\">🚚 Lives in Trailer</option><option value=\"false\">🛒 Buy Each Trip</option>';";
    html += "trailerSelect.value=currentTrailer?'true':'false';";
    html += "let categorySelect=document.getElementById('edit-category');";
    html += "categorySelect.innerHTML='';";
    html += "document.querySelectorAll('.cat-title').forEach((catEl,idx)=>{";
    html += "let catText=catEl.textContent.trim();";
    html += "let countEl=catEl.querySelector('.cat-count');";
    html += "if(countEl){catText=catText.replace(countEl.textContent,'').trim();}";
    html += "let option=document.createElement('option');";
    html += "option.value=idx;";
    html += "option.textContent=catText;";
    html += "if(idx=='+cat+')option.selected=true;";
    html += "categorySelect.appendChild(option);});";
    html += "document.getElementById('edit-name').value=currentName;";
    html += "document.getElementById('edit-name').focus();";
    html += "}else{";
    html += "let newName=prompt('Enter new name:',currentName);";
    html += "if(!newName||newName.trim()=='')return;";
    html += "fetch('/inventory/edit?cat='+cat+'&item='+item+'&name='+encodeURIComponent(newName)).then(r=>{";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}else{alert('Edit failed');}});}}";

    html += "function closeEditModal(){";
    html += "let modal=document.getElementById('editModal');";
    html += "if(modal){modal.remove();return;}";
    html += "modal=document.querySelector('[style*=\"position:fixed\"]');";
    html += "if(modal)modal.remove();}";

    html += "function showAddConsumableModal(){";
    html += "let modal=document.createElement('div');";
    html += "modal.id='addModal';";
    html += "modal.style='position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.8);display:flex;align-items:center;justify-content:center;z-index:1000';";
    html += "let modalContent=document.createElement('div');";
    html += "modalContent.style='background:#222;padding:20px;border-radius:10px;width:90%;max-width:400px;color:#fff';";
    html += "modalContent.innerHTML='<h3 style=\"margin:0 0 15px 0\">Add Consumable Item</h3>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:15px\"><label style=\"display:block;margin-bottom:5px\">Item Name:</label>';";
    html += "modalContent.innerHTML+='<input type=\"text\" id=\"add-name\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\" placeholder=\"Enter item name\"></div>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:15px\"><label style=\"display:block;margin-bottom:5px\">Location:</label>';";
    html += "modalContent.innerHTML+='<select id=\"add-trailer\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\"><option value=\"false\">🛒 Buy Each Trip</option><option value=\"true\">🚚 Lives in Trailer</option></select></div>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:20px\"><label style=\"display:block;margin-bottom:5px\">Category:</label>';";
    html += "modalContent.innerHTML+='<select id=\"add-category\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\"></select></div>';";
    html += "modalContent.innerHTML+='<div style=\"display:flex;gap:10px;justify-content:flex-end\">';";
    html += "modalContent.innerHTML+='<button onclick=\"closeAddModal()\" style=\"padding:8px 16px;background:#666;border:none;border-radius:4px;color:#fff;cursor:pointer\">Cancel</button>';";
    html += "modalContent.innerHTML+='<button onclick=\"saveNewConsumable()\" style=\"padding:8px 16px;background:#22c55e;border:none;border-radius:4px;color:#000;cursor:pointer;font-weight:bold\">Add Item</button>';";
    html += "modalContent.innerHTML+='</div>';";
    html += "modal.appendChild(modalContent);";
    html += "modal.onclick=(e)=>{if(e.target===modal)closeAddModal();};";
    html += "document.body.appendChild(modal);";
    html += "document.querySelectorAll('.cat-title').forEach((catEl,idx)=>{";
    html += "let catText=catEl.textContent.trim();";
    html += "let countEl=catEl.querySelector('.cat-count');";
    html += "if(countEl){catText=catText.replace(countEl.textContent,'').trim();}";
    html += "let option=document.createElement('option');";
    html += "option.value=idx;";
    html += "option.textContent=catText;";
    html += "document.getElementById('add-category').appendChild(option);});";
    html += "document.getElementById('add-name').focus();}";

    html += "function showAddEquipmentModal(){";
    html += "let modal=document.createElement('div');";
    html += "modal.id='addModal';";
    html += "modal.style='position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.8);display:flex;align-items:center;justify-content:center;z-index:1000';";
    html += "let modalContent=document.createElement('div');";
    html += "modalContent.style='background:#222;padding:20px;border-radius:10px;width:90%;max-width:400px;color:#fff';";
    html += "modalContent.innerHTML='<h3 style=\"margin:0 0 15px 0\">Add Equipment Item</h3>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:15px\"><label style=\"display:block;margin-bottom:5px\">Item Name:</label>';";
    html += "modalContent.innerHTML+='<input type=\"text\" id=\"add-name\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\" placeholder=\"Enter item name\"></div>';";
    html += "modalContent.innerHTML+='<div style=\"margin-bottom:20px\"><label style=\"display:block;margin-bottom:5px\">Category:</label>';";
    html += "modalContent.innerHTML+='<select id=\"add-category\" style=\"width:100%;padding:8px;border:1px solid #444;background:#333;color:#fff;border-radius:4px\"></select></div>';";
    html += "modalContent.innerHTML+='<div style=\"display:flex;gap:10px;justify-content:flex-end\">';";
    html += "modalContent.innerHTML+='<button onclick=\"closeAddModal()\" style=\"padding:8px 16px;background:#666;border:none;border-radius:4px;color:#fff;cursor:pointer\">Cancel</button>';";
    html += "modalContent.innerHTML+='<button onclick=\"saveNewEquipment()\" style=\"padding:8px 16px;background:#22c55e;border:none;border-radius:4px;color:#000;cursor:pointer;font-weight:bold\">Add Item</button>';";
    html += "modalContent.innerHTML+='</div>';";
    html += "modal.appendChild(modalContent);";
    html += "modal.onclick=(e)=>{if(e.target===modal)closeAddModal();};";
    html += "document.body.appendChild(modal);";
    html += "document.querySelectorAll('.cat-title').forEach((catEl,idx)=>{";
    html += "let catText=catEl.textContent.trim();";
    html += "let countEl=catEl.querySelector('.cat-count');";
    html += "if(countEl){catText=catText.replace(countEl.textContent,'').trim();}";
    html += "let option=document.createElement('option');";
    html += "option.value=idx;";
    html += "option.textContent=catText;";
    html += "document.getElementById('add-category').appendChild(option);});";
    html += "document.getElementById('add-name').focus();}";

    html += "function closeAddModal(){";
    html += "let modal=document.getElementById('addModal');";
    html += "if(modal){modal.remove();return;}";
    html += "modal=document.querySelector('[style*=\"position:fixed\"]');";
    html += "if(modal)modal.remove();}";

    html += "function saveNewConsumable(){";
    html += "let name=document.getElementById('add-name').value.trim();";
    html += "let category=parseInt(document.getElementById('add-category').value);";
    html += "let trailer=document.getElementById('add-trailer').value;";
    html += "if(!name){alert('Name cannot be empty');return;}";
    html += "let params='cat='+category+'&name='+encodeURIComponent(name)+'&livesInTrailer='+trailer;";
    html += "fetch('/inventory/add?'+params).then(r=>{";
    html += "if(r.ok){closeAddModal();";
    html += "let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}else{alert('Failed to add item');}});}";
    
    html += "function saveNewEquipment(){";
    html += "let name=document.getElementById('add-name').value.trim();";
    html += "let category=parseInt(document.getElementById('add-category').value);";
    html += "if(!name){alert('Name cannot be empty');return;}";
    html += "let params='cat='+category+'&name='+encodeURIComponent(name);";
    html += "fetch('/inventory/add?'+params).then(r=>{";
    html += "if(r.ok){closeAddModal();";
    html += "let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}else{alert('Failed to add item');}});}";;
    
    html += "function deleteItemFromModal(){";
    html += "if(confirm('Delete this item? This cannot be undone.')){";
    html += "fetch('/inventory/remove?cat='+editingCategory+'&item='+editingIndex).then(r=>{";
    html += "if(r.ok){closeEditModal();";
    html += "let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}else{alert('Delete failed');}});}}";
    
    html += "function saveItemEdit(){";
    html += "let name=document.getElementById('edit-name').value.trim();";
    html += "let trailer=document.getElementById('edit-trailer').value==='true';";
    html += "let newCategory=parseInt(document.getElementById('edit-category').value);";
    html += "if(!name){alert('Name cannot be empty');return;}";
    html += "if(newCategory!==editingCategory){";
    html += "let editParams='cat='+editingCategory+'&item='+editingIndex+'&name='+encodeURIComponent(name)+'&livesInTrailer='+(trailer?'true':'false');";
    html += "fetch('/inventory/edit-consumable?'+editParams).then(r=>{";
    html += "if(r.ok){";
    html += "let moveParams='cat='+editingCategory+'&item='+editingIndex+'&target='+newCategory;";
    html += "return fetch('/inventory/move-item',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:moveParams});";
    html += "}else{throw new Error('Edit failed');}";
    html += "}).then(r=>{";
    html += "if(r.ok){closeEditModal();";
    html += "let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}";
    html += "else{alert('Error moving item to new category');}";
    html += "}).catch(e=>{alert('Error: '+e.message);});";
    html += "}else{";
    html += "let params='cat='+editingCategory+'&item='+editingIndex+'&name='+encodeURIComponent(name)+'&livesInTrailer='+(trailer?'true':'false');";
    html += "fetch('/inventory/edit-consumable?'+params).then(r=>{";
    html += "if(r.ok){closeEditModal();";
    html += "let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}else{alert('Edit failed');}});";
    html += "}";
    html += "}";

    html += "function removeItem(cat,item){";
    html += "if(!confirm('Delete this item? This cannot be undone.'))return;";
    html += "fetch('/inventory/remove?cat='+cat+'&item='+item).then(r=>{";
    html += "if(r.ok){let currentTab=document.querySelector('.tab.active');";
    html += "let tabIndex=Array.from(currentTab.parentNode.children).indexOf(currentTab);";
    html += "setTimeout(()=>showTab(tabIndex),100);}else{alert('Failed to remove item');}});}";

    html += "function refreshShoppingList(){";
    html += "fetch('/inventory/shopping').then(r=>r.json()).then(data=>{";
    html += "let container=document.getElementById('cat99');";
    html += "let header=document.getElementById('shop-count');";
    html += "if(data.count==0){";
    html += "header.innerHTML='0 items';";
    html += "container.innerHTML='<div class=\"empty\"><h3>\u{2713} All stocked up!</h3><p>No items need purchasing</p></div>';";
    html += "container.classList.add('expanded');";
    html += "}else{";
    html += "let countHTML='';";
    html += "if(data.outCount>0){countHTML+='<span style=\"color:#ef4444\">'+data.outCount+' OUT</span>';";
    html += "if(data.lowCount>0)countHTML+=' + ';}";
    html += "if(data.lowCount>0)countHTML+='<span style=\"color:#f59e0b\">'+data.lowCount+' LOW</span>';";
    html += "header.innerHTML=countHTML;";
    html += "let html='<div style=\"margin-bottom:10px;display:flex;gap:6px;flex-wrap:wrap\">';";
    html += "html+='<button onclick=\"selectItems(\\'all\\')\" style=\"padding:6px 12px;background:rgba(255,255,255,0.2);border:none;border-radius:6px;color:#fff;font-size:12px;cursor:pointer\">Select All</button>';";
    html += "html+='<button onclick=\"selectItems(\\'out\\')\" style=\"padding:6px 12px;background:rgba(239,68,68,0.3);border:none;border-radius:6px;color:#fff;font-size:12px;cursor:pointer\">Select Out Only</button>';";
    html += "html+='<button onclick=\"selectItems(\\'low\\')\" style=\"padding:6px 12px;background:rgba(245,158,11,0.3);border:none;border-radius:6px;color:#fff;font-size:12px;cursor:pointer\">Select Low Only</button>';";
    html += "html+='<button onclick=\"selectItems(\\'none\\')\" style=\"padding:6px 12px;background:rgba(255,255,255,0.1);border:none;border-radius:6px;color:#fff;font-size:12px;cursor:pointer\">Deselect All</button></div>';";
    html += "data.items.forEach(item=>{";
    html += "let statusClass=item.status==2?'out':'low';";
    html += "let badge=item.status==2?'OUT':'LOW';";
    html += "html+='<div class=\"shop-item '+statusClass+'\" data-cat=\"'+item.cat+'\" data-item=\"'+item.item+'\" data-status=\"'+item.status+'\" data-trailer=\"'+item.livesInTrailer+'\">';";
    html += "html+='<input type=\"checkbox\" class=\"shop-check\" style=\"width:18px;height:18px;margin-right:10px;cursor:pointer\">';";
    html += "html+='<span style=\"margin-right:8px;font-size:14px\">'+(item.livesInTrailer?'🚚':'🛒')+'</span>';";
    html += "html+='<div style=\"flex:1\"><div style=\"font-weight:bold;margin-bottom:2px\">'+item.name+'</div>';";
    html += "html+='<div style=\"font-size:13px;opacity:0.8\">'+item.category+'</div></div>';";
    html += "html+='<span class=\"badge '+statusClass+'\">'+badge+'</span></div>';});";
    html += "container.innerHTML=html;container.classList.add('expanded');";
    html += "setTimeout(updateShoppingTileCounts,100);}});}";

    // Add Category Dialog Functions
    html += "function showAddCategoryDialog(isConsumable){";
    html += "let type=isConsumable?'consumable':'equipment';";
    html += "let typeLabel=isConsumable?'Consumable':'Equipment';";
    html += "let name=prompt('Enter '+typeLabel+' category name:');";
    html += "if(!name||name.trim()=='')return;";
    html += "name=name.trim();";
    html += "let icon=prompt('Enter emoji icon (e.g., 🍝 or 🏕️):','📦');";
    html += "if(!icon||icon.trim()=='')icon='📦';";
    html += "icon=icon.trim();";
    html += "if(name.length>50){alert('Category name too long (max 50 chars)');return;}";
    html += "if(icon.length>10){alert('Icon too long (max 10 chars)');return;}";
    html += "let url='/inventory/add-category?name='+encodeURIComponent(name)+'&type='+type+'&icon='+encodeURIComponent(icon);";
    html += "if(!isConsumable){";
    html += "let subcategory=prompt('Choose subcategory:\\n\\n1 = TRAILER (always packed)\\n2 = ESSENTIALS (must pack every trip)\\n3 = OPTIONAL (extra items)\\n\\nEnter 1, 2, or 3:');";
    html += "if(subcategory=='1')subcategory='trailer';";
    html += "else if(subcategory=='2')subcategory='essentials';";
    html += "else if(subcategory=='3')subcategory='optional';";
    html += "else{alert('Invalid selection. Please choose 1, 2, or 3.');return;}";
    html += "url+='&subcategory='+subcategory;}";
    html += "fetch(url).then(r=>{if(r.ok){alert('✅ Category \"'+name+'\" created!');loadTabContent(currentTab);}";
    html += "else{r.text().then(msg=>alert('❌ Error: '+msg));}});}";
    html += "</script></body></html>";

    server.send(200, "text/html", html);
}

void handleInventorySet() {
    Serial.printf("[INVENTORY] handleInventorySet called\n");
    if (!server.hasArg("cat") || !server.hasArg("item") || !server.hasArg("status")) {
        Serial.printf("[INVENTORY] Missing parameters\n");
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    int item = server.arg("item").toInt();
    int status = server.arg("status").toInt();
    
    Serial.printf("[INVENTORY] Setting status: cat=%d, item=%d, status=%d\n", cat, item, status);

    if (cat >= 0 && cat < (int)inventory.size() && item >= 0 && status >= 0 && status <= 3) {
        if (inventory[cat].isConsumable && item < (int)inventory[cat].consumables.size()) {
            String itemName = inventory[cat].consumables[item].name;
            String catName = inventory[cat].name;
            inventory[cat].consumables[item].status = (ItemStatus)status;
            Serial.printf("[INVENTORY] Changed '%s' in '%s' to status %d\n", itemName.c_str(), catName.c_str(), status);
            saveInventoryToSPIFFS();  // Auto-save on every change
            server.send(200, "text/plain", "OK");
            return;
        } else {
            Serial.printf("[INVENTORY] Invalid category or item index\n");
        }
    } else {
        Serial.printf("[INVENTORY] Parameter validation failed\n");
    }

    server.send(400, "text/plain", "Invalid parameters");
}

void handleInventoryCheck() {
    if (!server.hasArg("cat") || !server.hasArg("item") || !server.hasArg("type") || !server.hasArg("val")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    int item = server.arg("item").toInt();
    int type = server.arg("type").toInt();
    int val = server.arg("val").toInt();

    if (cat >= 0 && cat < (int)inventory.size() && item >= 0 && type >= 0 && type <= 2) {
        if (!inventory[cat].isConsumable && item < (int)inventory[cat].equipment.size()) {
            if (type == 0) {
                inventory[cat].equipment[item].checked = (val == 1);
            } else if (type == 1) {
                inventory[cat].equipment[item].packed = (val == 1);
            } else if (type == 2) {
                inventory[cat].equipment[item].taking = (val == 1);
            }
            saveInventoryToSPIFFS();  // Auto-save on every change
            server.send(200, "text/plain", "OK");
            return;
        }
    }

    server.send(400, "text/plain", "Invalid parameters");
}

// Helper function to save inventory to SPIFFS using ArduinoJson
bool saveInventoryToSPIFFS() {
    File file = SPIFFS.open("/inventory.json", "w");
    if (!file) {
        Serial.println("[INVENTORY] Failed to open file for writing");
        return false;
    }

    JsonDocument doc;
    JsonArray categories = doc.to<JsonArray>();

    for (size_t i = 0; i < inventory.size(); i++) {
        JsonObject category = categories.add<JsonObject>();
        category["name"] = inventory[i].name;
        category["icon"] = inventory[i].icon;
        category["isConsumable"] = inventory[i].isConsumable;
        category["subcategory"] = inventory[i].subcategory;
        
        JsonArray items = category["items"].to<JsonArray>();
        
        if (inventory[i].isConsumable) {
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                JsonObject item = items.add<JsonObject>();
                item["name"] = inventory[i].consumables[j].name;
                item["status"] = inventory[i].consumables[j].status;
                item["livesInTrailer"] = inventory[i].consumables[j].livesInTrailer;
                item["lastUpdated"] = inventory[i].consumables[j].lastUpdated;
            }
        } else {
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                JsonObject item = items.add<JsonObject>();
                item["name"] = inventory[i].equipment[j].name;
                item["checked"] = inventory[i].equipment[j].checked;
                item["packed"] = inventory[i].equipment[j].packed;
                item["taking"] = inventory[i].equipment[j].taking;
                item["livesInTrailer"] = inventory[i].equipment[j].livesInTrailer;
                item["lastUpdated"] = inventory[i].equipment[j].lastUpdated;
            }
        }
    }

    serializeJson(doc, file);
    file.close();
    
    Serial.printf("[INVENTORY] Auto-saved %d categories to SPIFFS\n", inventory.size());
    Serial.printf("[DEBUG] JSON size: %u bytes\n", measureJson(doc));
    return true;
}

void handleInventorySave() {
    if (saveInventoryToSPIFFS()) {
        server.send(200, "text/plain", "OK");
    } else {
        server.send(500, "text/plain", "Failed to save");
    }
}

void handleInventoryResetAll() {
    int reset = 0;
    
    // Check if request has JSON body (filtered items)
    if (server.method() == HTTP_POST && server.hasArg("plain")) {
        String body = server.arg("plain");
        Serial.printf("[INVENTORY] Reset filtered items body: %s\n", body.c_str());
        
        // Simple JSON parsing for {items:[{cat:0,item:1},{cat:0,item:2}]}
        int itemsStart = body.indexOf("\"items\":[");
        if (itemsStart >= 0) {
            itemsStart += 9; // Skip "items":["
            int itemsEnd = body.indexOf(']', itemsStart);
            if (itemsEnd > itemsStart) {
                String itemsStr = body.substring(itemsStart, itemsEnd);
                Serial.printf("[INVENTORY] Items string: %s\n", itemsStr.c_str());
                
                // Parse each {cat:X,item:Y} object
                int pos = 0;
                while (pos < itemsStr.length()) {
                    int objStart = itemsStr.indexOf('{', pos);
                    if (objStart < 0) break;
                    int objEnd = itemsStr.indexOf('}', objStart);
                    if (objEnd < 0) break;
                    
                    String obj = itemsStr.substring(objStart, objEnd + 1);
                    Serial.printf("[INVENTORY] Processing object: %s\n", obj.c_str());
                    
                    // Extract cat and item numbers
                    int catPos = obj.indexOf("\"cat\":");
                    int itemPos = obj.indexOf("\"item\":");
                    if (catPos >= 0 && itemPos >= 0) {
                        int catStart = catPos + 7; // Skip "cat":" and opening quote
                        int catEnd = obj.indexOf('"', catStart); // Find closing quote
                        
                        int itemStart = itemPos + 8; // Skip "item":" and opening quote
                        int itemEnd = obj.indexOf('"', itemStart); // Find closing quote
                        
                        int catNum = obj.substring(catStart, catEnd).toInt();
                        int itemNum = obj.substring(itemStart, itemEnd).toInt();
                        
                        Serial.printf("[INVENTORY] Parsed cat:%d, item:%d\n", catNum, itemNum);
                        
                        if (catNum < inventory.size() && inventory[catNum].isConsumable && 
                            itemNum < inventory[catNum].consumables.size()) {
                            inventory[catNum].consumables[itemNum].status = STATUS_FULL;
                            reset++;
                            Serial.printf("[INVENTORY] Set item to FULL: cat:%d, item:%d\n", catNum, itemNum);
                        }
                    }
                    
                    pos = objEnd + 1;
                }
            }
        }
        Serial.printf("[INVENTORY] Reset %d filtered items to Full\n", reset);
    } else {
        // Fallback: reset all consumables (old behavior)
        for (size_t i = 0; i < inventory.size(); i++) {
            if (!inventory[i].isConsumable) continue;
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                inventory[i].consumables[j].status = STATUS_FULL;
                reset++;
            }
        }
        Serial.printf("[INVENTORY] Reset all %d consumables to Full\n", reset);
    }
    
    saveInventoryToSPIFFS();  // Auto-save after reset
    server.send(200, "text/plain", "OK");
}

void handleInventoryRestock() {
    // Check if request has JSON body (selective restock)
    if (server.method() == HTTP_POST && server.hasArg("plain")) {
        String body = server.arg("plain");
        Serial.printf("[INVENTORY] Restock body: %s\n", body.c_str());

        // Parse JSON array of selected items
        int startIdx = 0;
        int count = 0;
        while (startIdx < body.length()) {
            // Look for cat and item values (they might not have quotes around numbers)
            int catStart = body.indexOf("\"cat\":\"", startIdx);
            int itemStart = body.indexOf("\"item\":\"", startIdx);

            // Also try without quotes (JavaScript might send numbers without quotes)
            if (catStart == -1) {
                catStart = body.indexOf("\"cat\":", startIdx);
                if (catStart != -1) catStart += 6; // Skip past "cat":
            } else {
                catStart += 7; // Skip past "cat":"
            }

            if (itemStart == -1) {
                itemStart = body.indexOf("\"item\":", startIdx);
                if (itemStart != -1) itemStart += 7; // Skip past "item":
            } else {
                itemStart += 8; // Skip past "item":"
            }

            if (catStart == -1 || itemStart == -1) break;

            // Find the end of the value (could be comma, } or end of string)
            int catEnd = body.length();
            for (int i = catStart; i < body.length(); i++) {
                if (body[i] == ',' || body[i] == '}' || body[i] == '"') {
                    catEnd = i;
                    break;
                }
            }

            int itemEnd = body.length();
            for (int i = itemStart; i < body.length(); i++) {
                if (body[i] == ',' || body[i] == '}' || body[i] == '"') {
                    itemEnd = i;
                    break;
                }
            }

            String catStr = body.substring(catStart, catEnd);
            String itemStr = body.substring(itemStart, itemEnd);
            catStr.trim();
            itemStr.trim();

            int cat = catStr.toInt();
            int item = itemStr.toInt();

            Serial.printf("[INVENTORY] Parsing: cat=%d, item=%d\n", cat, item);

            if (cat >= 0 && cat < (int)inventory.size()) {
                if (inventory[cat].isConsumable && item >= 0 && item < (int)inventory[cat].consumables.size()) {
                    Serial.printf("[INVENTORY] Restocking: %s (cat %d, item %d)\n",
                                  inventory[cat].consumables[item].name.c_str(), cat, item);
                    inventory[cat].consumables[item].status = STATUS_FULL;
                    count++;
                }
            }

            startIdx = itemEnd + 1;
        }

        Serial.printf("[INVENTORY] Restocked %d selected items\n", count);
        saveInventoryToSPIFFS();  // Auto-save after restock
        server.send(200, "text/plain", "OK");
    } else {
        Serial.println("[INVENTORY] No JSON body, this shouldn't happen");
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleInventoryShopping() {
    // Check for filter parameter: "trailer", "trip", or "all"
    String filter = server.hasArg("filter") ? server.arg("filter") : "all";
    
    String json = "{\"count\":0,\"lowCount\":0,\"outCount\":0,\"items\":[";

    int totalCount = 0;
    int lowCount = 0;
    int outCount = 0;
    bool first = true;

    for (size_t i = 0; i < inventory.size(); i++) {
        if (!inventory[i].isConsumable) continue;
        for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
            if (inventory[i].consumables[j].status == STATUS_LOW || inventory[i].consumables[j].status == STATUS_OUT) {
                // Apply trailer filter
                bool includeItem = true;
                if (filter == "trailer" && !inventory[i].consumables[j].livesInTrailer) includeItem = false;
                else if (filter == "trip" && inventory[i].consumables[j].livesInTrailer) includeItem = false;
                
                if (includeItem) {
                    if (!first) json += ",";
                    first = false;

                    json += "{\"cat\":" + String(i);
                    json += ",\"item\":" + String(j);
                    json += ",\"name\":\"" + inventory[i].consumables[j].name + "\"";
                    json += ",\"category\":\"" + inventory[i].name + "\"";
                    json += ",\"status\":" + String(inventory[i].consumables[j].status);
                    json += ",\"livesInTrailer\":" + String(inventory[i].consumables[j].livesInTrailer ? "true" : "false") + "}";

                    totalCount++;
                    if (inventory[i].consumables[j].status == STATUS_LOW) lowCount++;
                    else outCount++;
                }
            }
        }
    }

    json += "],\"count\":" + String(totalCount);
    json += ",\"lowCount\":" + String(lowCount);
    json += ",\"outCount\":" + String(outCount);
    json += ",\"filter\":\"" + filter + "\"}";

    server.send(200, "application/json", json);
}

void handleInventoryStats() {
    if (server.hasArg("tab")) {
        int tabNum = server.arg("tab").toInt();
        int total = 0, checked = 0, packed = 0, taking = 0;
        
        if (tabNum == 2) { // Trailer
            for (size_t i = 0; i < inventory.size(); i++) {
                if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_TRAILER) continue;
                total += inventory[i].equipment.size();
                for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                    if (inventory[i].equipment[j].checked) checked++;
                    if (inventory[i].equipment[j].packed) packed++;
                }
            }
        } else if (tabNum == 3) { // Essentials
            for (size_t i = 0; i < inventory.size(); i++) {
                if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_ESSENTIALS) continue;
                total += inventory[i].equipment.size();
                for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                    if (inventory[i].equipment[j].checked) checked++;
                    if (inventory[i].equipment[j].packed) packed++;
                }
            }
        } else if (tabNum == 4) { // Optional
            for (size_t i = 0; i < inventory.size(); i++) {
                if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_OPTIONAL) continue;
                total += inventory[i].equipment.size();
                for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                    if (inventory[i].equipment[j].taking) taking++;
                    if (inventory[i].equipment[j].taking && inventory[i].equipment[j].checked) checked++;
                    if (inventory[i].equipment[j].taking && inventory[i].equipment[j].packed) packed++;
                }
            }
        }
        
        String json = "{\"total\":" + String(total) + ",\"taking\":" + String(taking) + 
                     ",\"needCheck\":" + String(taking > 0 ? taking - checked : total - checked) + 
                     ",\"needPack\":" + String(taking > 0 ? taking - packed : total - packed) + "}";
        server.send(200, "application/json", json);
    } else {
        // Original consumables stats
        int okCount = 0, lowCount = 0, outCount = 0;
        for (size_t i = 0; i < inventory.size(); i++) {
            if (!inventory[i].isConsumable) continue;
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                if (inventory[i].consumables[j].status <= STATUS_OK) okCount++;
                else if (inventory[i].consumables[j].status == STATUS_LOW) lowCount++;
                else if (inventory[i].consumables[j].status == STATUS_OUT) outCount++;
            }
        }

        String json = "{\"ok\":" + String(okCount) + ",\"low\":" + String(lowCount) + ",\"out\":" + String(outCount) + "}";
        server.send(200, "application/json", json);
    }
}

void handleInventoryDownload() {
    String txt = "SHOPPING LIST\n";
    txt += "=============\n\n";

    for (size_t i = 0; i < inventory.size(); i++) {
        if (!inventory[i].isConsumable) continue;
        bool hasItems = false;
        String categoryItems = "";

        for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
            if (inventory[i].consumables[j].status == STATUS_LOW || inventory[i].consumables[j].status == STATUS_OUT) {
                if (!hasItems) {
                    categoryItems += inventory[i].name + ":\n";
                    hasItems = true;
                }
                categoryItems += "  - " + inventory[i].consumables[j].name;
                categoryItems += " [" + String(getStatusName(inventory[i].consumables[j].status)) + "]\n";
            }
        }

        if (hasItems) {
            txt += categoryItems + "\n";
        }
    }

    if (txt.length() <= 25) {
        txt += "All items in stock!\n";
    }

    server.sendHeader("Content-Disposition", "attachment; filename=shopping-list.txt");
    server.send(200, "text/plain", txt);
}

// ============ CSV EXPORT/IMPORT HANDLERS ============

void handleInventoryExportCSV() {
    String csv = "Category Type,Category,Item Name,Status,Lives in Trailer,Checked,Packed,Taking,Last Updated\n";
    
    for (size_t i = 0; i < inventory.size(); i++) {
        String categoryType;
        if (inventory[i].isConsumable) {
            categoryType = "CONSUMABLES"; // Clear indication this goes in Consumables tab
        } else {
            // For equipment, show which tab it appears in
            switch (inventory[i].subcategory) {
                case SUBCATEGORY_TRAILER: categoryType = "TRAILER"; break;
                case SUBCATEGORY_ESSENTIALS: categoryType = "ESSENTIALS"; break;
                case SUBCATEGORY_OPTIONAL: categoryType = "OPTIONAL"; break;
                default: categoryType = "UNKNOWN"; break;
            }
        }
        
        String categoryName = inventory[i].name;
        categoryName.replace("\"", "\"\""); // Escape quotes
        
        // Export consumable items
        if (inventory[i].isConsumable) {
            for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                String itemName = inventory[i].consumables[j].name;
                itemName.replace("\"", "\"\""); // Escape quotes
                
                csv += categoryType + ",";
                csv += "\"" + categoryName + "\",";
                csv += "\"" + itemName + "\",";
                csv += getStatusName(inventory[i].consumables[j].status);
                csv += ",";
                csv += (inventory[i].consumables[j].livesInTrailer ? "Yes" : "No");
                csv += ",N/A,N/A,N/A,"; // Consumables don't have checked/packed/taking
                csv += String(inventory[i].consumables[j].lastUpdated) + "\n";
            }
        } else {
            // Export equipment items
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                String itemName = inventory[i].equipment[j].name;
                itemName.replace("\"", "\"\""); // Escape quotes
                
                csv += categoryType + ",";
                csv += "\"" + categoryName + "\",";
                csv += "\"" + itemName + "\",";
                csv += "OK,"; // Equipment always has OK status
                csv += (inventory[i].equipment[j].livesInTrailer ? "Yes" : "No");
                csv += ",";
                csv += (inventory[i].equipment[j].checked ? "Yes" : "No");
                csv += ",";
                csv += (inventory[i].equipment[j].packed ? "Yes" : "No");
                csv += ",";
                csv += (inventory[i].equipment[j].taking ? "Yes" : "No");
                csv += ",";
                csv += String(inventory[i].equipment[j].lastUpdated) + "\n";
            }
        }
    }
    
    server.sendHeader("Content-Disposition", "attachment; filename=trailer-inventory.csv");
    server.send(200, "text/csv", csv);
}

void handleInventoryImportCSV() {
    if (!server.hasArg("csvdata")) {
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Import CSV</title>";
        html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
        html += "<style>body{font-family:Arial;padding:20px;background:#000;color:#fff}";
        html += ".form-group{margin:15px 0}label{display:block;margin-bottom:5px;font-weight:bold}";
        html += "textarea{width:100%;height:400px;padding:10px;border:1px solid #444;background:#222;color:#fff;border-radius:5px}";
        html += ".btn{padding:12px 20px;background:#4af;color:#000;border:none;border-radius:5px;cursor:pointer;font-weight:bold;margin-right:10px}";
        html += ".btn:hover{background:#3af}.warning{background:#ff6b35;color:#fff;padding:15px;border-radius:5px;margin:15px 0}";
        html += "</style></head><body>";
        
        html += "<h2>📊 Import Complete Inventory from CSV</h2>";
        html += "<div class='warning'>⚠️ <strong>Warning:</strong> This will completely replace your current inventory with the CSV data. Make sure you have a backup!</div>";
        
        html += "<form method='post'>";
        html += "<div class='form-group'>";
        html += "<label>Paste CSV Data:</label>";
        html += "<textarea name='csvdata' placeholder='Category Type,Category,Item Name,Status,Lives in Trailer,Checked,Packed,Taking,Last Updated&#10;TRAILER,Kitchen Equipment,Coffee Maker,OK,Yes,Yes,Yes,Yes,1728238800000&#10;ESSENTIALS,Food & Beverages,Milk,Out,No,N/A,N/A,N/A,1728238900000'></textarea>";
        html += "</div>";
        html += "<button type='submit' class='btn'>Import CSV Data</button>";
        html += "<button type='button' class='btn' onclick='window.location.href=\"/inventory\"' style='background:#666'>Cancel</button>";
        html += "</form>";
        
        html += "<h3>📋 CSV Format & Category Management:</h3>";
        html += "<p>Expected columns: Category Type, Category, Item Name, Status, Lives in Trailer, Checked, Packed, Taking, Last Updated</p>";
        html += "<h4>Category Types & Tabs:</h4>";
        html += "<ul>";
        html += "<li><strong>CONSUMABLES:</strong> Food/supplies that get used up - shows on Consumables tab</li>";
        html += "<li><strong>TRAILER:</strong> Equipment permanently in trailer - shows on Trailer tab</li>";
        html += "<li><strong>ESSENTIALS:</strong> Equipment needed every trip - shows on Essentials tab</li>";
        html += "<li><strong>OPTIONAL:</strong> Equipment for specific trips - shows on Optional tab</li>";
        html += "</ul>";
        html += "<p><strong>New Categories:</strong> Use 'CONSUMABLES' for food/supplies, or TRAILER/ESSENTIALS/OPTIONAL for equipment.</p>";
        html += "<h4>Moving Items Between Tabs:</h4>";
        html += "<p>Consumables always show on Consumables tab regardless of Category Type.</p>";
        html += "<p>Equipment items are assigned to tabs by Category Type:</p>";
        html += "<ol>";
        html += "<li>Change 'Category Type' column: TRAILER/ESSENTIALS/OPTIONAL</li>";
        html += "<li>For UNKNOWN categories: Assign them to TRAILER, ESSENTIALS, or OPTIONAL</li>";
        html += "<li>Consumables→Equipment: Change to equipment status, set Checked/Packed values</li>";
        html += "<li>Equipment→Consumables: Set Checked/Packed/Taking to 'N/A', use consumable status</li>";
        html += "<li>Import the modified CSV</li>";
        html += "</ol>";
        html += "<h4>Adding New Categories:</h4>";
        html += "<p>Simply use a new Category name in the CSV - it will be created automatically!</p>";
        html += "<p><strong>Status:</strong> Full/OK/Low/Out (consumables), OK (equipment)</p>";
        html += "<p><strong>Boolean fields:</strong> Yes/No or N/A for not applicable</p>";
        
        html += "</body></html>";
        server.send(200, "text/html", html);
        return;
    }
    
    // Process CSV data
    String csvData = server.arg("csvdata");
    Serial.println("[CSV] Starting import...");
    
    // Clear existing inventory
    inventory.clear();
    
    // Detect separator from header line
    char separator = ',';
    int firstLineEnd = csvData.indexOf('\n');
    if (firstLineEnd > 0) {
        String headerLine = csvData.substring(0, firstLineEnd);
        int commas = 0, semicolons = 0;
        for (int i = 0; i < headerLine.length(); i++) {
            if (headerLine.charAt(i) == ',') commas++;
            else if (headerLine.charAt(i) == ';') semicolons++;
        }
        separator = (semicolons > commas) ? ';' : ',';
        Serial.printf("[CSV] Detected separator: '%c' (commas: %d, semicolons: %d)\n", separator, commas, semicolons);
    }
    
    // Parse CSV line by line
    int lineNum = 0;
    int startPos = 0;
    int imported = 0;
    
    while (startPos < csvData.length()) {
        int endPos = csvData.indexOf('\n', startPos);
        if (endPos == -1) endPos = csvData.length();
        
        String line = csvData.substring(startPos, endPos);
        line.trim();
        
        if (lineNum > 0 && line.length() > 0) { // Skip header and empty lines
            // Parse CSV line - using detected separator
            std::vector<String> fields;
            int fieldStart = 0;
            bool inQuotes = false;
            
            for (int i = 0; i <= line.length(); i++) {
                char c = (i < line.length()) ? line.charAt(i) : separator;
                
                if (c == '"') {
                    inQuotes = !inQuotes;
                } else if (c == separator && !inQuotes) {
                    String field = line.substring(fieldStart, i);
                    field.trim();
                    if (field.startsWith("\"") && field.endsWith("\"")) {
                        field = field.substring(1, field.length() - 1);
                        field.replace("\"\"", "\""); // Unescape quotes
                    }
                    fields.push_back(field);
                    fieldStart = i + 1;
                }
            }
            
            if (fields.size() >= 8) {
                String categoryTypeStr = fields[0];
                String categoryName = fields[1];
                String itemName = fields[2];
                String statusStr = fields[3];
                String livesInTrailerStr = fields[4];
                String checkedStr = fields[5];
                String packedStr = fields[6];
                String takingStr = fields[7];
                
                // Find or create category
                int categoryIndex = -1;
                for (size_t i = 0; i < inventory.size(); i++) {
                    if (inventory[i].name == categoryName) {
                        categoryIndex = i;
                        break;
                    }
                }
                
                if (categoryIndex == -1) {
                    // Create new category
                    DynamicCategory newCategory;
                    newCategory.name = categoryName;
                    newCategory.icon = "📦"; // Default icon
                    
                    // Determine if consumable and set subcategory
                    if (categoryTypeStr == "CONSUMABLES") {
                        newCategory.isConsumable = true;
                        newCategory.subcategory = SUBCATEGORY_TRAILER; // Consumables always use SUBCATEGORY_TRAILER
                    } else {
                        newCategory.isConsumable = false; // Equipment
                        if (categoryTypeStr == "TRAILER") newCategory.subcategory = SUBCATEGORY_TRAILER;
                        else if (categoryTypeStr == "ESSENTIALS") newCategory.subcategory = SUBCATEGORY_ESSENTIALS;
                        else if (categoryTypeStr == "OPTIONAL") newCategory.subcategory = SUBCATEGORY_OPTIONAL;
                        else {
                            // Fallback: auto-detect based on status
                            newCategory.isConsumable = (statusStr != "OK" || checkedStr == "N/A");
                            newCategory.subcategory = newCategory.isConsumable ? SUBCATEGORY_TRAILER : SUBCATEGORY_OPTIONAL;
                        }
                    }
                    
                    inventory.push_back(newCategory);
                    categoryIndex = inventory.size() - 1;
                }
                
                // Add item to category
                if (inventory[categoryIndex].isConsumable) {
                    ConsumableItem item;
                    item.name = itemName;
                    item.livesInTrailer = (livesInTrailerStr == "Yes");
                    
                    // Parse status
                    if (statusStr == "Low") item.status = STATUS_LOW;
                    else if (statusStr == "Out") item.status = STATUS_OUT;
                    else item.status = STATUS_OK;
                    
                    item.lastUpdated = millis();
                    inventory[categoryIndex].consumables.push_back(item);
                } else {
                    EquipmentItem item;
                    item.name = itemName;
                    item.livesInTrailer = (livesInTrailerStr == "Yes");
                    item.checked = (checkedStr == "Yes");
                    item.packed = (packedStr == "Yes");
                    item.taking = (takingStr == "Yes");
                    item.lastUpdated = millis();
                    inventory[categoryIndex].equipment.push_back(item);
                }
                
                imported++;
            }
        }
        
        lineNum++;
        startPos = endPos + 1;
    }
    
    // Save to SPIFFS
    if (saveInventoryToSPIFFS()) {
        Serial.printf("[CSV] Import complete: %d items imported\n", imported);
        server.sendHeader("Location", "/inventory?imported=" + String(imported));
        server.send(303);
    } else {
        server.send(500, "text/plain", "Failed to save imported data");
    }
}

void handleInventoryAddItem() {
    if (!server.hasArg("cat") || !server.hasArg("name")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    String name = server.arg("name");
    name.trim();

    if (name.length() == 0 || name.length() > 100) {
        server.send(400, "text/plain", "Invalid item name");
        return;
    }

    if (cat >= 0 && cat < (int)inventory.size()) {
        if (inventory[cat].isConsumable) {
            bool livesInTrailer = false;
            if (server.hasArg("livesInTrailer")) {
                livesInTrailer = (server.arg("livesInTrailer") == "true");
            }
            inventory[cat].consumables.push_back(ConsumableItem(name, STATUS_FULL, livesInTrailer));
            Serial.printf("[INVENTORY] Added consumable '%s' to category %d (livesInTrailer: %s)\n", name.c_str(), cat, livesInTrailer ? "true" : "false");
        } else {
            inventory[cat].equipment.push_back(EquipmentItem(name, false, false));
            Serial.printf("[INVENTORY] Added equipment '%s' to category %d\n", name.c_str(), cat);
        }
        sortCategoryItems(inventory[cat]);  // Sort alphabetically after adding
        saveInventoryToSPIFFS();  // Auto-save on add
        server.send(200, "text/plain", "OK");
        return;
    }

    server.send(400, "text/plain", "Invalid category");
}

void handleInventoryRemoveItem() {
    if (!server.hasArg("cat") || !server.hasArg("item")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    int item = server.arg("item").toInt();

    if (cat >= 0 && cat < (int)inventory.size() && item >= 0) {
        if (inventory[cat].isConsumable && item < (int)inventory[cat].consumables.size()) {
            String itemName = inventory[cat].consumables[item].name;
            inventory[cat].consumables.erase(inventory[cat].consumables.begin() + item);
            Serial.printf("[INVENTORY] Removed consumable '%s' from category %d\n", itemName.c_str(), cat);
            saveInventoryToSPIFFS();  // Auto-save on remove
            server.send(200, "text/plain", "OK");
            return;
        } else if (!inventory[cat].isConsumable && item < (int)inventory[cat].equipment.size()) {
            String itemName = inventory[cat].equipment[item].name;
            inventory[cat].equipment.erase(inventory[cat].equipment.begin() + item);
            Serial.printf("[INVENTORY] Removed equipment '%s' from category %d\n", itemName.c_str(), cat);
            saveInventoryToSPIFFS();  // Auto-save on remove
            server.send(200, "text/plain", "OK");
            return;
        }
    }

    server.send(400, "text/plain", "Invalid parameters");
}

void handleInventoryAddCategory() {
    if (!server.hasArg("name") || !server.hasArg("type") || !server.hasArg("icon")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    String name = server.arg("name");
    String type = server.arg("type");
    String icon = server.arg("icon");
    String subcategoryStr = server.arg("subcategory");  // Optional parameter
    
    name.trim();
    icon.trim();

    if (name.length() == 0 || name.length() > 50) {
        server.send(400, "text/plain", "Invalid category name (1-50 chars)");
        return;
    }

    if (icon.length() == 0 || icon.length() > 10) {
        server.send(400, "text/plain", "Invalid icon");
        return;
    }

    bool isConsumable = (type == "consumable");
    Subcategory subcategory = SUBCATEGORY_TRAILER;

    // Determine subcategory for equipment
    if (!isConsumable) {
        if (subcategoryStr == "trailer") {
            subcategory = SUBCATEGORY_TRAILER;
        } else if (subcategoryStr == "essentials") {
            subcategory = SUBCATEGORY_ESSENTIALS;
        } else if (subcategoryStr == "optional") {
            subcategory = SUBCATEGORY_OPTIONAL;
        }
        // No default assignment - must be explicitly set
    }

    // Check if category name already exists
    for (size_t i = 0; i < inventory.size(); i++) {
        if (inventory[i].name.equalsIgnoreCase(name)) {
            server.send(400, "text/plain", "Category name already exists");
            return;
        }
    }

    // Create new category
    DynamicCategory newCategory(name, icon, isConsumable, subcategory);

    // Add to inventory
    inventory.push_back(newCategory);
    
    Serial.printf("[INVENTORY] Added new %s category '%s' (subcategory: %d) with icon '%s'\n", 
                  isConsumable ? "consumable" : "equipment", name.c_str(), subcategory, icon.c_str());
    
    saveInventoryToSPIFFS();  // Auto-save on add
    server.send(200, "text/plain", "OK");
}

void handleInventoryRenameItem() {
    if (!server.hasArg("cat") || !server.hasArg("item") || !server.hasArg("name")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    int item = server.arg("item").toInt();
    String newName = server.arg("name");
    newName.trim();

    if (newName.length() == 0 || newName.length() > 100) {
        server.send(400, "text/plain", "Invalid item name");
        return;
    }

    if (cat >= 0 && cat < (int)inventory.size() && item >= 0) {
        if (inventory[cat].isConsumable && item < (int)inventory[cat].consumables.size()) {
            String oldName = inventory[cat].consumables[item].name;
            inventory[cat].consumables[item].name = newName;
            Serial.printf("[INVENTORY] Renamed consumable '%s' to '%s' in category %d\n", oldName.c_str(), newName.c_str(), cat);
            sortCategoryItems(inventory[cat]);  // Re-sort after rename
            saveInventoryToSPIFFS();  // Auto-save on rename
            server.send(200, "text/plain", "OK");
            return;
        } else if (!inventory[cat].isConsumable && item < (int)inventory[cat].equipment.size()) {
            String oldName = inventory[cat].equipment[item].name;
            inventory[cat].equipment[item].name = newName;
            Serial.printf("[INVENTORY] Renamed equipment '%s' to '%s' in category %d\n", oldName.c_str(), newName.c_str(), cat);
            sortCategoryItems(inventory[cat]);  // Re-sort after rename
            saveInventoryToSPIFFS();  // Auto-save on rename
            server.send(200, "text/plain", "OK");
            return;
        }
    }

    server.send(400, "text/plain", "Invalid parameters");
}

void handleInventoryEditConsumable() {
    if (!server.hasArg("cat") || !server.hasArg("item") || !server.hasArg("name") || !server.hasArg("livesInTrailer")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    int item = server.arg("item").toInt();
    String newName = server.arg("name");
    bool livesInTrailer = (server.arg("livesInTrailer") == "true");
    newName.trim();

    if (newName.length() == 0 || newName.length() > 100) {
        server.send(400, "text/plain", "Invalid item name");
        return;
    }

    if (cat >= 0 && cat < (int)inventory.size() && item >= 0) {
        if (inventory[cat].isConsumable && item < (int)inventory[cat].consumables.size()) {
            String oldName = inventory[cat].consumables[item].name;
            bool oldTrailer = inventory[cat].consumables[item].livesInTrailer;
            
            inventory[cat].consumables[item].name = newName;
            inventory[cat].consumables[item].livesInTrailer = livesInTrailer;
            
            Serial.printf("[INVENTORY] Updated consumable '%s' to '%s', livesInTrailer: %s->%s in category %d\n", 
                         oldName.c_str(), newName.c_str(), 
                         oldTrailer ? "true" : "false", 
                         livesInTrailer ? "true" : "false", cat);
            
            sortCategoryItems(inventory[cat]);  // Re-sort after edit
            saveInventoryToSPIFFS();  // Auto-save on edit
            server.send(200, "text/plain", "OK");
            return;
        }
    }

    server.send(400, "text/plain", "Invalid parameters or not a consumable item");
}

void handleInventoryMoveItem() {
    if (!server.hasArg("cat") || !server.hasArg("item") || !server.hasArg("target")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int sourceCat = server.arg("cat").toInt();
    int itemIndex = server.arg("item").toInt();
    int targetCat = server.arg("target").toInt();

    if (sourceCat >= 0 && sourceCat < (int)inventory.size() && 
        targetCat >= 0 && targetCat < (int)inventory.size() && 
        itemIndex >= 0 && sourceCat != targetCat) {
        
        // Check if source and target are same type (consumable vs equipment)
        if (inventory[sourceCat].isConsumable != inventory[targetCat].isConsumable) {
            server.send(400, "text/plain", "Cannot move between consumable and equipment categories");
            return;
        }

        if (inventory[sourceCat].isConsumable && itemIndex < (int)inventory[sourceCat].consumables.size()) {
            // Move consumable item
            ConsumableItem item = inventory[sourceCat].consumables[itemIndex];
            inventory[sourceCat].consumables.erase(inventory[sourceCat].consumables.begin() + itemIndex);
            inventory[targetCat].consumables.push_back(item);
            Serial.printf("[INVENTORY] Moved consumable '%s' from category %d to %d\n", item.name.c_str(), sourceCat, targetCat);
        } else if (!inventory[sourceCat].isConsumable && itemIndex < (int)inventory[sourceCat].equipment.size()) {
            // Move equipment item
            EquipmentItem item = inventory[sourceCat].equipment[itemIndex];
            inventory[sourceCat].equipment.erase(inventory[sourceCat].equipment.begin() + itemIndex);
            inventory[targetCat].equipment.push_back(item);
            Serial.printf("[INVENTORY] Moved equipment '%s' from category %d to %d\n", item.name.c_str(), sourceCat, targetCat);
        } else {
            server.send(400, "text/plain", "Invalid item index");
            return;
        }

        // Re-sort both categories
        sortCategoryItems(inventory[sourceCat]);
        sortCategoryItems(inventory[targetCat]);
        saveInventoryToSPIFFS();  // Auto-save on move
        server.send(200, "text/plain", "OK");
        return;
    }

    server.send(400, "text/plain", "Invalid parameters");
}

void handleInventoryDeleteItem() {
    if (!server.hasArg("cat") || !server.hasArg("item")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    int item = server.arg("item").toInt();

    if (cat >= 0 && cat < (int)inventory.size() && item >= 0) {
        if (inventory[cat].isConsumable && item < (int)inventory[cat].consumables.size()) {
            String itemName = inventory[cat].consumables[item].name;
            inventory[cat].consumables.erase(inventory[cat].consumables.begin() + item);
            Serial.printf("[INVENTORY] Deleted consumable '%s' from category %d\n", itemName.c_str(), cat);
            saveInventoryToSPIFFS();
            server.send(200, "text/plain", "OK");
            return;
        } else if (!inventory[cat].isConsumable && item < (int)inventory[cat].equipment.size()) {
            String itemName = inventory[cat].equipment[item].name;
            inventory[cat].equipment.erase(inventory[cat].equipment.begin() + item);
            Serial.printf("[INVENTORY] Deleted equipment '%s' from category %d\n", itemName.c_str(), cat);
            saveInventoryToSPIFFS();
            server.send(200, "text/plain", "OK");
            return;
        }
    }

    server.send(400, "text/plain", "Invalid parameters");
}

void handleCategoryRename() {
    if (!server.hasArg("cat") || !server.hasArg("name")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();
    String newName = server.arg("name");
    newName.trim();

    if (newName.length() == 0 || newName.length() > 100) {
        server.send(400, "text/plain", "Invalid category name");
        return;
    }

    if (cat >= 0 && cat < (int)inventory.size()) {
        String oldName = inventory[cat].name;
        inventory[cat].name = newName;
        Serial.printf("[INVENTORY] Renamed category '%s' to '%s'\n", oldName.c_str(), newName.c_str());
        saveInventoryToSPIFFS();
        server.send(200, "text/plain", "OK");
        return;
    }

    server.send(400, "text/plain", "Invalid category");
}

void handleCategoryDelete() {
    if (!server.hasArg("cat")) {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    int cat = server.arg("cat").toInt();

    if (cat >= 0 && cat < (int)inventory.size()) {
        // Don't allow deleting if it's the last category
        if (inventory.size() <= 1) {
            server.send(400, "text/plain", "Cannot delete the last category");
            return;
        }

        String catName = inventory[cat].name;
        int itemCount = inventory[cat].isConsumable ? inventory[cat].consumables.size() : inventory[cat].equipment.size();
        
        inventory.erase(inventory.begin() + cat);
        Serial.printf("[INVENTORY] Deleted category '%s' with %d items\n", catName.c_str(), itemCount);
        saveInventoryToSPIFFS();
        server.send(200, "text/plain", "OK");
        return;
    }

    server.send(400, "text/plain", "Invalid category");
}

// ============ SORTING FUNCTIONS ============

// Sort items alphabetically within a category (case-insensitive)
void sortCategoryItems(DynamicCategory& category) {
    if (category.isConsumable) {
        std::sort(category.consumables.begin(), category.consumables.end(),
                  [](const ConsumableItem& a, const ConsumableItem& b) {
                      String aLower = a.name;
                      String bLower = b.name;
                      aLower.toLowerCase();
                      bLower.toLowerCase();
                      return aLower < bLower;
                  });
    } else {
        std::sort(category.equipment.begin(), category.equipment.end(),
                  [](const EquipmentItem& a, const EquipmentItem& b) {
                      String aLower = a.name;
                      String bLower = b.name;
                      aLower.toLowerCase();
                      bLower.toLowerCase();
                      return aLower < bLower;
                  });
    }
}

// Sort all categories alphabetically
void sortAllInventory() {
    for (size_t i = 0; i < inventory.size(); i++) {
        sortCategoryItems(inventory[i]);
    }
}

void handleInventoryReset() {
    Serial.println("[INVENTORY] Smart Reset - Creating backup and saving state...");
    
    // Create backup before reset
    createBackup("auto_reset");
    
    // Save current state to preserve customizations
    saveInventoryToSPIFFS();
    Serial.println("[INVENTORY] Current state saved with backup");
    
    // Clear memory and reload from saved data
    inventory.clear();
    loadInventoryFromSPIFFS();
    Serial.printf("[INVENTORY] Reset complete - %d categories loaded\n", inventory.size());
    String html = createStyledConfirmationPage(
        "Smart Reset Complete", 
        "🔄", 
        "✓ Backup created successfully (auto_reset)<br>✓ Current state saved and reloaded<br>✓ All customizations preserved<br><br><strong>Your inventory is refreshed and ready!</strong>", 
        "📦 View Inventory", 
        "/inventory",
        "success"
    );
    server.send(200, "text/html", html);
}

bool createBackup(String backupName) {
    if (!SPIFFS.exists("/inventory.json")) {
        Serial.println("[BACKUP] No inventory.json to backup");
        return false;
    }
    
    // Add timestamp to backup name for better identification
    String timestamp = String(millis() / 1000);  // Simple timestamp
    String backupPath = "/backup_" + backupName + ".json";
    
    // Check available space before creating backup
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    
    // Estimate backup size (same as inventory.json)
    File tempCheck = SPIFFS.open("/inventory.json", "r");
    size_t inventorySize = 0;
    if (tempCheck) {
        inventorySize = tempCheck.size();
        tempCheck.close();
    }
    
    Serial.printf("[BACKUP] SPIFFS: %d/%d bytes used (%.1f%%), need ~%d bytes\n", 
                  usedBytes, totalBytes, (usedBytes * 100.0 / totalBytes), inventorySize);
    
    // Auto-cleanup if low on space (less than 10KB free or backup won't fit)
    if (freeBytes < 10240 || freeBytes < (inventorySize + 1024)) {
        Serial.println("[BACKUP] Low on space, attempting cleanup...");
        cleanupOldBackups();
    }
    
    // Read current inventory
    File sourceFile = SPIFFS.open("/inventory.json", "r");
    if (!sourceFile) {
        Serial.println("[BACKUP] Failed to open source file");
        return false;
    }
    
    // Create backup file  
    File backupFile = SPIFFS.open(backupPath, "w");
    if (!backupFile) {
        sourceFile.close();
        Serial.println("[BACKUP] Failed to create backup file");
        return false;
    }
    
    // Copy data
    while (sourceFile.available()) {
        backupFile.write(sourceFile.read());
    }
    
    sourceFile.close();
    backupFile.close();
    
    Serial.printf("[BACKUP] Created backup: %s (%.1fKB)\n", backupPath.c_str(), inventorySize/1024.0);
    return true;
}

void cleanupOldBackups() {
    // Clean up old numbered backups first (backup2, backup3, etc.)
    for (int i = 5; i >= 3; i--) {
        String oldBackup = "/backup_backup" + String(i) + ".json";
        if (SPIFFS.exists(oldBackup)) {
            SPIFFS.remove(oldBackup);
            Serial.printf("[CLEANUP] Removed old backup: %s\n", oldBackup.c_str());
        }
    }
    
    // If still low on space, remove backup2
    size_t freeBytes = SPIFFS.totalBytes() - SPIFFS.usedBytes();
    if (freeBytes < 8192) {  // Less than 8KB free
        if (SPIFFS.exists("/backup_backup2.json")) {
            SPIFFS.remove("/backup_backup2.json");
            Serial.println("[CLEANUP] Removed backup2 due to low space");
        }
    }
}

String getStorageInfo() {
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    float usedPercent = (usedBytes * 100.0 / totalBytes);
    
    return String(usedBytes / 1024) + "KB / " + String(totalBytes / 1024) + "KB used (" + 
           String(usedPercent, 1) + "%) - " + String(freeBytes / 1024) + "KB free";
}

String createStyledConfirmationPage(String title, String icon, String message, String buttonText, String buttonUrl, String buttonColor) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no'>";
    html += "<title>" + title + "</title>";
    html += "<style>";
    html += "*{margin:0;padding:0;box-sizing:border-box;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}";
    html += "body{background:#000;color:#fff;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;padding:20px;line-height:1.4;min-height:100vh;display:flex;align-items:center;justify-content:center}";
    html += ".container{max-width:500px;width:100%;text-align:center}";
    html += ".card{background:linear-gradient(135deg,#1a1a1a,#111);padding:30px;border-radius:16px;box-shadow:0 8px 24px rgba(0,0,0,0.4);border:1px solid #333}";
    html += ".icon{font-size:4em;margin-bottom:15px;display:block;filter:drop-shadow(0 4px 8px rgba(0,0,0,0.3))}";
    html += ".title{font-size:1.8em;font-weight:600;margin-bottom:15px;color:#fff}";
    html += ".message{font-size:1.1em;margin-bottom:25px;color:#ddd;line-height:1.6}";
    html += ".btn{padding:14px 24px;border:none;border-radius:10px;font-size:1.1em;font-weight:600;cursor:pointer;text-decoration:none;display:inline-block;margin:8px;transition:all 0.2s;box-shadow:0 4px 12px rgba(0,0,0,0.3)}";
    html += ".btn-primary{background:linear-gradient(135deg,#667eea,#764ba2);color:#fff}";
    html += ".btn-success{background:linear-gradient(135deg,#10b981,#059669);color:#fff}";
    html += ".btn-danger{background:linear-gradient(135deg,#ef4444,#dc2626);color:#fff}";
    html += ".btn-secondary{background:rgba(255,255,255,0.1);color:#fff;border:1px solid rgba(255,255,255,0.2)}";
    html += ".btn:hover{transform:translateY(-2px);box-shadow:0 6px 16px rgba(0,0,0,0.4)}";
    html += ".secondary-links{margin-top:20px;font-size:0.85em;display:flex;justify-content:center;gap:15px}";
    html += ".secondary-links a{color:#999;text-decoration:none;padding:10px 15px;border-radius:8px;transition:all 0.2s;text-align:center;min-width:70px;display:flex;flex-direction:column;align-items:center}";
    html += ".secondary-links a span{font-size:1.4em;margin-bottom:2px;display:block}";
    html += ".secondary-links a:hover{background:rgba(255,255,255,0.1);color:#fff;transform:translateY(-1px)}";
    
    // Tablet/iPad optimization
    html += "@media(min-width:768px){";
    html += "body{padding:30px}";
    html += ".card{padding:40px;max-width:600px}";
    html += ".icon{font-size:5em;margin-bottom:20px}";
    html += ".title{font-size:2.2em;margin-bottom:18px}";
    html += ".message{font-size:1.2em;margin-bottom:30px}";
    html += ".btn{padding:16px 28px;font-size:1.2em;margin:10px}";
    html += ".secondary-links{gap:20px;font-size:0.9em}";
    html += ".secondary-links a{padding:12px 18px;min-width:80px}";
    html += ".secondary-links a span{font-size:1.5em;margin-bottom:3px}";
    html += "}";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<div class='card'>";
    html += "<div class='icon'>" + icon + "</div>";
    html += "<h1 class='title'>" + title + "</h1>";
    html += "<div class='message'>" + message + "</div>";
    html += "<a href='" + buttonUrl + "' class='btn btn-" + buttonColor + "'>" + buttonText + "</a>";
    html += "<div class='secondary-links'>";
    html += "<a href='/inventory/backups'><span>📄</span><br>Backups</a>";
    html += "<a href='/inventory'><span>📦</span><br>Inventory</a>";
    html += "<a href='/'><span>🏠</span><br>Home</a>";
    html += "</div>";
    html += "</div></div>";
    
    html += "</body></html>";
    return html;
}

void rotateBackups() {
    // Keep 3 backups: current, backup1, backup2
    
    // Remove oldest backup
    SPIFFS.remove("/backup_backup2.json");
    
    // Shift backups
    if (SPIFFS.exists("/backup_backup1.json")) {
        SPIFFS.rename("/backup_backup1.json", "/backup_backup2.json");
    }
    
    if (SPIFFS.exists("/backup_current.json")) {
        SPIFFS.rename("/backup_current.json", "/backup_backup1.json");
    }
    
    Serial.println("[BACKUP] Backup rotation complete");
}

void handleInventorySaveAsDefaults() {
    Serial.println("[INVENTORY] Smart Save - Creating backup first...");
    
    // Rotate existing backups and create new backup
    rotateBackups();
    createBackup("current");
    
    // Save current state as new defaults
    if (saveInventoryToSPIFFS()) {
        Serial.println("[INVENTORY] ✓ Current state saved as new defaults (with backup)");
        String html = createStyledConfirmationPage(
            "Defaults Updated Successfully", 
            "💾", 
            "✓ Backup created before saving<br>✓ Current inventory state saved as new defaults<br>✓ Future resets will restore this configuration<br><br><strong>Your new defaults are now active!</strong>", 
            "📦 Return to Inventory", 
            "/inventory",
            "success"
        );
        server.send(200, "text/html", html);
    } else {
        String html = createStyledConfirmationPage(
            "Save Failed", 
            "❌", 
            "Failed to save defaults to storage.<br><br>Your backup is safe, but the new defaults could not be saved. Please try again.", 
            "📦 Return to Inventory", 
            "/inventory",
            "danger"
        );
        server.send(500, "text/html", html);
    }
}

void handleInventoryBackups() {
    Serial.println("[BACKUP] Listing backups...");
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no'>";
    html += "<title>Inventory Backups</title>";
    html += "<style>";
    html += "*{margin:0;padding:0;box-sizing:border-box;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}";
    html += "body{background:#000;color:#fff;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;padding:8px;line-height:1.3;font-size:15px}";
    html += ".header{text-align:center;padding:15px;background:linear-gradient(135deg,#1a1a1a,#111);border-radius:12px;margin-bottom:15px}";
    html += ".header h1{font-size:1.4em;margin-bottom:8px}";
    
    // Storage info card
    html += ".storage-info{background:linear-gradient(135deg,#1a1a2e,#16213e);padding:12px;border-radius:10px;margin-bottom:15px;border-left:3px solid #4af}";
    html += ".storage-bar{background:#333;border-radius:6px;height:8px;margin:8px 0;position:relative;overflow:hidden}";
    html += ".storage-fill{background:linear-gradient(90deg,#4af,#0ea5e9);height:100%;border-radius:6px;transition:width 0.3s}";
    html += ".storage-text{font-size:0.9em;opacity:0.9;display:flex;justify-content:space-between}";
    
    // Backup cards
    html += ".backup-card{background:linear-gradient(135deg,#1a1a1a,#111);margin:8px 0;padding:12px;border-radius:12px;border-left:3px solid #333;box-shadow:0 4px 6px rgba(0,0,0,0.3);position:relative}";
    html += ".backup-card.current{border-left-color:#10b981;box-shadow:0 4px 6px rgba(16,185,129,0.2)}";
    html += ".backup-card.auto{border-left-color:#3b82f6;box-shadow:0 4px 6px rgba(59,130,246,0.2)}";
    html += ".backup-card.daily{border-left-color:#f59e0b;box-shadow:0 4px 6px rgba(245,158,11,0.2)}";
    html += ".backup-card.weekly{border-left-color:#8b5cf6;box-shadow:0 4px 6px rgba(139,92,246,0.2)}";
    html += ".backup-card.monthly{border-left-color:#06b6d4;box-shadow:0 4px 6px rgba(6,182,212,0.2)}";
    html += ".backup-card.quarterly{border-left-color:#f97316;box-shadow:0 4px 6px rgba(249,115,22,0.2)}";
    html += ".backup-card.old{border-left-color:#6b7280;opacity:0.8}";
    html += ".backup-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px}";
    html += ".backup-name{font-size:1.1em;font-weight:600;display:flex;align-items:center;gap:6px}";
    html += ".backup-size{font-size:0.8em;opacity:0.7;background:rgba(255,255,255,0.1);padding:2px 8px;border-radius:4px}";
    html += ".backup-desc{font-size:0.85em;opacity:0.8;margin-bottom:10px}";
    html += ".backup-actions{display:flex;gap:8px;flex-wrap:wrap}";
    html += ".btn{padding:8px 16px;border:none;border-radius:8px;font-size:0.9em;font-weight:500;cursor:pointer;text-decoration:none;display:inline-block;text-align:center}";
    html += ".btn-primary{background:linear-gradient(135deg,#667eea,#764ba2);color:#fff}";
    html += ".btn-danger{background:linear-gradient(135deg,#ef4444,#dc2626);color:#fff}";
    html += ".btn-secondary{background:rgba(255,255,255,0.1);color:#fff}";
    
    // Action section
    html += ".actions{background:linear-gradient(135deg,#1a1a1a,#111);padding:15px;border-radius:12px;margin-top:15px}";
    html += ".actions h3{margin-bottom:12px;color:#ddd}";
    html += ".action-grid{display:grid;grid-template-columns:1fr;gap:10px}";
    html += ".action-card{background:rgba(255,255,255,0.05);padding:12px;border-radius:10px;display:flex;justify-content:space-between;align-items:center}";
    html += ".action-desc{flex:1}";
    html += ".action-title{font-weight:600;margin-bottom:4px}";
    html += ".action-subtitle{font-size:0.8em;opacity:0.7}";
    
    // Empty state
    html += ".empty{text-align:center;padding:40px;background:rgba(255,255,255,0.05);border-radius:12px;margin:15px 0}";
    html += ".empty h3{margin-bottom:8px;color:#999}";
    
    // Tablet optimization
    html += "@media(min-width:768px){";
    html += "body{padding:12px 15px;max-width:800px;margin:0 auto;font-size:16px}";
    html += ".header{padding:15px}";
    html += ".header h1{font-size:1.6em}";
    html += ".backup-card{margin:10px 0;padding:15px}";
    html += ".backup-header{margin-bottom:10px}";
    html += ".backup-name{font-size:1.2em}";
    html += ".backup-actions{gap:10px}";
    html += ".btn{padding:10px 20px;font-size:1em}";
    html += ".action-grid{grid-template-columns:1fr 1fr;gap:12px}";
    html += ".actions{padding:18px}";
    html += "}";
    html += "</style></head><body>";
    
    // Header
    html += "<div class='header'>";
    html += "<h1>🗂️ Backup Manager</h1>";
    html += "<p>Manage inventory backups & storage</p>";
    html += "</div>";
    
    // Storage info
    String storageInfo = getStorageInfo();
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    float usedPercent = (usedBytes * 100.0 / totalBytes);
    
    html += "<div class='storage-info'>";
    html += "<div style='display:flex;align-items:center;gap:8px;margin-bottom:8px'>";
    html += "<span style='font-size:1.2em'>💾</span>";
    html += "<strong>Storage Status</strong>";
    html += "</div>";
    html += "<div class='storage-bar'>";
    html += "<div class='storage-fill' style='width:" + String(usedPercent, 1) + "%'></div>";
    html += "</div>";
    html += "<div class='storage-text'>";
    html += "<span>" + String(usedBytes/1024) + "KB used</span>";
    html += "<span>" + String((totalBytes-usedBytes)/1024) + "KB free</span>";
    html += "</div>";
    html += "<div style='font-size:0.8em;opacity:0.7;margin-top:4px'>";
    html += String(usedPercent, 1) + "% of " + String(totalBytes/1024) + "KB total";
    html += "</div></div>";
    
    // Backup files
    bool hasBackups = false;
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    
    struct BackupInfo {
        String name;
        String description;
        String cssClass;
        size_t size;
        bool isSpecial;
    };
    
    std::vector<BackupInfo> backups;
    
    while (file) {
        String fileName = file.name();
        Serial.printf("[BACKUP] Found file: %s\n", fileName.c_str());
        if (fileName.startsWith("backup_") && fileName.endsWith(".json")) {
            hasBackups = true;
            String backupName = fileName.substring(7, fileName.length() - 5);
            Serial.printf("[BACKUP] Valid backup: %s\n", backupName.c_str());
            
            BackupInfo info;
            info.name = backupName;
            info.size = file.size();
            info.isSpecial = true;
            
            if (backupName == "current") {
                info.description = "💾 Latest manual save - Your most recent saved state";
                info.cssClass = "current";
            } else if (backupName == "auto_reset") {
                info.description = "🔄 Smart reset backup - Created before last reset";
                info.cssClass = "auto";
            } else if (backupName == "before_restore") {
                info.description = "↩️ Pre-restore backup - Saved before last restore operation";
                info.cssClass = "auto";
            } else if (backupName.startsWith("daily_")) {
                info.description = "📅 Daily auto-backup - Saved automatically each day";
                info.cssClass = "daily";
            } else if (backupName.startsWith("weekly_")) {
                info.description = "📊 Weekly auto-backup - Saved every Sunday";
                info.cssClass = "weekly";
            } else if (backupName.startsWith("monthly_")) {
                info.description = "🗓️ Monthly auto-backup - Saved on 1st of month";
                info.cssClass = "monthly";
            } else if (backupName.startsWith("quarterly_")) {
                info.description = "📈 Quarterly auto-backup - Long-term archive";
                info.cssClass = "quarterly";
            } else if (backupName.startsWith("backup")) {
                info.description = "📦 Rotated backup - Automatically managed";
                info.cssClass = "old";
            } else {
                info.description = "📝 Manual backup";
                info.cssClass = "";
                info.isSpecial = false;
            }
            
            backups.push_back(info);
        }
        file = root.openNextFile();
    }
    
    Serial.printf("[BACKUP] Total backups found: %d\n", backups.size());
    
    if (hasBackups) {
        for (const auto& backup : backups) {
            html += "<div class='backup-card " + backup.cssClass + "'>";
            html += "<div class='backup-header'>";
            html += "<div class='backup-name'>";
            if (backup.name == "current") html += "🟢 ";
            else if (backup.name == "auto_reset") html += "🔄 ";
            else if (backup.name == "before_restore") html += "⏪ ";
            else html += "📦 ";
            html += backup.name;
            html += "</div>";
            html += "<div class='backup-size'>" + String(backup.size) + "B</div>";
            html += "</div>";
            html += "<div class='backup-desc'>" + backup.description + "</div>";
            html += "<div class='backup-actions'>";
            html += "<a href='/inventory/restore?backup=" + backup.name + "' class='btn btn-primary' onclick='return confirm(\"Restore from backup: " + backup.name + "?\\n\\nThis will replace your current inventory.\")'>🔄 Restore</a>";
            if (!backup.isSpecial || backup.name.startsWith("backup")) {
                html += "<a href='/inventory/delete-backup?backup=" + backup.name + "' class='btn btn-danger' onclick='return confirm(\"Delete backup: " + backup.name + "?\\n\\nThis cannot be undone.\")'>🗑️ Delete</a>";
            }
            html += "</div></div>";
        }
    } else {
        html += "<div class='empty'>";
        html += "<h3>No backups found</h3>";
        html += "<p>Create your first backup by saving your inventory</p>";
        html += "</div>";
    }
    
    // Actions section
    html += "<div class='actions'>";
    html += "<h3>🔧 Backup Actions</h3>";
    html += "<div class='action-grid'>";
    
    html += "<div class='action-card'>";
    html += "<div class='action-desc'>";
    html += "<div class='action-title'>Save as Defaults</div>";
    html += "<div class='action-subtitle'>Make current state the new reset target</div>";
    html += "</div>";
    html += "<a href='/inventory/save-as-defaults' class='btn btn-secondary' onclick='return confirm(\"Save current inventory as new defaults?\\n\\nFuture resets will restore this state.\")'>💾 Save</a>";
    html += "</div>";
    
    html += "<div class='action-card'>";
    html += "<div class='action-desc'>";
    html += "<div class='action-title'>Smart Reset</div>";
    html += "<div class='action-subtitle'>Refresh system while preserving customizations</div>";
    html += "</div>";
    html += "<a href='/inventory/reset' class='btn btn-secondary' onclick='return confirm(\"Smart Reset will:\\n\\n✓ Create backup first\\n✓ Preserve all customizations\\n✓ Refresh the system\\n\\nProceed?\")'>🔄 Smart Reset</a>";
    html += "</div>";
    
    html += "</div></div>";
    
    // Navigation buttons
    html += "<div style='display:flex;gap:8px;margin-top:20px'>";
    html += "<button class='btn btn-primary' onclick='window.location.href=\"/inventory\"' style='flex:1'>\u{1F4C4} Back to Inventory</button>";
    html += "<button class='btn btn-secondary' onclick='window.location.href=\"/\"' style='flex:1'>\u{1F3E0} Home</button>";
    html += "</div>";
    
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleInventoryRestore() {
    if (!server.hasArg("backup")) {
        server.send(400, "text/plain", "Missing backup parameter");
        return;
    }
    
    String backupName = server.arg("backup");
    String backupPath = "/backup_" + backupName + ".json";
    
    if (!SPIFFS.exists(backupPath)) {
        String html = createStyledConfirmationPage(
            "Backup Not Found", 
            "🔍", 
            "Backup '<strong>" + backupName + "</strong>' does not exist.<br><br>The backup file may have been deleted or corrupted.", 
            "📄 Back to Backups", 
            "/inventory/backups",
            "secondary"
        );
        server.send(404, "text/html", html);
        return;
    }
    
    Serial.printf("[BACKUP] Restoring from backup: %s\n", backupPath.c_str());
    
    // Create backup of current state before restoring
    createBackup("before_restore");
    
    // Copy backup to main inventory file
    File backupFile = SPIFFS.open(backupPath, "r");
    File inventoryFile = SPIFFS.open("/inventory.json", "w");
    
    if (!backupFile || !inventoryFile) {
        String html = createStyledConfirmationPage(
            "Restore Failed", 
            "❌", 
            "Failed to open backup files for restore operation.<br><br>The files may be corrupted or the storage system is busy. Please try again.", 
            "📄 Back to Backups", 
            "/inventory/backups",
            "danger"
        );
        server.send(500, "text/html", html);
        return;
    }
    
    // Copy data
    while (backupFile.available()) {
        inventoryFile.write(backupFile.read());
    }
    
    backupFile.close();
    inventoryFile.close();
    
    // Reload inventory
    inventory.clear();
    loadInventoryFromSPIFFS();
    
    Serial.printf("[BACKUP] ✓ Restored from backup: %s\n", backupName.c_str());
    String html = createStyledConfirmationPage(
        "Restore Complete", 
        "🔄", 
        "Successfully restored from backup '<strong>" + backupName + "</strong>'.<br><br>✓ Your inventory has been restored<br>✓ Previous state backed up as 'before_restore'<br>✓ Ready to use!", 
        "📦 View Inventory", 
        "/inventory",
        "success"
    );
    server.send(200, "text/html", html);
}

void handleInventoryDeleteBackup() {
    if (!server.hasArg("backup")) {
        server.send(400, "text/plain", "Missing backup parameter");
        return;
    }
    
    String backupName = server.arg("backup");
    String backupPath = "/backup_" + backupName + ".json";
    
    if (!SPIFFS.exists(backupPath)) {
        String html = createStyledConfirmationPage(
            "Backup Not Found", 
            "🔍", 
            "Backup '<strong>" + backupName + "</strong>' does not exist.<br><br>It may have already been deleted or the name is incorrect.", 
            "📄 Back to Backups", 
            "/inventory/backups",
            "secondary"
        );
        server.send(404, "text/html", html);
        return;
    }
    
    // Prevent deletion of critical backups
    if (backupName == "current" || backupName == "auto_reset" || backupName == "before_restore") {
        String html = createStyledConfirmationPage(
            "Cannot Delete System Backup", 
            "🔒", 
            "System backup '<strong>" + backupName + "</strong>' cannot be deleted for safety.<br><br>Critical backups are protected to prevent accidental data loss.", 
            "📄 Back to Backups", 
            "/inventory/backups",
            "secondary"
        );
        server.send(403, "text/html", html);
        return;
    }
    
    Serial.printf("[BACKUP] Deleting backup: %s\n", backupPath.c_str());
    
    if (SPIFFS.remove(backupPath)) {
        Serial.printf("[BACKUP] ✓ Deleted backup: %s\n", backupName.c_str());
        String html = createStyledConfirmationPage(
            "Backup Deleted Successfully", 
            "🗑️", 
            "Successfully deleted backup '<strong>" + backupName + "</strong>'.<br><br>The backup file has been permanently removed from storage.", 
            "📄 Back to Backups", 
            "/inventory/backups",
            "primary"
        );
        server.send(200, "text/html", html);
    } else {
        Serial.printf("[BACKUP] ✗ Failed to delete backup: %s\n", backupName.c_str());
        String html = createStyledConfirmationPage(
            "Delete Failed", 
            "❌", 
            "Failed to delete backup '<strong>" + backupName + "</strong>'.<br><br>The file may be in use or protected. Please try again.", 
            "📄 Back to Backups", 
            "/inventory/backups",
            "danger"
        );
        server.send(500, "text/html", html);
    }
}

void handleInventoryFactoryReset() {
    Serial.println("[INVENTORY] FACTORY RESET - Wiping ALL data including backups...");
    
    // Remove all inventory and backup files
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        if (fileName.equals("/inventory.json") || fileName.startsWith("/backup_")) {
            SPIFFS.remove(fileName);
            Serial.printf("[FACTORY] Deleted: %s\n", fileName.c_str());
        }
        file = root.openNextFile();
    }
    
    inventory.clear();
    initializeDefaultInventory();
    Serial.println("[INVENTORY] Factory reset complete - all data wiped");
    String html = createStyledConfirmationPage(
        "Factory Reset Complete", 
        "🏭", 
        "<strong>⚠️ ALL data and backups wiped clean.</strong><br><br>✓ Inventory reset to original hardcoded defaults<br>✓ All customizations removed<br>✓ Fresh start ready", 
        "📦 Setup Inventory", 
        "/inventory",
        "success"
    );
    server.send(200, "text/html", html);
}

void initializeDefaultInventory() {
    Serial.println("[INVENTORY] Initializing default inventory structure...");
    inventory.clear();
    
    // Default trailer inventory - you can customize this or load from external source
    DynamicCategory food("Food & Beverages", "🍽️", true, SUBCATEGORY_TRAILER);
    food.consumables.push_back(ConsumableItem("Milk", STATUS_OK, false));  // Trip item
    food.consumables.push_back(ConsumableItem("Bread", STATUS_OK, false)); // Trip item
    food.consumables.push_back(ConsumableItem("Salt", STATUS_OK, true));   // Trailer item
    food.consumables.push_back(ConsumableItem("Pepper", STATUS_OK, true)); // Trailer item
    inventory.push_back(food);
    
    DynamicCategory cleaning("Cleaning Supplies", "🧽", true, SUBCATEGORY_TRAILER);
    cleaning.consumables.push_back(ConsumableItem("Paper Towels", STATUS_OK, true));
    cleaning.consumables.push_back(ConsumableItem("Dish Soap", STATUS_OK, true));
    inventory.push_back(cleaning);
    
    DynamicCategory kitchen("Kitchen Equipment", "🍳", false, SUBCATEGORY_TRAILER);
    kitchen.equipment.push_back(EquipmentItem("Coffee Maker", false, false, false));
    kitchen.equipment.push_back(EquipmentItem("Can Opener", false, false, false));
    inventory.push_back(kitchen);
    
    Serial.printf("[INVENTORY] Created default inventory with %d categories\n", inventory.size());
}

void loadInventoryFromSPIFFS() {
    if (!SPIFFS.exists("/inventory.json")) {
        Serial.println("[INVENTORY] No saved data, initializing defaults");
        initializeDefaultInventory();
        return;
    }

    File file = SPIFFS.open("/inventory.json", "r");
    if (!file) {
        Serial.println("[INVENTORY] Failed to open file, initializing defaults");
        initializeDefaultInventory();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[INVENTORY] JSON parse error: %s\n", error.c_str());
        Serial.println("[INVENTORY] Initializing defaults");
        initializeDefaultInventory();
        return;
    }

    inventory.clear();
    JsonArray categories = doc.as<JsonArray>();
    
    for (JsonObject categoryObj : categories) {
        String catName = categoryObj["name"];
        String catIcon = categoryObj["icon"];
        bool isConsumable = categoryObj["isConsumable"];
        // Load subcategory with proper defaults (no migration needed after fix-tabs)
        Subcategory subcategory = (Subcategory)(categoryObj["subcategory"] | SUBCATEGORY_TRAILER);
        
        DynamicCategory category(catName, catIcon, isConsumable, subcategory);
        JsonArray items = categoryObj["items"];
        
        if (isConsumable) {
            for (JsonObject itemObj : items) {
                String itemName = itemObj["name"];
                uint8_t status = itemObj["status"];
                
                // Load or calculate livesInTrailer field
                bool livesInTrailer;
                if (itemObj["livesInTrailer"].is<bool>()) {
                    livesInTrailer = itemObj["livesInTrailer"];
                } else {
                    // Migration: Apply smart defaults for existing data
                    livesInTrailer = shouldLiveInTrailer(itemName, catName);
                    Serial.printf("[MIGRATION] %s/%s -> livesInTrailer: %s\n", 
                                 catName.c_str(), itemName.c_str(), 
                                 livesInTrailer ? "Yes" : "No");
                }
                
                unsigned long lastUpdated = itemObj["lastUpdated"] | millis();
                
                ConsumableItem item(itemName, (ItemStatus)status, livesInTrailer);
                item.lastUpdated = lastUpdated;
                category.consumables.push_back(item);
            }
        } else {
            for (JsonObject itemObj : items) {
                String itemName = itemObj["name"];
                bool checked = itemObj["checked"];
                bool packed = itemObj["packed"];
                bool taking = itemObj["taking"] | true;
                
                // Load or set livesInTrailer (always true for equipment by default)
                bool livesInTrailer = itemObj["livesInTrailer"] | true;
                unsigned long lastUpdated = itemObj["lastUpdated"] | millis();
                
                EquipmentItem item(itemName, checked, packed, taking);
                item.livesInTrailer = livesInTrailer;
                item.lastUpdated = lastUpdated;
                category.equipment.push_back(item);
            }
        }
        
        inventory.push_back(category);
    }

    Serial.printf("[INVENTORY] Loaded %d categories from SPIFFS\n", inventory.size());
    if (inventory.size() == 0) {
        Serial.println("[INVENTORY] Empty file, initializing defaults");
        initializeDefaultInventory();
    }
    sortAllInventory();  // Ensure alphabetical order after loading
}

void handleInventoryReload() {
    Serial.println("[INVENTORY] Reloading from SPIFFS...");
    inventory.clear();
    loadInventoryFromSPIFFS();
    Serial.printf("[INVENTORY] Reload complete - %d categories loaded\n", inventory.size());
    server.send(200, "text/html", "<html><body><h1>Inventory Reloaded</h1><p>Loaded " + String(inventory.size()) + " categories from saved data.</p><p><a href='/inventory'>Return to Inventory</a></p></body></html>");
}

void handleInventoryFixTabs() {
    Serial.println("[INVENTORY] EMERGENCY FIX: Correcting tab assignments...");
    
    // Force correct subcategory assignments based on known category names
    for (size_t i = 0; i < inventory.size(); i++) {
        if (inventory[i].isConsumable) {
            // All consumables should be SUBCATEGORY_TRAILER (shows in Consumables tab)
            inventory[i].subcategory = SUBCATEGORY_TRAILER;
        } else {
            // Equipment assignments based on actual category names
            String catName = inventory[i].name;
            
            if (catName == "Trailer Systems" || catName == "Kitchen Basics" || 
                catName == "Crockery and Cutlery" || catName == "Tools and Utilities") {
                inventory[i].subcategory = SUBCATEGORY_TRAILER; // These go in Trailer tab
                Serial.printf("[FIX] %s -> TRAILER tab\n", catName.c_str());
            }
            else if (catName == "Personal Items" || catName == "Sleep and Comfort" || 
                     catName == "Safety and Fun" || catName == "Kitchen Extras") {
                inventory[i].subcategory = SUBCATEGORY_ESSENTIALS; // These go in Essentials tab
                Serial.printf("[FIX] %s -> ESSENTIALS tab\n", catName.c_str());
            }
            else {
                inventory[i].subcategory = SUBCATEGORY_OPTIONAL; // Everything else goes in Optional tab
                Serial.printf("[FIX] %s -> OPTIONAL tab\n", catName.c_str());
            }
        }
    }
    
    // Save the corrected assignments
    if (saveInventoryToSPIFFS()) {
        Serial.println("[FIX] Tab assignments corrected and saved!");
        server.send(200, "text/html", 
            "<html><body style='font-family:Arial;padding:20px;background:#000;color:#fff'>"
            "<h1 style='color:#4af'>✅ Tab Assignments Fixed!</h1>"
            "<p>Categories have been reassigned to correct tabs:</p>"
            "<ul>"
            "<li><strong>🍴 Consumables:</strong> All food/supply items</li>"
            "<li><strong>🚚 Trailer:</strong> Trailer Systems, Kitchen Basics, Crockery, Tools</li>"
            "<li><strong>⭐ Essentials:</strong> Personal Items, Sleep/Comfort, Safety, Kitchen Extras</li>"
            "<li><strong>📦 Optional:</strong> Camping gear, tents, furniture, etc.</li>"
            "</ul>"
            "<p><a href='/inventory' style='color:#4af'>Return to Inventory</a></p>"
            "</body></html>");
    } else {
        Serial.println("[FIX] Error saving corrected assignments!");
        server.send(500, "text/plain", "Error saving corrections");
    }
}

void handleInventoryClearAll() {
    if (!server.hasArg("type")) {
        server.send(400, "text/plain", "Missing type parameter");
        return;
    }

    String type = server.arg("type");
    int cleared = 0;
    
    if (type == "essentials") {
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_ESSENTIALS) continue;
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                inventory[i].equipment[j].taking = false;
                inventory[i].equipment[j].checked = false;
                inventory[i].equipment[j].packed = false;
                cleared++;
            }
        }
        Serial.printf("[INVENTORY] Cleared all Essentials - %d items reset\n", cleared);
    } else if (type == "optional") {
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_OPTIONAL) continue;
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                inventory[i].equipment[j].taking = false;
                inventory[i].equipment[j].checked = false;
                inventory[i].equipment[j].packed = false;
                cleared++;
            }
        }
        Serial.printf("[INVENTORY] Cleared all Optional - %d items reset\n", cleared);
    } else if (type == "trailer") {
        for (size_t i = 0; i < inventory.size(); i++) {
            if (inventory[i].isConsumable || inventory[i].subcategory != SUBCATEGORY_TRAILER) continue;
            for (size_t j = 0; j < inventory[i].equipment.size(); j++) {
                inventory[i].equipment[j].checked = false;
                inventory[i].equipment[j].packed = false;
                cleared++;
            }
        }
        Serial.printf("[INVENTORY] Cleared all Trailer - %d items reset\n", cleared);
    } else if (type == "consumables") {
        // Check if request has JSON body (filtered items)
        if (server.method() == HTTP_POST && server.hasArg("plain")) {
            String body = server.arg("plain");
            Serial.printf("[INVENTORY] Clear filtered items body: %s\n", body.c_str());
            
            // Simple JSON parsing for {items:[{cat:0,item:1},{cat:0,item:2}]}
            int itemsStart = body.indexOf("\"items\":[");
            if (itemsStart >= 0) {
                itemsStart += 9; // Skip "items":["
                int itemsEnd = body.indexOf(']', itemsStart);
                if (itemsEnd > itemsStart) {
                    String itemsStr = body.substring(itemsStart, itemsEnd);
                    Serial.printf("[INVENTORY] Items string: %s\n", itemsStr.c_str());
                    
                    // Parse each {cat:X,item:Y} object
                    int pos = 0;
                    while (pos < itemsStr.length()) {
                        int objStart = itemsStr.indexOf('{', pos);
                        if (objStart < 0) break;
                        int objEnd = itemsStr.indexOf('}', objStart);
                        if (objEnd < 0) break;
                        
                        String obj = itemsStr.substring(objStart, objEnd + 1);
                        Serial.printf("[INVENTORY] Processing object: %s\n", obj.c_str());
                        
                        // Extract cat and item numbers
                        int catPos = obj.indexOf("\"cat\":");
                        int itemPos = obj.indexOf("\"item\":");
                        if (catPos >= 0 && itemPos >= 0) {
                            int catStart = catPos + 7; // Skip "cat":" and opening quote
                            int catEnd = obj.indexOf('"', catStart); // Find closing quote
                            
                            int itemStart = itemPos + 8; // Skip "item":" and opening quote
                            int itemEnd = obj.indexOf('"', itemStart); // Find closing quote
                            
                            int catNum = obj.substring(catStart, catEnd).toInt();
                            int itemNum = obj.substring(itemStart, itemEnd).toInt();
                            
                            Serial.printf("[INVENTORY] Parsed cat:%d, item:%d\n", catNum, itemNum);
                            
                            if (catNum < inventory.size() && inventory[catNum].isConsumable && 
                                itemNum < inventory[catNum].consumables.size()) {
                                inventory[catNum].consumables[itemNum].status = STATUS_OUT;
                                cleared++;
                                Serial.printf("[INVENTORY] Set item to OUT: cat:%d, item:%d\n", catNum, itemNum);
                            }
                        }
                        
                        pos = objEnd + 1;
                    }
                }
            }
            Serial.printf("[INVENTORY] Cleared %d filtered consumables to OUT\n", cleared);
        } else {
            // Fallback: clear all consumables (old behavior)
            for (size_t i = 0; i < inventory.size(); i++) {
                if (!inventory[i].isConsumable) continue;
                for (size_t j = 0; j < inventory[i].consumables.size(); j++) {
                    inventory[i].consumables[j].status = STATUS_OUT;
                    cleared++;
                }
            }
            Serial.printf("[INVENTORY] Cleared all Consumables - %d items set to OUT\n", cleared);
        }
    } else {
        server.send(400, "text/plain", "Invalid type parameter");
        return;
    }

    saveInventoryToSPIFFS();
    server.send(200, "text/plain", "OK");
}

void handleNotFound() {
    Serial.printf("[WEB] 404: %s\n", server.uri().c_str());
    server.send(404, "text/plain", "Not Found");
}

// ============ SMART BACKUP SYSTEM ============
void checkWeeklyMonthlyBackups(unsigned long currentDay);
void smartBackupCleanup(unsigned long currentDay);

void checkDailyBackup() {
    static unsigned long lastBackupCheck = 0;
    static unsigned long lastBackupDay = 0;
    
    // Check once per hour (3,600,000 ms)
    if (millis() - lastBackupCheck < 3600000) {
        return;
    }
    lastBackupCheck = millis();
    
    // Get current day (days since boot)
    unsigned long currentDay = millis() / 86400000;
    
    // Check if we need a daily backup (once per day)
    if (currentDay != lastBackupDay) {
        lastBackupDay = currentDay;
        
        // Create timestamped daily backup
        String backupName = "daily_" + String(currentDay);
        createBackup(backupName);
        Serial.printf("[AUTO-BACKUP] ✓ Daily: %s\n", backupName.c_str());
        
        // Check if we need weekly/monthly backups
        checkWeeklyMonthlyBackups(currentDay);
        
        // Clean up old backups using smart retention
        smartBackupCleanup(currentDay);
    }
}

void checkWeeklyMonthlyBackups(unsigned long currentDay) {
    // Create weekly backup every 7 days (simulating Sunday)
    if (currentDay % 7 == 0) {
        String weeklyName = "weekly_" + String(currentDay / 7);
        createBackup(weeklyName);
        Serial.printf("[AUTO-BACKUP] ✓ Weekly: %s\n", weeklyName.c_str());
    }
    
    // Create monthly backup every 30 days (simulating 1st of month)
    if (currentDay % 30 == 0) {
        String monthlyName = "monthly_" + String(currentDay / 30);
        createBackup(monthlyName);
        Serial.printf("[AUTO-BACKUP] ✓ Monthly: %s\n", monthlyName.c_str());
    }
    
    // Create quarterly backup every 90 days
    if (currentDay % 90 == 0) {
        String quarterlyName = "quarterly_" + String(currentDay / 90);
        createBackup(quarterlyName);
        Serial.printf("[AUTO-BACKUP] ✓ Quarterly: %s\n", quarterlyName.c_str());
    }
}

void smartBackupCleanup(unsigned long currentDay) {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    
    while (file) {
        String fileName = file.name();
        bool shouldDelete = false;
        
        // Daily backups: keep last 7 days
        if (fileName.startsWith("/backup_daily_")) {
            unsigned long day = fileName.substring(14, fileName.indexOf(".json")).toInt();
            if (currentDay - day > 7) {
                shouldDelete = true;
                Serial.printf("[CLEANUP] Daily backup expired: %s\n", fileName.c_str());
            }
        }
        
        // Weekly backups: keep last 4 weeks
        else if (fileName.startsWith("/backup_weekly_")) {
            unsigned long week = fileName.substring(15, fileName.indexOf(".json")).toInt();
            if (currentDay / 7 - week > 4) {
                shouldDelete = true;
                Serial.printf("[CLEANUP] Weekly backup expired: %s\n", fileName.c_str());
            }
        }
        
        // Monthly backups: keep last 6 months
        else if (fileName.startsWith("/backup_monthly_")) {
            unsigned long month = fileName.substring(16, fileName.indexOf(".json")).toInt();
            if (currentDay / 30 - month > 6) {
                shouldDelete = true;
                Serial.printf("[CLEANUP] Monthly backup expired: %s\n", fileName.c_str());
            }
        }
        
        // Quarterly backups: keep last 4 quarters
        else if (fileName.startsWith("/backup_quarterly_")) {
            unsigned long quarter = fileName.substring(18, fileName.indexOf(".json")).toInt();
            if (currentDay / 90 - quarter > 4) {
                shouldDelete = true;
                Serial.printf("[CLEANUP] Quarterly backup expired: %s\n", fileName.c_str());
            }
        }
        
        if (shouldDelete) {
            SPIFFS.remove(fileName);
        }
        
        file = root.openNextFile();
    }
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

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("[SPIFFS] Mount failed!");
    } else {
        Serial.println("✓ SPIFFS initialized");
        // Load inventory from SPIFFS
        loadInventoryFromSPIFFS();
    }

    // PWA support endpoints
    server.on("/manifest.json", handleManifest);
    server.on("/sw.js", handleServiceWorker);
    server.on("/icon-192.png", handleIcon192);
    server.on("/icon-512.png", handleIcon512);

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/test", handleTest);
    server.on("/fridge", handleFridge);
    server.on("/fridge/cmd", handleFridgeCommand);
    server.on("/fridge/eco", handleFridgeEco);
    server.on("/fridge/battery", handleFridgeBattery);
    server.on("/fridge/status", handleFridgeStatus);
    server.on("/inventory", handleInventory);
    server.on("/inventory/set", handleInventorySet);
    server.on("/inventory/check", handleInventoryCheck);
    server.on("/inventory/save", handleInventorySave);
    server.on("/inventory/stats", handleInventoryStats);
    server.on("/inventory/shopping", handleInventoryShopping);
    server.on("/inventory/resetall", handleInventoryResetAll);
    server.on("/inventory/restock", handleInventoryRestock);
    server.on("/inventory/download", handleInventoryDownload);
    server.on("/inventory/export-csv", handleInventoryExportCSV);
    server.on("/inventory/import-csv", HTTP_GET, handleInventoryImportCSV);
    server.on("/inventory/import-csv", HTTP_POST, handleInventoryImportCSV);
    server.on("/inventory/add", handleInventoryAddItem);
    server.on("/inventory/add-category", handleInventoryAddCategory);
    server.on("/inventory/remove", handleInventoryRemoveItem);
    server.on("/inventory/rename-item", handleInventoryRenameItem);
    server.on("/inventory/move-item", handleInventoryMoveItem);
    server.on("/inventory/reset", handleInventoryReset);
    server.on("/inventory-reset", handleInventoryReset);  // Alternative URL with dash
    server.on("/inventory/save-as-defaults", handleInventorySaveAsDefaults);
    server.on("/inventory/backups", handleInventoryBackups);
    server.on("/inventory/restore", handleInventoryRestore);
    server.on("/inventory/delete-backup", handleInventoryDeleteBackup);
    server.on("/inventory/factory-reset", handleInventoryFactoryReset);
    server.on("/inventory-factory-reset", handleInventoryFactoryReset);  // Alternative URL with dash
    server.on("/inventory/reload", handleInventoryReload);
    server.on("/inventory/fix-tabs", handleInventoryFixTabs); // Emergency fix for tab assignments
    server.on("/inventory/edit", handleInventoryRenameItem);  // Alias for edit
    server.on("/inventory/edit-consumable", handleInventoryEditConsumable);  // Edit consumable name and trailer status
    server.on("/inventory/clearall", handleInventoryClearAll);
    server.on("/inventory/rename", handleInventoryRenameItem);  // Item rename
    server.on("/inventory/delete", handleInventoryDeleteItem);  // Item delete  
    server.on("/inventory/move", handleInventoryMoveItem);      // Item move
    server.on("/inventory/renamecat", handleCategoryRename);    // Category rename
    server.on("/inventory/deletecat", handleCategoryDelete);    // Category delete
    server.on("/inventory/additem", handleInventoryAddItem);    // Add item
    server.on("/inventory/force-new-structure", []() {
        Serial.println("[DEBUG] Force loading new 12-category structure...");
        inventory.clear();
        initializeDefaultInventory();
        saveInventoryToSPIFFS();
        Serial.printf("[DEBUG] Loaded %d categories, saved to SPIFFS\n", inventory.size());
        server.send(200, "text/plain", "New structure loaded! Go back to /inventory");
    });
    
    // Debug endpoint to check JavaScript and memory
    server.on("/debug/js", []() {
        String js = "";
        js += "function showTab(n){";
        js += "document.querySelectorAll('.content').forEach((c,i)=>c.classList.toggle('active',i==n));";
        js += "document.querySelectorAll('.tab').forEach((t,i)=>t.classList.toggle('active',i==n));";
        js += "if(n==2){refreshShoppingList();}}";
        js += "\\n\\nfunction toggleCat(id){";
        js += "let el=document.getElementById('cat'+id);";
        js += "let hdr=el.previousElementSibling;";
        js += "el.classList.toggle('expanded');hdr.classList.toggle('expanded');}";
        js += "\\n\\nfunction collapseAll(){";
        js += "document.querySelectorAll('.items-container').forEach(el=>{";
        js += "el.classList.remove('expanded');";
        js += "let hdr=el.previousElementSibling;";
        js += "if(hdr)hdr.classList.remove('expanded');});}";
        js += "\\n\\nfunction expandAll(){";
        js += "document.querySelectorAll('.items-container').forEach(el=>{";
        js += "el.classList.add('expanded');";
        js += "let hdr=el.previousElementSibling;";
        js += "if(hdr)hdr.classList.add('expanded');});}";
        
        String response = "Expected JS Functions:\\n" + js + "\\n\\nFree heap: " + String(ESP.getFreeHeap()) + " bytes";
        response += "\\nTotal heap: " + String(ESP.getHeapSize()) + " bytes";
        response += "\\nInventory categories: " + String(inventory.size());
        
        server.send(200, "text/plain", response);
    });
    
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
    processMasterQueue();  // Send queued commands when Victron is ready
    checkDailyBackup();    // Auto-backup once per day
    yield();
}

