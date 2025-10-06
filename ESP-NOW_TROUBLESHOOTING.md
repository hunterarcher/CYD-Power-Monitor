# ESP-NOW Troubleshooting Guide

This document details the ESP-NOW issues encountered and how they were resolved.

---

## Problem: ESP-NOW Commands Not Reaching Victron ESP32

### Symptoms
- Master ESP32 showed "Send status: ✓ Success"
- Victron ESP32 never received commands
- No errors reported on either side
- Data flowing Victron → Master worked fine (unidirectional)

### Root Cause Analysis

#### Issue 1: Wrong MAC Address Target
**What happened:**
- Master was sending commands to Victron's **AP MAC** address
- ESP-NOW in WIFI_AP_STA mode uses **STA MAC** for receiving

**Investigation:**
```cpp
// Victron printed both MACs:
Victron ESP32 STA MAC: 78:21:84:9C:9B:88  ← Use this for receiving!
Victron ESP32 AP MAC:  78:21:84:9C:9B:89  ← Wrong!
```

**Solution:**
```cpp
// Master_ESP32/src/main.cpp
// Changed from AP MAC to STA MAC
uint8_t victronMAC[] = {0x78, 0x21, 0x84, 0x9C, 0x9B, 0x88};
//                                                      ^^ Changed from 89 to 88
```

**Lesson:** In WIFI_AP_STA mode, devices receive ESP-NOW on their STA MAC, not AP MAC.

---

#### Issue 2: WiFi Channel Mismatch
**What happened:**
- Master's AP was on channel 1 (default)
- Victron's STA was on channel 0 (not connected to any WiFi)
- ESP-NOW requires both devices on same channel

**Investigation:**
```cpp
// Victron setup:
WiFi.mode(WIFI_AP_STA);  // But STA not connected = channel 0
```

**Solution - Option 1: Force Victron STA to Channel 1**
```cpp
// Victron_ESP32/src/main.cpp
WiFi.mode(WIFI_AP_STA);
esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // Force channel 1
```

**Solution - Option 2: Set Channel When Adding Peer**
```cpp
// Master_ESP32/src/main.cpp
esp_now_peer_info_t peerInfo = {};
memcpy(peerInfo.peer_addr, victronMAC, 6);
peerInfo.channel = 1;  // Explicitly set peer channel
peerInfo.encrypt = false;
esp_now_add_peer(&peerInfo);
```

**Lesson:** ESP-NOW peers must be on the same WiFi channel. Use `esp_wifi_set_channel()` or set peer channel explicitly.

---

#### Issue 3: Peer Not Added on Victron Side
**What happened:**
- Master could send to Victron (after MAC/channel fix)
- Victron couldn't send ACKs back to Master
- Victron had no peer registered

**Solution:**
```cpp
// Victron_ESP32/src/main.cpp - Added in setup()
esp_now_peer_info_t peerInfo = {};
memcpy(peerInfo.peer_addr, masterMAC, 6);
peerInfo.channel = 1;  // Master's AP channel
peerInfo.encrypt = false;

if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("✓ Added Master ESP32 as peer");
}
```

**Lesson:** Bidirectional ESP-NOW requires both devices to add each other as peers.

---

## Complete ESP-NOW Setup Checklist

### Master ESP32 (WiFi AP + Web Server)
```cpp
void setup() {
    // 1. Set WiFi mode FIRST
    WiFi.mode(WIFI_AP_STA);
    delay(100);

    // 2. Start Access Point on channel 1
    WiFi.softAP(AP_SSID, AP_PASSWORD, 1, 0, 4);
    delay(500);

    // 3. Print MAC addresses for reference
    Serial.printf("Master STA MAC: %s\n", WiFi.macAddress().c_str());
    // Victron needs this MAC ^^

    // 4. Initialize ESP-NOW
    esp_now_init();
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceive);

    // 5. Add Victron as peer (use Victron's STA MAC!)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, victronMAC, 6);
    peerInfo.channel = 1;  // Same as our AP
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}
```

### Victron ESP32 (Sensor Hub)
```cpp
void setup() {
    // 1. Set WiFi mode FIRST
    WiFi.mode(WIFI_AP_STA);
    delay(100);

    // 2. Force channel 1 to match Master's AP
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    // 3. Print MAC addresses
    uint8_t sta_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, sta_mac);
    Serial.printf("Victron STA MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 sta_mac[0], sta_mac[1], sta_mac[2],
                 sta_mac[3], sta_mac[4], sta_mac[5]);
    // Master needs this MAC ^^

    // 4. Initialize ESP-NOW
    esp_now_init();
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    // 5. Add Master as peer (use Master's STA MAC!)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterMAC, 6);
    peerInfo.channel = 1;  // Master's AP channel
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}
```

---

## Testing ESP-NOW Connection

### Test 1: Unidirectional (Victron → Master)
```cpp
// On Victron: Send test packet
VictronPacket packet;
packet.packetId = 123;
esp_now_send(masterMAC, (uint8_t*)&packet, sizeof(packet));

// On Master: Should see in callback
void onDataReceive(const esp_now_recv_info *recv_info,
                   const uint8_t *data, int len) {
    Serial.printf("Received packet ID: %d\n",
                 ((VictronPacket*)data)->packetId);
}
```

