#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>

// Slave ESP32 MAC (Victron device)
uint8_t slaveMAC[] = {0x78, 0x21, 0x84, 0x9C, 0x9B, 0x88};

uint32_t sendCount = 0;
uint32_t receiveCount = 0;

// WiFi AP credentials
const char* AP_SSID = "PowerMonitor";
const char* AP_PASSWORD = "12345678";

// Web server
WebServer server(80);

// Send callback
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.printf("[SEND] Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "✓ Success" : "✗ Failed");
}

// Receive callback
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
    Serial.printf("\n[RECEIVE] Got message #%d from: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 ++receiveCount,
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
    Serial.printf("          Data: %s\n\n", (char*)data);
}

// Web server handlers
void handleRoot() {
    Serial.println("[WEB] Root page requested");
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<style>body{font-family:Arial;margin:20px;} button{padding:15px 30px;font-size:18px;margin:10px;}</style>";
    html += "</head><body>";
    html += "<h1>ESP-NOW Test Dashboard</h1>";
    html += "<p>Messages Sent: " + String(sendCount) + "</p>";
    html += "<p>Messages Received: " + String(receiveCount) + "</p>";
    html += "<button onclick=\"location.href='/send'\">Send Test Message</button>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleSend() {
    Serial.println("[WEB] Send command received");
    sendCount++;
    char message[32];
    sprintf(message, "MASTER msg #%d", sendCount);

    Serial.printf("[WEB] Sending message #%d...\n", sendCount);
    esp_now_send(slaveMAC, (uint8_t*)message, strlen(message) + 1);

    server.sendHeader("Location", "/");
    server.send(303);
}

void handleNotFound() {
    Serial.printf("[WEB] 404: %s\n", server.uri().c_str());
    server.send(404, "text/plain", "Not Found");
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("   ESP-NOW + Web Server Test - MASTER");
    Serial.println("========================================\n");

    // CRITICAL: Use AP_STA mode for ESP-NOW + Web Server
    WiFi.mode(WIFI_AP_STA);

    // Start AP for web access
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    delay(100);

    Serial.printf("My STA MAC Address: %s\n", WiFi.macAddress().c_str());
    Serial.println("^^ Use this MAC in SLAVE code!\n");
    Serial.printf("AP Started: %s\n", AP_SSID);
    Serial.printf("AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("✗ ESP-NOW init failed!");
        return;
    }
    Serial.println("✓ ESP-NOW initialized");

    // Register callbacks
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);
    Serial.println("✓ Callbacks registered");

    // Add slave as peer (channel 1 = default AP channel)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, slaveMAC, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.printf("✓ Added SLAVE as peer: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                     slaveMAC[0], slaveMAC[1], slaveMAC[2], slaveMAC[3], slaveMAC[4], slaveMAC[5]);
    } else {
        Serial.println("✗ Failed to add SLAVE as peer!\n");
    }

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/send", handleSend);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("✓ Web server started\n");

    Serial.println("========================================");
    Serial.println("Connect to WiFi: PowerMonitor");
    Serial.println("Then browse to: http://192.168.4.1");
    Serial.println("Or press 'S' + ENTER in serial");
    Serial.println("========================================\n");
}

void loop() {
    server.handleClient();

    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'S' || c == 's') {
            sendCount++;
            char message[32];
            sprintf(message, "MASTER msg #%d", sendCount);

            Serial.printf("[SEND] Sending message #%d...\n", sendCount);
            esp_now_send(slaveMAC, (uint8_t*)message, strlen(message) + 1);
        }
    }
    yield();  // Allow WiFi/web tasks to run instead of delay
}
