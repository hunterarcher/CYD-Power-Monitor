#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Slave ESP32 MAC (Victron device)
uint8_t slaveMAC[] = {0x78, 0x21, 0x84, 0x9C, 0x9B, 0x88};

uint32_t sendCount = 0;
uint32_t receiveCount = 0;

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

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("   ESP-NOW Bidirectional Test - MASTER");
    Serial.println("========================================\n");

    // Init WiFi in STA mode (exactly like RandomNerd tutorial)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // CRITICAL: Disconnect from any AP
    delay(100);

    Serial.printf("My MAC Address: %s\n", WiFi.macAddress().c_str());
    Serial.println("^^ Use this MAC in SLAVE code!\n");

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

    // Add slave as peer (channel 0 = auto like tutorial)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, slaveMAC, 6);
    peerInfo.channel = 0;  // 0 = auto (like RandomNerd tutorial)
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.printf("✓ Added SLAVE as peer: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                     slaveMAC[0], slaveMAC[1], slaveMAC[2], slaveMAC[3], slaveMAC[4], slaveMAC[5]);
    } else {
        Serial.println("✗ Failed to add SLAVE as peer!\n");
    }

    Serial.println("========================================");
    Serial.println("Press 'S' + ENTER to send a test message");
    Serial.println("========================================\n");
}

void loop() {
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
    delay(10);
}
