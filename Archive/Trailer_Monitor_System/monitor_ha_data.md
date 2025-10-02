# Monitor Home Assistant Victron Data

## Method 1: Enable Victron HACS Debug Logging
Add to Home Assistant `configuration.yaml`:

```yaml
logger:
  default: info
  logs:
    custom_components.victron_ble: debug
    victron_ble: debug
```

Then check logs at: **Settings → System → Logs**

## Method 2: Monitor Bluetooth Proxy ESPHome Device
If using ESPHome Bluetooth proxy, enable debug logging in the ESPHome config:

```yaml
logger:
  level: DEBUG
  logs:
    esp32_ble_tracker: DEBUG
    bluetooth_proxy: DEBUG
```

## Method 3: Home Assistant Developer Tools
Real-time entity monitoring:
1. **Developer Tools → States**
2. Filter for Victron entities
3. Watch values update in real-time

## Method 4: ESPHome Logs (Most Detailed)
If you have access to the ESPHome Bluetooth proxy device:
- View logs in ESPHome dashboard
- Or via `esphome logs bluetooth-proxy.yaml`

## What We Need to Capture:
1. **Raw BLE advertisement data** from Bluetooth proxy
2. **Parsed values** sent to Home Assistant
3. **Entity state changes** in Home Assistant
4. **Timestamps** to correlate with our ESP32 data

## Our Target Data Format:
```
BMV (C0:3B:98:39:E6:FE):
- Raw: A2 9C E6 40 A8 E0 2A 1E 2C 98 63 7E 8A 0B E6 EE 54
- HACS Result: ??.?V, ??.?A, ??% SOC

MPPT (E8:86:01:5D:79:38):  
- Raw: 7C A2 DB 6D B0 1A 14 10 A2 C5 28 4C DE 01
- HACS Result: ??.?V, ??.?A, ????W

AC (C7:A2:C2:61:9F:C4):
- Raw: 41 04 6E CF FE AA 57 AE 4B B3 25 FE FA B1 E0
- HACS Result: ??.?V, ??.?A, ????W
```