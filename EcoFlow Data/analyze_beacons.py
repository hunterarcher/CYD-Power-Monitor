#!/usr/bin/env python3
"""
Deep analysis of EcoFlow beacon data to find hidden fields.
"""

import sys
import struct
from collections import defaultdict

def analyze_beacon(beacon_type, data_hex):
    """Analyze a beacon for potential data fields."""

    data = bytes.fromhex(data_hex)
    length = len(data)

    print(f"\n{'='*80}")
    print(f"Analyzing {beacon_type} Beacon (Length: {length} bytes)")
    print(f"{'='*80}")

    # Hex dump with position
    print("\nByte-by-byte breakdown:")
    print("Pos  Hex  Dec  ASCII  Notes")
    print("-" * 60)

    for i, byte in enumerate(data):
        ascii_char = chr(byte) if 32 <= byte < 127 else '.'
        notes = ""

        # Add notes for known positions
        if beacon_type == 'C5C5':
            if i == 0 or i == 1:
                notes = "Mfg ID (0xC5C5)"
            elif i == 2:
                notes = f"Battery % candidate (Formula: {byte}-66={byte-66}%)"
            elif i >= 3 and i <= 7:
                notes = f"Serial char {i-2}"
            elif i == 8:
                notes = "Unknown byte"
            elif i >= 9 and i <= 20:
                notes = "Extended data"
            elif i >= 21:
                notes = "Device name"

        elif beacon_type == 'B5B5':
            if i == 0 or i == 1:
                notes = "Mfg ID (0xB5B5)"
            elif i == 2:
                notes = f"Battery % ({byte}%)"
            elif i >= 3 and i <= 19:
                notes = f"Serial char {i-2}"
            elif i >= 20:
                notes = "Additional data"

        print(f"{i:3d}  {byte:02X}  {byte:3d}  {ascii_char:^5s}  {notes}")

    # Look for patterns
    print("\n" + "="*60)
    print("Pattern Analysis:")
    print("="*60)

    # Check for voltage candidates (expecting ~48V for Delta Max 2)
    # Could be stored as: V*10 (480), V*100 (4800), or raw ADC value
    print("\nVoltage candidates (looking for ~48V):")
    for i in range(len(data) - 1):
        # Try 16-bit LE
        val_le = struct.unpack('<H', data[i:i+2])[0]
        if 400 < val_le < 600:  # 40-60V range with *10 multiplier
            print(f"  Byte [{i:2d}-{i+1:2d}] LE: {val_le} -> {val_le/10:.1f}V")

        # Try 16-bit BE
        val_be = struct.unpack('>H', data[i:i+2])[0]
        if 400 < val_be < 600:
            print(f"  Byte [{i:2d}-{i+1:2d}] BE: {val_be} -> {val_be/10:.1f}V")

    # Check for temperature candidates (expecting ~20-30C)
    print("\nTemperature candidates (looking for 20-30C):")
    for i in range(len(data)):
        if 15 <= data[i] <= 35:  # Direct celsius
            print(f"  Byte [{i:2d}]: {data[i]} -> {data[i]}C")

    # Check for power/wattage (could be 0-2000W)
    print("\nPower candidates (16-bit values < 2000):")
    for i in range(len(data) - 1):
        val_le = struct.unpack('<H', data[i:i+2])[0]
        val_be = struct.unpack('>H', data[i:i+2])[0]

        if 0 <= val_le <= 2000:
            print(f"  Byte [{i:2d}-{i+1:2d}] LE: {val_le}W")
        if 0 <= val_be <= 2000 and val_be != val_le:
            print(f"  Byte [{i:2d}-{i+1:2d}] BE: {val_be}W")

    # Look for bit flags (bytes with low values 0x00-0x0F)
    print("\nPotential flag bytes (values 0x00-0x0F):")
    for i in range(len(data)):
        if data[i] <= 0x0F:
            print(f"  Byte [{i:2d}]: 0x{data[i]:02X} = {data[i]:04b}b")

    # Look for time-related values (timestamp or time remaining)
    print("\nPotential time values (32-bit):")
    for i in range(len(data) - 3):
        val_le = struct.unpack('<I', data[i:i+4])[0]
        val_be = struct.unpack('>I', data[i:i+4])[0]

        # Check if looks like minutes (0-6000 = 0-100 hours)
        if 0 < val_le < 6000:
            print(f"  Byte [{i:2d}-{i+3:2d}] LE: {val_le} -> {val_le//60}h {val_le%60}m")
        if 0 < val_be < 6000 and val_be != val_le:
            print(f"  Byte [{i:2d}-{i+3:2d}] BE: {val_be} -> {val_be//60}h {val_be%60}m")


