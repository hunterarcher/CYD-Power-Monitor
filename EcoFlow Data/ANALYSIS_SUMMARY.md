# EcoFlow Delta Max 2 BLE Protocol Analysis

**Date:** October 3, 2025
**Device:** EcoFlow Delta Max 2 (MAC: 74:4D:BD:CE:A2:49)
**Goal:** Reverse-engineer BLE protocol for full data access and control

---

## Captures Analyzed

1. **btsnoop_hci_EF_AApp.log** - Official EcoFlow app traffic (2174 packets)
2. **btsnoop_hci_nRF.log** - nRF Connect exploration (1848 packets)

---

## BLE Service Discovery (from nRF Connect log)

### Services & Characteristics:
```
Service: 00000001-0000-1000-8000-00805f9b34fb
├── Characteristic (Write):  00000002-0000-1000-8000-00805f9b34fb [W]
└── Characteristic (Notify): 00000003-0000-1000-8000-00805f9b34fb [N]
    └── CCCD: 0x2902
```

**Key Observations:**
- Simple UUID structure (similar to fridge)
- Write characteristic for commands
- Notify characteristic for data/responses
- Connection timeout ~9 seconds when no interaction (nRF log shows disconnect at status 19)

---

## Protocol Analysis Findings

### Expected vs Actual:
**Research indicated:** EcoFlow V2 protocol uses 0x5A 0x5A header, protobuf payloads, encrypted session
**What we found:** NO 0x5A 0x5A headers in either capture

### Packet Structure:
Both captures show L2CAP payloads with consistent patterns:

**Common payload start bytes:**
```
00 19 FE 07 01 00 ...  (appears frequently)
00 19 17 06 01 00 ...  (data frames?)
00 19 20 04 01 00 ...  (command frames?)
00 19 10 04 01 00 ...  (response frames?)
00 19 E7 04 01 00 ...  (status frames?)
```

**Byte breakdown hypothesis:**
- `00 19` - Could be frame type or length prefix
- `FE 07` / `17 06` / `20 04` - Message type indicators
- `01 00` - Might be sequence or version

---

## L2CAP Channel Distribution

**EcoFlow App Log (CIDs):**
```
0x01FF-0x45FF: ~150 different dynamic CIDs
Pattern: Each "session" or message flow uses unique CID
```

This indicates **LE Credit Based Flow Control (LECBFC)** mode, NOT standard ATT protocol!

**Important Discovery:**
The EcoFlow app is NOT using standard GATT ATT protocol (CID 0x0004). Instead, it's using LE Credit-Based Channels which are proprietary L2CAP connections. This explains:
1. Why we don't see ATT Write/Notify opcodes
2. Why the packet structure is different from expected
3. Why simple GATT tools might not work

---

## Data from nRF Connect Log

**Connection sequence:**
1. Connect → Services discovered
2. Enable notifications on 00000003 characteristic
3. Device disconnects after ~9 seconds (likely timeout - needs keep-alive)

**No data exchange observed** - nRF Connect doesn't send any commands, just enables notifications

---

## Comparison: EcoFlow App vs nRF

| Aspect | EcoFlow App | nRF Connect |
|--------|-------------|-------------|
| Packets | 2174 | 1848 |
| L2CAP Mode | LE Credit-Based | LE Credit-Based |
| CID Range | 0x01FF-0x45FF | 0x08FF-0x11FF |
| Data Frames | Many (00 19 17 06...) | Few (00 19 17 06...) |
| Commands | Many (00 19 20 04...) | None |
| Connection Duration | ~164 seconds | Short (discovery only) |

---

## Key Insights

### 1. **NOT Standard GATT**
The Delta Max 2 uses LE Credit-Based Channels, which is a more complex L2CAP mode than standard GATT. This requires:
- L2CAP connection establishment handshake
- Credit-based flow control
- Custom framing protocol

### 2. **Proprietary Framing**
The `00 19` prefix and message type bytes suggest EcoFlow has a custom protocol layer on top of L2CAP, different from the documented 0x5A 0x5A V2 protocol.

### 3. **Delta Max 2 May Be Different**
The research repos focused on Delta 2, Delta Pro Ultra, and SHP2. The Delta Max 2 might:
- Use an older protocol version
- Have different encryption (or none?)
- Use simpler framing than protobuf

### 4. **No Obvious Encryption**
Looking at the payloads, there's no clear encryption handshake or key exchange visible. The data might be:
- Unencrypted
- Using a pre-shared key
- Encrypted with device-specific keys

---

## Available Data from App Screenshots

From the app images, we know the Delta Max 2 provides:
- Battery SOC % (84%)
- Time remaining (99h59m)
- Battery temperature (21°C)
- Input power (0W) - AC/Solar/Car
- Output power (1W) - Total and per-output
- AC/DC/USB output toggles
- X-Boost toggle
- Charge speed settings
- Timeout settings
- Car input current settings

All of this data must be in these L2CAP packets - we just need to decode the framing.

---

## Next Steps

### Option 1: Deep Wireshark Analysis
Open the btsnoop files in Wireshark GUI and:
1. Follow the L2CAP stream
2. Identify the connection establishment sequence
3. Map message types to app actions (using timestamps)
4. Correlate toggle actions with specific command packets

### Option 2: ESP32 Direct Connection Test
Since we have the service/characteristic UUIDs:
1. Connect ESP32 to EcoFlow using NimBLE
2. Enable notifications on characteristic 0x0003
3. Capture raw notification data
4. Try writing simple commands to 0x0002
5. See what responses we get

### Option 3: L2CAP Packet Correlation
1. Look at EcoFlow app packet timestamps
2. Cross-reference with app screenshots (12:02-12:03 timeframe)
3. Identify packets sent when toggling AC/DC/USB
4. Reverse-engineer command structure

### Option 4: Consult EcoFlow Community
The Home Assistant integration is in alpha but might have insights:
- https://community.home-assistant.io/t/ecoflow-delta-pro-ha-integration-via-ble
- GitHub repos for EcoFlow BLE (might have Delta Max 2 notes)

---

## Immediate Action Plan

**RECOMMENDED: ESP32 Direct Connection Test**

Why this first:
1. Fastest way to get actual data
2. Will confirm if we need complex L2CAP or if basic GATT works
3. Can compare ESP32 captures with app captures
4. If notifications work, we might get unencrypted data

**Steps:**
1. Write ESP32 sketch to connect to Delta Max 2
2. Use UUIDs: Service `00000001...`, Write `00000002...`, Notify `00000003...`
3. Enable notifications and capture raw bytes
4. Try simple write commands (even random data) to see responses
5. Document what we learn

This will tell us definitively:
- Does standard BLE connection work?
- Is the data encrypted?
- Do we need the L2CAP complexity?
- What does the raw notification data look like?

---

## Questions to Answer

1. ❓ Does Delta Max 2 accept standard GATT connections or require L2CAP?
2. ❓ Is there a keep-alive requirement? (nRF disconnected after 9 sec)
3. ❓ What triggers the device to send notifications?
4. ❓ Is any encryption/authentication needed?
5. ❓ Can we get useful data without the 0x5A 0x5A protocol?

---

## Files Generated

- `parse_ecoflow_ble.py` - ATT protocol parser (found no ATT packets)
- `simple_parse.py` - L2CAP payload dumper (revealed LE Credit-Based mode)
- `app_analysis.txt` - CID distribution analysis
- `ANALYSIS_SUMMARY.md` - This document
