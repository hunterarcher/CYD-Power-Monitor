#!/usr/bin/env python3
"""
Track beacon changes over time in the EcoFlow app capture.
Looking for fields that change when AC outlets are toggled.
"""

import sys
import struct
from datetime import datetime

def parse_btsnoop_with_time(filename):
    """Parse btsnoop and track beacons over time."""

    beacons_by_time = []

    with open(filename, 'rb') as f:
        # Skip header
        header = f.read(16)
        if header[:8] != b'btsnoop\x00':
            print("[!] Not a valid btsnoop file")
            return

        print(f"[*] Parsing {filename} for temporal beacon changes\n")

        packet_count = 0
        start_timestamp = None

        while True:
            record_header = f.read(24)
            if len(record_header) < 24:
                break

            orig_len, inc_len, flags, drops, timestamp = struct.unpack('>IIIIQ', record_header)
            packet_data = f.read(inc_len)
            if len(packet_data) < inc_len:
                break

            packet_count += 1

            if start_timestamp is None:
                start_timestamp = timestamp

            # Calculate relative time in seconds
            rel_time = (timestamp - start_timestamp) / 1000000.0  # Convert microseconds to seconds

            # Look for HCI Event packets (0x04) - these contain advertisements
            if len(packet_data) > 0 and packet_data[0] == 0x04:
                hex_str = packet_data.hex().upper()

                # Look for C5C5 or B5B5 patterns
                for pattern in ['C5C5', 'B5B5']:
                    idx = hex_str.find(pattern)
                    if idx > 4:
                        ff_idx = hex_str.rfind('FF', max(0, idx - 10), idx)
                        if ff_idx >= 0 and ff_idx < idx:
                            try:
                                length_byte = int(hex_str[ff_idx+2:ff_idx+4], 16)
                                mfg_data_start = ff_idx + 4
                                mfg_data_end = min(mfg_data_start + (length_byte * 2), len(hex_str))
                                mfg_data = hex_str[mfg_data_start:mfg_data_end]

                                beacons_by_time.append({
                                    'time': rel_time,
                                    'packet': packet_count,
                                    'type': pattern,
                                    'data': mfg_data
                                })
                            except:
                                pass

    print(f"[*] Total packets: {packet_count}")
    print(f"[*] Total beacons found: {len(beacons_by_time)}")
    print(f"[*] Capture duration: {beacons_by_time[-1]['time']:.1f} seconds\n")

    # Group beacons by type and find unique patterns
    print("="*80)
    print("BEACON CHANGES OVER TIME")
    print("="*80)

    # Track unique beacons per type
    c5c5_beacons = {}
    b5b5_beacons = {}

    for beacon in beacons_by_time:
        if beacon['type'] == 'C5C5':
            if beacon['data'] not in c5c5_beacons:
                c5c5_beacons[beacon['data']] = []
            c5c5_beacons[beacon['data']].append(beacon['time'])
        elif beacon['type'] == 'B5B5':
            if beacon['data'] not in b5b5_beacons:
                b5b5_beacons[beacon['data']] = []
            b5b5_beacons[beacon['data']].append(beacon['time'])

    print(f"\n0xC5C5 Beacons: {len(c5c5_beacons)} unique pattern(s)")
    for i, (data, times) in enumerate(sorted(c5c5_beacons.items())):
        print(f"\n  Pattern {i+1} (seen {len(times)}x):")
        formatted = ' '.join(data[i:i+2] for i in range(0, len(data), 2))
        print(f"    Data: {formatted}")
        print(f"    First seen: {times[0]:.1f}s, Last seen: {times[-1]:.1f}s")

        # Try to decode
        bytes_data = bytes.fromhex(data)
        if len(bytes_data) >= 3:
            # Check if it's actually C5 C5 or if C5C5 is split differently
            if bytes_data[0] == 0xC5 and bytes_data[1] == 0xC5:
                # Manufacturer data starts with C5 C5
                battery_raw = bytes_data[2]
                print(f"    Battery: {battery_raw - 66}% (raw: 0x{battery_raw:02X})")
            elif bytes_data[0] == 0xC5:
                # Just C5 followed by data
                battery_raw = bytes_data[1]
                print(f"    Battery: {battery_raw}% (raw: 0x{battery_raw:02X})")

    print(f"\n0xB5B5 Beacons: {len(b5b5_beacons)} unique pattern(s)")
    for i, (data, times) in enumerate(sorted(b5b5_beacons.items())):
        print(f"\n  Pattern {i+1} (seen {len(times)}x):")
        formatted = ' '.join(data[i:i+2] for i in range(0, len(data), 2))
        print(f"    Data: {formatted}")
        print(f"    First seen: {times[0]:.1f}s, Last seen: {times[-1]:.1f}s")

        # Try to decode
        bytes_data = bytes.fromhex(data)
        if len(bytes_data) >= 3:
            if bytes_data[0] == 0xB5 and bytes_data[1] == 0xB5:
                battery_raw = bytes_data[2]
                print(f"    Battery: {battery_raw}% (raw: 0x{battery_raw:02X})")
            elif bytes_data[0] == 0xB5:
                battery_raw = bytes_data[1]
                print(f"    Battery: {battery_raw}% (raw: 0x{battery_raw:02X})")

    # Look for beacon changes
    print("\n" + "="*80)
    print("ANALYSIS: Looking for dynamic fields")
    print("="*80)

    if len(c5c5_beacons) == 1:
        print("\n0xC5C5: NO CHANGES during entire capture")
        print("  -> This beacon is STATIC (does not reflect device state changes)")

    if len(b5b5_beacons) > 1:
        print(f"\n0xB5B5: {len(b5b5_beacons)} different patterns found")
        print("  -> Analyzing differences...")

        patterns = list(b5b5_beacons.keys())
        for i in range(len(patterns) - 1):
            p1 = bytes.fromhex(patterns[i])
            p2 = bytes.fromhex(patterns[i+1])

            print(f"\n  Pattern {i+1} vs Pattern {i+2}:")
            for j in range(min(len(p1), len(p2))):
                if p1[j] != p2[j]:
                    print(f"    Byte {j}: 0x{p1[j]:02X} -> 0x{p2[j]:02X} (changed)")
    elif len(b5b5_beacons) == 1:
        print("\n0xB5B5: NO CHANGES during entire capture")
        print("  -> This beacon is STATIC (does not reflect device state changes)")

    print("\n" + "="*80)
    print("CONCLUSION")
    print("="*80)
    print("""
If beacons don't change when AC outlets are toggled in the app, then:
  * Beacons are ANNOUNCEMENT-ONLY (device presence + basic info)
  * Real-time state (AC on/off, power, voltage) is NOT in beacons
  * You MUST connect to the device to get/set real-time data

This is common for BLE devices - beacons advertise "I'm here",
but connection is required for control and detailed telemetry.
""")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python find_beacon_changes.py <btsnoop_file>")
        sys.exit(1)

    parse_btsnoop_with_time(sys.argv[1])
