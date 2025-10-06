# EcoFlow Delta Max 2 - BLE Authentication Implementation Plan

**Status:** Awaiting EcoFlow Developer Profile approval for userId
**Date:** October 3, 2025

---

## Summary of Findings

### ✅ BLE Authentication IS Possible!

Based on analysis of:
- Bluetooth HCI snoop logs (btsnoop_hci.log)
- Rabit's reverse engineering work: https://github.com/rabits/ef-ble-reverse
- Home Assistant community thread: https://community.home-assistant.io/t/ecoflow-ble-unofficial/774794

**Key Discovery:** The app uses standard GATT with EcoFlow Protocol V2 (0x5A 0x5A header) + AES-CBC encryption

---

## Protocol Details

### BLE Service/Characteristics
```
Service UUID:  00000001-0000-1000-8000-00805f9b34fb
Write Char:    00000002-0000-1000-8000-00805f9b34fb (Handle 0x000C)
Notify Char:   00000003-0000-1000-8000-00805f9b34fb
```

### Packet Structure
```
[0x5A] [0x5A] [Type] [Encrypted Payload...]

Example from snoop:
5A 5A 00 01 2C 00 01 00 26 13 CF CB F4 86 B4 F7 8E F7 8C 9D...
│  │  │  │  │  │  └─ Encrypted data starts here
│  │  │  │  └──────── Length fields
│  │  └─ └─────────── Message type
└──└─────────────────── Protocol header (EcoFlow V2)
```

### Encryption Method

**Algorithm:** AES-CBC
**Key Exchange:** ECDH with SECP160r1 elliptic curve
**IV Generation:** MD5 hash of shared secret

---

## Authentication Flow

### 1. Prerequisites
- **userId** - Obtained from EcoFlow web portal (awaiting developer account)
- **Device MAC** - `74:4D:BD:CE:A2:49` (we have this)
- **Device Serial** - Extractable from BLE advertising beacon

### 2. Login Key Generation
Using Rabit's `login_key_gen.py`:
```python
# Needs: userId, possibly serial number
# Generates: login_key.bin (base64 encoded binary)
```

### 3. ECDH Key Exchange
```python
# 1. Generate ECDSA private/public key pair (SECP160r1)
private_key = ecdsa.SigningKey.generate(curve=ecdsa.SECP160r1)
public_key = private_key.get_verifying_key()

# 2. Send our public key to device
# 3. Receive device's public key

# 4. Generate shared secret
shared_key = ecdsa.ECDH(ecdsa.SECP160r1, private_key, dev_pub_key).generate_sharedsecret_bytes()

# 5. Create IV from shared key
iv = hashlib.md5(shared_key).digest()
```

### 4. Session Key Generation
```python
# Combine: login_key + device_seed + random_bytes
data = login_key + seed + sRand
session_key = hashlib.md5(data).digest()
```

### 5. Encryption/Decryption
```python
# Encrypt
aes_session = AES.new(session_key, AES.MODE_CBC, iv)
encrypted = aes_session.encrypt(pad(payload, AES.block_size))

# Decrypt
aes_session = AES.new(session_key, AES.MODE_CBC, iv)
decrypted = unpad(aes_session.decrypt(encrypted_payload), AES.block_size)
```

---

## Observed Session Sequence (from btsnoop)

```
Time      | Handle | Data                                           | Notes
----------|--------|------------------------------------------------|------------------
5430.96s  | 0x000C | 5A 5A 00 01 2C 00 01 00 26 13 CF CB...        | Initial handshake
5431.09s  | 0x000C | 5A 5A 00 01 03 00 02 86 C2                     | Short message
5431.15s  | 0x000C | 5A 5A 10 01 22 00 4F E7 C0 0C 32 AC...        | Encrypted command
5431.27s  | 0x000C | 5A 5A 10 01 42 00 58 9C B0 CE 98 2F...        | Encrypted command
5431.33s  | 0x000C | 5A 5A 10 01 32 00 CC EB C6 23 A8 FA...        | Encrypted command
...       |        |                                                |
5511.81s  | 0x000C | 5A 5A 10 01 22 00 46 21 DF FF BC E9...        | Keep-alive (20s intervals)
5516.99s  | 0x000C | 5A 5A 10 01 22 00 F9 23 65 19 B1 D8...        | AC toggle command?
5521.97s  | 0x000C | 5A 5A 10 01 22 00 F9 23 65 19 B1 D8...        | Confirmation?
```

**Pattern:** Regular encrypted messages every ~20 seconds after initial handshake

