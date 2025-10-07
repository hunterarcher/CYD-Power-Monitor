#!/usr/bin/env python3
"""
Simple parser to dump L2CAP payloads from btsnoop.
"""

import sys
import struct

def parse_btsnoop(filename):
    """Parse btsnoop and dump L2CAP payloads."""

    with open(filename, 'rb') as f:
        # Skip header
        header = f.read(16)
        if header[:8] != b'btsnoop\x00':
            print("[!] Not a valid btsnoop file")
            return

        print(f"[*] Parsing {filename}\n")

        packet_count = 0
        payloads_with_5a = []

        while True:
            record_header = f.read(24)
            if len(record_header) < 24:
                break

            orig_len, inc_len, flags, drops, timestamp = struct.unpack('>IIIIQ', record_header)
            packet_data = f.read(inc_len)
            if len(packet_data) < inc_len:
                break

            packet_count += 1

            # Look for HCI ACL Data (0x02)
            if len(packet_data) > 5 and packet_data[0] == 0x02:
                # Skip HCI header (5 bytes)
                l2cap_data = packet_data[5:]

                if len(l2cap_data) > 4:
                    l2cap_len = struct.unpack('<H', l2cap_data[0:2])[0]
                    l2cap_cid = struct.unpack('<H', l2cap_data[2:4])[0]
                    payload = l2cap_data[4:4+l2cap_len] if len(l2cap_data) >= 4+l2cap_len else l2cap_data[4:]

                    # Look for 0x5A 0x5A header
                    if len(payload) >= 2 and payload[0] == 0x5A and payload[1] == 0x5A:
                        payloads_with_5a.append({
                            'packet': packet_count,
                            'cid': l2cap_cid,
                            'payload': payload
                        })

        print(f"[*] Total packets: {packet_count}")
        print(f"[*] Packets with 0x5A 0x5A header: {len(payloads_with_5a)}\n")

        if payloads_with_5a:
            print("=" * 80)
            print("PACKETS WITH 0x5A 0x5A HEADER (EcoFlow Protocol)")
            print("=" * 80)

            for pkt in payloads_with_5a[:30]:  # Show first 30
                print(f"\n[Packet #{pkt['packet']}] CID: 0x{pkt['cid']:04X}, Length: {len(pkt['payload'])} bytes")

                # Print full hex dump
                hex_lines = []
                for i in range(0, len(pkt['payload']), 16):
                    chunk = pkt['payload'][i:i+16]
                    hex_part = ' '.join(f"{b:02X}" for b in chunk)
                    ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
                    hex_lines.append(f"  {i:04X}:  {hex_part:<48} {ascii_part}")

                for line in hex_lines[:8]:  # Show first 8 lines (128 bytes)
                    print(line)
                if len(hex_lines) > 8:
                    print(f"  ... ({len(pkt['payload']) - 128} more bytes)")

        # Also dump first few L2CAP payloads regardless of header
        print("\n" + "=" * 80)
        print("FIRST 10 L2CAP PAYLOADS (for reference)")
        print("=" * 80)

        f.seek(16)  # Reset to start after header
        count = 0
        shown = 0

        while shown < 10:
            record_header = f.read(24)
            if len(record_header) < 24:
                break

            orig_len, inc_len, flags, drops, timestamp = struct.unpack('>IIIIQ', record_header)
            packet_data = f.read(inc_len)
            if len(packet_data) < inc_len:
                break

            count += 1

            if len(packet_data) > 5 and packet_data[0] == 0x02:
                l2cap_data = packet_data[5:]

                if len(l2cap_data) > 4:
                    l2cap_len = struct.unpack('<H', l2cap_data[0:2])[0]
                    l2cap_cid = struct.unpack('<H', l2cap_data[2:4])[0]
                    payload = l2cap_data[4:]

                    if len(payload) > 0:
                        print(f"\n[Packet #{count}] CID: 0x{l2cap_cid:04X}")
                        hex_str = ' '.join(f"{b:02X}" for b in payload[:64])
                        if len(payload) > 64:
                            hex_str += " ..."
                        print(f"  {hex_str}")
                        shown += 1

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python simple_parse.py <btsnoop_file>")
        sys.exit(1)

    parse_btsnoop(sys.argv[1])
