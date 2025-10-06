#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Master ESP32 MAC (CYD device)
uint8_t masterMAC[] = {0x7C, 0x87, 0xCE, 0x31, 0xFE, 0x50};

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
    Serial.println("   ESP-NOW Bidirectional Test - SLAVE");
    Serial.println("========================================\n");

    // Init WiFi in STA mode (exactly like RandomNerd tutorial)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // CRITICAL: Disconnect from any AP
    delay(100);

    Serial.printf("My MAC Address: %s\n", WiFi.macAddress().c_str());
    Serial.println("^^ Use this MAC in MASTER code!\n");

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

    // Set WiFi channel to 1 to match Master AP
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    Serial.println("✓ WiFi channel set to 1");

    // Add master as peer (channel 1 to match AP)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterMAC, 6);
    peerInfo.channel = 1;  // Channel 1 to match Master AP
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.printf("✓ Added MASTER as peer: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                     masterMAC[0], masterMAC[1], masterMAC[2], masterMAC[3], masterMAC[4], masterMAC[5]);
    } else {
        Serial.println("✗ Failed to add MASTER as peer!\n");
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
            sprintf(message, "SLAVE msg #%d", sendCount);

            Serial.printf("[SEND] Sending message #%d...\n", sendCount);
            esp_now_send(masterMAC, (uint8_t*)message, strlen(message) + 1);
        }
    }
    delay(10);
}
