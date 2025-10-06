# How to Check HACS Victron Device Identification

## Method 1: Home Assistant UI
1. Go to **Settings** → **Devices & Services**
2. Find the **Victron** integration
3. Click on it to see all detected devices
4. Each device will show:
   - Device Name
   - Model
   - MAC Address
   - Device Type (BMV, MPPT, etc.)

## Method 2: Developer Tools - States
1. Go to **Developer Tools** → **States**
2. Filter for entities starting with `sensor.`
3. Look for Victron-related sensors
4. Each sensor shows its `device_class`, `friendly_name`, and attributes

## Method 3: HACS Integration Logs
1. Go to **Settings** → **System** → **Logs**
2. Filter for "victron" or "bluetooth"
3. Look for device discovery messages like:
   ```
   Discovered Victron device: BMV-712 (C0:3B:98:39:E6:FE)
   Discovered Victron device: MPPT 100|20 (E8:86:01:5D:79:38)
   ```

## Method 4: Check Entity Registry (Advanced)
1. Go to **Developer Tools** → **Services**
2. Call service: `system_log.write`
3. Or use Home Assistant logs to see device registration

## What We Need to Find:
- **Device Model Names** (e.g., "BMV-712", "MPPT 100|20")
- **Device Types** as identified by HACS
- **Current Values** for voltage, current, power, SOC
- **MAC Address Mapping** to confirm we're reading the same devices

## Our Current Devices:
- C0:3B:98:39:E6:FE = "KARSTEN MAXI SHUNT" (BMV)
- E8:86:01:5D:79:38 = "KARSTEN MAXI SOLAR" (MPPT)  
- C7:A2:C2:61:9F:C4 = "KARSTEN MAXI AC" (AC Charger)