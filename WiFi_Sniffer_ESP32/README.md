# WiFi Traffic Sniffer for EcoFlow Delta Max 2

## Purpose
Capture HTTP/TCP traffic from EcoFlow Delta Max 2 when connected to ESP32 Access Point to discover local API endpoints.

## How It Works

1. **ESP32 creates WiFi AP** named "PowerMonitor"
2. **EcoFlow connects** to this AP (via app settings)
3. **ESP32 logs ALL HTTP requests** from the EcoFlow
4. **Analyze** captured requests to find API structure

## Setup Instructions

### 1. Upload to ESP32
```bash
cd "C:\Trailer\CYD Build\WiFi_Sniffer_ESP32"
pio run --target upload
pio device monitor
```

### 2. Connect EcoFlow to AP

**Using EcoFlow App:**
1. Open EcoFlow app on phone
2. Make sure phone has mobile data or is near WiFi (for app to work)
3. Go to Settings → WiFi (or similar)
4. Connect EcoFlow to WiFi network "PowerMonitor"
5. Password: `PowerMonitor123`

### 3. Watch Serial Monitor

You'll see:
- ✓ When EcoFlow connects (with MAC and IP address)
- 📡 All HTTP requests (method, URI, headers, body)
- Full request details logged

### 4. Test API Discovery

**Try these from EcoFlow app:**
- View battery status
- Toggle AC output ON/OFF
- Change settings

Each action should generate HTTP requests that we'll see!

## What We're Looking For

### Best Case:
```
📡 HTTP REQUEST RECEIVED!
Method: GET
URI: /api/status
Headers: ...
Response: {"battery": 84, "ac_output": true, ...}
```
= Simple JSON API! Easy to implement!

### Likely Case:
```
📡 HTTP REQUEST RECEIVED!
Method: POST
URI: /api/command
Body: [encrypted binary data]
```
= Encrypted API, need to analyze further

### Worst Case:
- No HTTP requests at all
- EcoFlow uses proprietary TCP protocol
- Would need TCP packet sniffer (more complex)

## Configuration

**AP Settings:**
- SSID: `PowerMonitor`
- Password: `PowerMonitor123`
- Channel: 1
- Max Clients: 4

**Change these in main.cpp if needed**

## Next Steps Based on Results

### If We See HTTP/JSON:
✅ Implement local API client on Master ESP32
✅ Parse JSON responses
✅ Send commands via HTTP

### If We See Encrypted HTTP:
⚠️ Analyze encryption method
⚠️ Compare with BLE encryption
⚠️ Might still be easier than BLE

### If No HTTP Traffic:
❌ Try TCP packet capture (more advanced)
❌ Or fall back to BLE authentication method

## Troubleshooting

**EcoFlow won't connect:**
- Check password is correct (8+ chars required)
- Make sure ESP32 AP is visible
- Try forgetting network and reconnecting
- Check EcoFlow firmware supports WiFi

**No requests logged:**
- EcoFlow might need internet for app to work
- Try with phone on mobile data
- EcoFlow might not make local requests
- Check firewall/security settings

**ESP32 crashes:**
- Increase monitoring task stack size
- Disable debug logging if too verbose

## Technical Details

- **Web Server:** ESP32 AsyncWebServer on port 80
- **WiFi Mode:** AP (Access Point) mode
- **Logging:** All HTTP methods (GET, POST, PUT, DELETE, PATCH)
- **Captures:** URI, headers, parameters, body

## Expected vs Actual

**What we HOPE to find:**
```
GET /api/status
Response: {"battery": 84, "power_in": 0, "power_out": 1, ...}

POST /api/ac_output
Body: {"enabled": true}
Response: {"success": true}
```

**What we might ACTUALLY find:**
```
POST /some/path
Body: [binary encrypted data]
Headers: X-Auth-Token: [something]
```

Either way, we'll learn something useful!

---

**Created:** October 3, 2025
**Status:** Ready to test