---

## Implementation Requirements for ESP32

### Libraries Needed
1. **ECDSA/ECDH** - For key exchange
   - Option A: Use `uECC` library (micro-ecc for embedded)
   - Option B: mbedTLS (included in ESP32 framework)

2. **AES-CBC** - For encryption/decryption
   - mbedTLS (already in ESP-IDF)
   - Or Arduino Crypto library

3. **MD5** - For IV/key generation
   - ESP32 built-in hardware MD5

4. **Protobuf** - For message parsing after decryption
   - nanopb (lightweight protobuf for embedded)

### Code Structure
```
EcoFlow_ESP32/
├── src/
│   ├── main.cpp              # Main connection logic
│   ├── ecoflow_crypto.cpp    # ECDH + AES encryption
│   └── ecoflow_proto.cpp     # Protobuf message handling
├── include/
│   ├── ecoflow_crypto.h
│   └── ecoflow_proto.h
└── lib/
    ├── uECC/                 # ECDH library
    └── nanopb/               # Protobuf library
```

---

## Step-by-Step Implementation Plan

### Phase 1: Get Credentials (WAITING)
- [ ] EcoFlow developer account approved
- [ ] Extract userId from web portal (F12 → Network → api-a.ecoflow.com → Response)
- [ ] Generate login_key.bin using Rabit's script

### Phase 2: Port Crypto to ESP32
- [ ] Add mbedTLS/uECC to platformio.ini
- [ ] Implement ECDH key exchange
- [ ] Implement AES-CBC encrypt/decrypt
- [ ] Test with known vectors from Rabit's code

### Phase 3: BLE Handshake
- [ ] Connect to EcoFlow via BLE
- [ ] Send public key
- [ ] Receive device public key
- [ ] Generate session key
- [ ] Verify handshake works (compare with snoop log)

### Phase 4: Message Protocol
- [ ] Send encrypted keep-alive messages
- [ ] Receive and decrypt notifications
- [ ] Parse protobuf messages
- [ ] Map data fields (battery %, power, temp, etc.)

### Phase 5: Commands
- [ ] Identify command structure from Rabit's code
- [ ] Implement AC/DC/USB toggle commands
- [ ] Test control functions

**Estimated Time:** 1-2 weeks (after getting credentials)

---

## Reference Links

### Repositories
- **Rabit's BLE Reverse Engineering:** https://github.com/rabits/ef-ble-reverse
- **Rabit's HA Integration:** https://github.com/rabits/ha-ef-ble
- **nielsole's Earlier Work:** https://github.com/nielsole/ecoflow-bt-reverse-engineering

### Documentation
- **HA Community Thread:** https://community.home-assistant.io/t/ecoflow-ble-unofficial/774794
- **ESP32 mbedTLS Docs:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mbedtls.html
- **uECC Library:** https://github.com/kmackay/micro-ecc

---

## Alternative Approaches

### If BLE is Too Complex:

1. **WiFi Local API** (currently testing)
   - Connect EcoFlow to ESP32 AP
   - Sniff HTTP traffic
   - Might be simpler than BLE

2. **Cloud API** (fallback)
   - Uses MQTT to mqtt.ecoflow.com
   - Requires internet
   - Not truly local

3. **Beacon Scanning Only** (current working solution)
   - Only gets battery %
   - No control capabilities
   - Simple and reliable

---

## Current Status

✅ **Completed:**
- BLE service/characteristic identification
- Protocol structure analysis (0x5A 0x5A header confirmed)
- Encryption method identified (AES-CBC + ECDH)
- Authentication flow documented

⏸️ **Waiting On:**
- EcoFlow Developer Profile approval
- userId extraction from web portal

🔄 **In Progress:**
- WiFi sniffer development (parallel approach)
- Testing WiFi local API viability

---

## Next Steps

**Immediate:**
1. Build WiFi sniffer to test local API
2. Connect EcoFlow to Master ESP32 AP
3. Capture HTTP traffic (if any)

**After Developer Approval:**
1. Extract userId from EcoFlow web portal
2. Generate login_key.bin
3. Begin ESP32 crypto implementation
4. Port authentication from Rabit's Python code

---

## Notes

- Delta Max 2 confirmed to use same Protocol V2 as Delta Pro Ultra
- Encryption is necessary - no plaintext communication observed
- Standard GATT works (don't need L2CAP CoC complexity)
- Keep-alive messages sent every ~20 seconds
- AC toggle command observed at 5516s in capture (encrypted)

**Last Updated:** October 3, 2025
