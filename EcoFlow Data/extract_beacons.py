#!/usr/bin/env python3
"""
Extract EcoFlow beacon data from btsnoop HCI log.
Looks for advertisement packets with 0xC5C5 and 0xB5B5 manufacturer IDs.
"""

import sys
import struct
from collections import defaultdict

def parse_btsnoop(filename):
    """Parse btsnoop and extract EcoFlow beacons."""

    beacons = defaultdict(lambda: {'count': 0, 'packets': []})

    with open(filename, 'rb') as f:
        # Skip header
        header = f.read(16)
        if header[:8] != b'btsnoop\x00':
            print("[!] Not a valid btsnoop file")
            return

        print(f"[*] Parsing {filename}\n")

        packet_count = 0

        while True:
            record_header = f.read(24)
            if len(record_header) < 24:
                break

            orig_len, inc_len, flags, drops, timestamp = struct.unpack('>IIIIQ', record_header)
            packet_data = f.read(inc_len)
            if len(packet_data) < inc_len:
                break

            packet_count += 1

            # Look for HCI Event packets (0x04) - these contain advertisements
            if len(packet_data) > 0 and packet_data[0] == 0x04:
                hex_str = packet_data.hex().upper()

                # Look for manufacturer data patterns
                # BLE advertisement format: ...FF<len><mfg_id_low><mfg_id_high><data>...

                # Pattern: FF followed by length, then C5C5 or B5B5
                for pattern in ['C5C5', 'B5B5']:
                    idx = hex_str.find(pattern)
                    if idx > 4:  # Need room for FF + length before pattern
                        # Go back to find the FF (manufacturer data type)
                        search_start = max(0, idx - 10)
                        ff_idx = hex_str.rfind('FF', search_start, idx)

                        if ff_idx >= 0 and ff_idx < idx:
                            # Extract length byte after FF
                            try:
                                length_byte = int(hex_str[ff_idx+2:ff_idx+4], 16)
                                # Extract full manufacturer data
                                mfg_data_start = ff_idx + 4  # After FF and length
                                mfg_data_end = min(mfg_data_start + (length_byte * 2), len(hex_str))
                                mfg_data = hex_str[mfg_data_start:mfg_data_end]

                                # Store beacon
                                key = (pattern, len(mfg_data) // 2, mfg_data)
                                beacons[key]['count'] += 1
                                if len(beacons[key]['packets']) < 5:
                                    beacons[key]['packets'].append(packet_count)
                            except:
                                pass

    print(f"[*] Total packets: {packet_count}")
    print(f"[*] Unique beacon patterns: {len(beacons)}\n")

    print("=" * 80)
    print("ECOFLOW BEACON PATTERNS")
    print("=" * 80)

    # Sort by type then by count
    sorted_beacons = sorted(beacons.items(), key=lambda x: (x[0][0], -x[1]['count']))

    for (beacon_type, length, data), info in sorted_beacons:
        print(f"\n[{beacon_type}] Length: {length} bytes (seen {info['count']}x)")
        print(f"  Packets: {info['packets'][:5]}")

        # Format data nicely
        formatted = ' '.join(data[i:i+2] for i in range(0, len(data), 2))
        print(f"  Data: {formatted}")

        # Try to decode
        if beacon_type == 'C5C5' and length == 10:
            try:
                bytes_data = bytes.fromhex(data)
                battery_raw = bytes_data[2]
                serial = bytes_data[3:8].decode('ascii', errors='ignore')
                battery_pct = battery_raw - 66 if battery_raw > 66 else battery_raw
                print(f"  → Battery: {battery_pct}% (raw: 0x{battery_raw:02X})")
                print(f"  → Serial: {serial}")
            except:
                pass

        elif beacon_type == 'B5B5' and length == 20:
            try:
                bytes_data = bytes.fromhex(data)
                battery_raw = bytes_data[2]
                serial = bytes_data[3:20].decode('ascii', errors='ignore')
                print(f"  → Battery: {battery_raw}% (raw: 0x{battery_raw:02X})")
                print(f"  → Serial: {serial}")
            except:
                pass

        elif beacon_type == 'B5B5' and length == 26:
            try:
                bytes_data = bytes.fromhex(data)
                battery_raw = bytes_data[2]
                # Try to find what's different in len=26 vs len=20
                print(f"  → Battery: {battery_raw}% (raw: 0x{battery_raw:02X})")
                print(f"  → Bytes [20-26]: {data[40:52]}")
            except:
                pass

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python extract_beacons.py <btsnoop_file>")
        sys.exit(1)

    parse_btsnoop(sys.argv[1])
