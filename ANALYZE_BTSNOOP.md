# How to Analyze btsnoop_hci.log with Wireshark

## Quick Analysis Steps

### 1. Open in Wireshark
1. Download Wireshark: https://www.wireshark.org/download.html
2. Open Wireshark
3. File → Open → Select `btsnoop_hci.log`

### 2. Filter for Fridge Traffic
In the filter bar at top, enter:
```
bluetooth.addr == ff:ff:11:c6:29:50
```
Click Apply

This shows ONLY traffic to/from your fridge.

---

## What We Need to Find

### A. Connection Parameters (From App Connection)

**Filter:**
```
bluetooth.addr == ff:ff:11:c6:29:50 && btl2cap
```

**Look for:** `LE Connection Complete` or `Connection Parameter Update`

**Copy these values:**
- Connection Interval: _______
- Slave Latency: _______
- Supervision Timeout: _______

---

### B. Timing Between Operations

Find these events in order and note the **TIME** column:

1. **Connection established**
   - Look for: `Connect Complete` or status = 0x00
   - Time: __:__:__.______

2. **MTU Exchange** (if present)
   - Look for: `Exchange MTU Request`
   - Time: __:__:__.______
   - MTU value: _______

3. **Service Discovery**
   - Look for: `Read By Group Type Request`
   - Time: __:__:__.______

4. **CCCD Write** (Enable notifications)
   - Look for: `Write Request` to handle 0x0004 or descriptor 0x2902
   - Time: __:__:__.______
   - Value written: (should be `01 00`)

5. **First Data After CCCD**
   - Look for: First packet AFTER CCCD write
   - Time: __:__:__.______

**Calculate delays:**
- Connection → MTU: _______ ms
- MTU → Service Discovery: _______ ms
- Service Discovery → CCCD: _______ ms
- CCCD → First Data: _______ ms

---

### C. What App Writes Immediately After Connection

**Filter:**
```
bluetooth.addr == ff:ff:11:c6:29:50 && btatt.opcode == 0x52
```

This shows all WRITE operations.

**For EACH write, copy:**
1. Time: __:__:__.______
2. Handle: 0x____
3. Value (hex): __ __ __ __ __ __
4. How long after connection: _______ ms

**Specifically look for:**
- Any writes BEFORE CCCD write
- The first write after connection
- Keep-alive pattern: `FE FE 03 01 02 00`

---

### D. Connection Sequence Analysis

**Filter for successful app connection:**
```
bluetooth.addr == ff:ff:11:c6:29:50 && (btatt || btl2cap.cid == 0x0004)
```

**Export the sequence:**
1. In Wireshark: File → Export Packet Dissections → As Plain Text
2. Save as `app_connection_sequence.txt`
3. Copy the first 50 packets here

---

### E. Compare nRF vs App

**Find the nRF connection attempt** (later in the log):
- Look for multiple connection attempts (Error 133)
- Find the one that succeeds
- Note if nRF does anything DIFFERENT than app

**Compare:**
- Does nRF send different connection parameters?
- Does nRF skip any steps?
- Does nRF send writes in different order?

---

## Quick Wireshark Tips

### Show Only Fridge Communication
```
bluetooth.addr == ff:ff:11:c6:29:50
```

### Show Only Writes
```
bluetooth.addr == ff:ff:11:c6:29:50 && btatt.opcode == 0x52
```

### Show Only Notifications/Indications
```
bluetooth.addr == ff:ff:11:c6:29:50 && (btatt.opcode == 0x1b || btatt.opcode == 0x1d)
```

### Show Connection Events
```
bluetooth.addr == ff:ff:11:c6:29:50 && bthci_evt
```

### Show ATT Errors
```
bluetooth.addr == ff:ff:11:c6:29:50 && btatt.error_code
```

---

## Alternative: Use tshark (Command Line)

If you prefer command line:

### Extract all fridge packets:
```bash
tshark -r btsnoop_hci.log -Y "bluetooth.addr == ff:ff:11:c6:29:50" -V > fridge_packets.txt
```

### Extract just write operations:
```bash
tshark -r btsnoop_hci.log -Y "bluetooth.addr == ff:ff:11:c6:29:50 && btatt.opcode == 0x52" -T fields -e frame.time_relative -e btatt.handle -e btatt.value
```

### Extract connection parameters:
```bash
tshark -r btsnoop_hci.log -Y "bluetooth.addr == ff:ff:11:c6:29:50 && bthci_evt.code == 0x0e" -V
```

---

## Critical Questions to Answer

1. **What does the app write FIRST after connection?**
   - Before CCCD?
   - After CCCD?
   - Any setup frames?

2. **How fast does the app operate?**
   - Time from connection to CCCD write?
   - Any delays built in?

3. **Does the app send keep-alive?**
   - How soon after connection?
   - What interval?

4. **Are there hidden operations?**
   - MTU exchange?
   - Connection parameter updates?
   - Any writes to handles we don't know about?

5. **Why does nRF fail but app succeeds?**
   - Different connection parameters?
   - Missing a step?
   - Timing issue?

---

## What to Share

After analysis, please share:

1. **Connection parameters used by app**
2. **Exact timing between operations** (in milliseconds)
3. **First 10 operations after connection** (type and timing)
4. **Any writes to unknown handles**
5. **Keep-alive timing** (if present)
6. **Differences between app and nRF attempts**

You can copy this info or export relevant Wireshark packets as text.

---

## If Wireshark is Overwhelming

Just share these key findings:

1. **Time from connection to CCCD write:** _______ ms
2. **First write after connection:** Handle _____, Value: __ __ __ __
3. **App sends keep-alive?** YES / NO, Interval: _______ ms
4. **Connection succeeds on attempt:** 1 / 2 / 3 / ...
5. **Any error codes seen?** _______

This is enough to adjust the ESP32 timing!
