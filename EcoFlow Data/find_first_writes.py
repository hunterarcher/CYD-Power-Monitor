#!/usr/bin/env python3
"""
Find the first few L2CAP payloads to see what the app sends initially.
Focus on short messages that might be handshake/hello/keep-alive.
"""

import sys
import struct

def analyze_initial_traffic(filename):
    """Find first messages in the session."""

    with open(filename, 'rb') as f:
        header = f.read(16)
        if header[:8] != b'btsnoop\x00':
            print("[!] Not valid btsnoop")
            return

        print(f"[*] Analyzing initial traffic from {filename}\n")
        print("=" * 80)
        print("FIRST 30 L2CAP PAYLOADS (chronological order)")
        print("=" * 80)

        count = 0
        shown = 0

        while shown < 30:
            record_header = f.read(24)
            if len(record_header) < 24:
                break

            orig_len, inc_len, flags, drops, timestamp_us = struct.unpack('>IIIIQ', record_header)
            packet_data = f.read(inc_len)
            if len(packet_data) < inc_len:
                break

            count += 1

            # HCI ACL packets
            if len(packet_data) > 5 and packet_data[0] == 0x02:
                l2cap_data = packet_data[5:]

                if len(l2cap_data) > 4:
                    l2cap_len = struct.unpack('<H', l2cap_data[0:2])[0]
                    l2cap_cid = struct.unpack('<H', l2cap_data[2:4])[0]
                    payload = l2cap_data[4:]

                    if len(payload) > 0:
                        # Calculate time in seconds from first packet
                        time_sec = timestamp_us / 1_000_000.0

                        print(f"\n[Packet #{count}] Time: {time_sec:.3f}s, CID: 0x{l2cap_cid:04X}, Length: {len(payload)} bytes")

                        # Print hex (limit to 128 bytes)
                        hex_lines = []
                        for i in range(0, min(len(payload), 128), 16):
                            chunk = payload[i:i+16]
                            hex_part = ' '.join(f"{b:02X}" for b in chunk)
                            hex_lines.append(f"  {hex_part}")

                        for line in hex_lines:
                            print(line)

                        if len(payload) > 128:
                            print(f"  ... ({len(payload) - 128} more bytes)")

                        # Look for interesting patterns
                        if len(payload) <= 32:
                            print(f"  --> SHORT MESSAGE (might be handshake/keep-alive)")

                        if payload[0:2] == b'\x00\x19':
                            if len(payload) > 2:
                                msg_type = payload[2:4]
                                print(f"  --> Message type: {msg_type.hex().upper()}")

                        shown += 1

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python find_first_writes.py <btsnoop_file>")
        sys.exit(1)

    analyze_initial_traffic(sys.argv[1])