### Test 2: Bidirectional (Master → Victron → ACK)
```cpp
// On Master: Send command
ControlCommand cmd;
cmd.commandId = 1;
cmd.device = 1;
esp_now_send(victronMAC, (uint8_t*)&cmd, sizeof(cmd));

// On Victron: Receive and ACK
void onDataRecv(const esp_now_recv_info *recv_info,
                const uint8_t *data, int len) {
    ControlCommand* cmd = (ControlCommand*)data;

    // Send ACK back
    CommandAck ack;
    ack.commandId = cmd->commandId;
    ack.received = true;
    esp_now_send(masterMAC, (uint8_t*)&ack, sizeof(ack));
}
```

---

## Common ESP-NOW Errors

### "ESP-NOW: Peer not found"
**Cause:** Peer MAC address not registered
**Fix:** Call `esp_now_add_peer()` with correct peer info

### "Send status: Failed"
**Causes:**
1. Wrong MAC address
2. Peer not on same channel
3. Peer hasn't added this device
4. Peer out of range

**Debug:**
```cpp
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.printf("Send to %02X:%02X:%02X:%02X:%02X:%02X: %s\n",
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5],
                 status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILED");
}
```

### "Receive callback never fires"
**Causes:**
1. Wrong WiFi channel
2. STA MAC vs AP MAC confusion
3. Callback not registered

**Debug:**
```cpp
// Print channel info
uint8_t primary;
wifi_second_chan_t second;
esp_wifi_get_channel(&primary, &second);
Serial.printf("WiFi Channel: %d\n", primary);
```

---

## ESP-NOW Packet Structure

### VictronPacket (Victron → Master)
```cpp
struct VictronPacket {
    BMVData bmv;              // 44 bytes
    MPPTData mppt;            // 32 bytes
    IP22Data ip22;            // 32 bytes
    EcoFlowData ecoflow;      // 64 bytes
    FridgeData fridge;        // 32 bytes
    uint32_t packetId;        // 4 bytes
    unsigned long senderTime; // 4 bytes
    bool readyForCommand;     // 1 byte
};  // Total: ~213 bytes (ESP-NOW max: 250 bytes)
```

### ControlCommand (Master → Victron)
```cpp
struct ControlCommand {
    uint32_t commandId;       // 4 bytes
    uint8_t device;           // 1 byte
    uint8_t command;          // 1 byte
    int16_t value1;           // 2 bytes
    int16_t value2;           // 2 bytes
    unsigned long timestamp;  // 4 bytes
};  // Total: 14 bytes
```

### CommandAck (Victron → Master)
```cpp
struct CommandAck {
    uint32_t commandId;       // 4 bytes
    bool received;            // 1 byte
    bool executed;            // 1 byte
    uint8_t errorCode;        // 1 byte
    unsigned long timestamp;  // 4 bytes
};  // Total: 11 bytes
```

---

## Performance Considerations

### Packet Size Limits
- **ESP-NOW max:** 250 bytes per packet
- **Current usage:** 213 bytes (VictronPacket) - Safe ✓
- **Headroom:** 37 bytes for future expansion

### Send Frequency
- **Victron → Master:** Every 5 seconds (data packets)
- **Master → Victron:** On-demand (commands)
- **ACKs:** Immediate response
- **No congestion observed**

### Range Testing
- **Same room:** 100% success
- **Through walls:** 100% success (tested up to 2 walls)
- **Outdoor:** Not tested
- **RSSI:** -40 to -60 dBm typical

---

## Debugging Tools

### Serial Monitor Output
```cpp
// Enable verbose ESP-NOW logging
#define ESP_NOW_DEBUG 1

// In callbacks, print detailed info
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.printf("[%lu] ESP-NOW Send: ", millis());
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X%s", mac_addr[i], i < 5 ? ":" : "");
    }
    Serial.printf(" - %s\n", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}
```

### WiFi Analyzer
Use WiFi analyzer app to:
- Verify AP is on channel 1
- Check for interference
- Confirm signal strength

---

## Best Practices

1. **Always use WIFI_AP_STA mode** for bidirectional ESP-NOW
2. **Set WiFi channel explicitly** - don't rely on defaults
3. **Use STA MAC for receiving** - AP MAC is for WiFi clients
4. **Add peers on both sides** - bidirectional requires mutual registration
5. **Keep packets under 200 bytes** - leaves margin for overhead
6. **Add error handling** - check return codes from esp_now_* functions
7. **Print MAC addresses on boot** - makes peer setup easier
8. **Test unidirectional first** - easier to debug
9. **Use unique packet IDs** - helps track lost packets
10. **Implement ACKs for critical commands** - confirms delivery

---

## Reference Links

- ESP-NOW API Reference: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
- ESP32 WiFi Modes: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html
- MAC Address Types: https://github.com/espressif/arduino-esp32/issues/3401

---

## Conclusion

ESP-NOW is powerful but requires careful setup:
- ✅ Correct MAC addresses (STA, not AP)
- ✅ Matching WiFi channels
- ✅ Mutual peer registration
- ✅ Proper packet structure

Once configured correctly, it's extremely reliable for local device communication with minimal latency and no router dependency.