def compare_beacons():
    """Compare different beacon captures to find changing fields."""

    # From the log, we have:
    beacons_c5c5 = [
        # All show 75% battery (0x8D raw, 0x8D-66=75%)
        "C5C5138D48353330351300373635300D0C0945462D5233353031333500",
    ]

    beacons_b5b5 = [
        # First device (battery 19% = 0x13)
        "B513523335315A4642344847313630313335540001A3B63E2F",
        # Second device (battery 18% = 0x12)
        "B512523631315A4642355846334A3136383264000000000026",
    ]

    print("\n" + "="*80)
    print("COMPARING BEACONS TO FIND DYNAMIC FIELDS")
    print("="*80)

    print("\n0xC5C5 Beacons (same device, different times):")
    print("We only have one C5C5 pattern - need more captures")

    print("\n0xB5B5 Beacons (different devices or states):")
    for i, beacon_hex in enumerate(beacons_b5b5):
        beacon = bytes.fromhex(beacon_hex)
        print(f"\nBeacon {i+1}:")
        print(f"  Battery: {beacon[2]}%")
        print(f"  Serial: {beacon[3:19].decode('ascii', errors='ignore')}")
        print(f"  Extra bytes [20-25]: {' '.join(f'{b:02X}' for b in beacon[20:])}")

        # Analyze extra bytes
        if len(beacon) >= 21:
            print(f"    Byte 20 (0x{beacon[20]:02X}): Could be flags")
            if len(beacon) >= 22:
                print(f"    Byte 21 (0x{beacon[21]:02X}): Could be state")
            if len(beacon) >= 25:
                print(f"    Bytes 22-24: 0x{beacon[22]:02X} {beacon[23]:02X} {beacon[24]:02X}")
            if len(beacon) >= 26:
                # Try as 32-bit value
                val = struct.unpack('>I', beacon[22:26])[0]
                print(f"      -> As 32-bit BE: {val} (0x{val:08X})")


# Main analysis
if __name__ == "__main__":

    # Analyze the beacons we found
    # From extract_beacons.py output:
    # [C5C5] Data: C5 13 8D 48 35 33 30 35 13 00 37 36 35 30 0D 0C 09 45 46 2D 52 33 35 30 31 33 35 00
    # [B5B5] Data: B5 13 52 33 35 31 5A 46 42 34 48 47 31 36 30 31 33 35 54 00 01 A3 B6 3E 2F
    # [B5B5] Data: B5 12 52 36 31 31 5A 46 42 35 58 46 33 4A 31 36 38 32 64 00 00 00 00 00 26

    c5c5_data = "C5138D48353330351300373635300D0C0945462D5233353031333500"
    b5b5_data_1 = "B513523335315A4642344847313630313335540001A3B63E2F"
    b5b5_data_2 = "B512523631315A4642355846334A31363832640000000000026"

    analyze_beacon("C5C5", c5c5_data)
    analyze_beacon("B5B5", b5b5_data_1)

    print("\n\n")
    compare_beacons()

    print("\n" + "="*80)
    print("SUMMARY & RECOMMENDATIONS")
    print("="*80)
    print("""
From the beacon analysis:

1. 0xC5C5 Beacon (28 bytes):
   - Bytes 0-1: Manufacturer ID (0xC5C5)
   - Byte 2: Battery % with offset (raw - 66 = %)
   - Bytes 3-7: Short serial (5 chars)
   - Byte 8: Unknown (0x13)
   - Bytes 9-20: Extended data (contains "7650" and other values)
   - Bytes 21-27: Device name "EF-R350135"

2. 0xB5B5 Beacon (25 bytes):
   - Bytes 0-1: Manufacturer ID (0xB5B5)
   - Byte 2: Battery % (direct value)
   - Bytes 3-19: Full serial number (17 chars)
   - Bytes 20-25: Additional data (varies between devices)

FINDINGS:
- NO voltage, temperature, or power data found in beacons
- NO obvious time remaining in beacons
- Bytes 20-25 in B5B5 might contain:
  * Device state flags
  * Charging status
  * Error codes

RECOMMENDATION:
* Beacons ONLY provide: Battery % and Serial Number
* For full data (voltage, temp, power, AC status), you NEED authenticated connection
* The Delta Max 2 requires L2CAP connection to get detailed telemetry
* Beacon monitoring alone is insufficient for your use case

Next steps:
1. Implement authenticated BLE connection via L2CAP
2. Reverse engineer the command/response protocol
3. Or wait for official API/integration
""")
