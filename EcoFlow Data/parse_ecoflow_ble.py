#!/usr/bin/env python3
"""
Parse EcoFlow BLE btsnoop logs to reverse-engineer the protocol.
"""

import sys
import struct

def parse_btsnoop(filename):
    """Parse btsnoop HCI log file and extract interesting BLE packets."""

    with open(filename, 'rb') as f:
        # Read btsnoop header (16 bytes)
        header = f.read(16)
        if header[:8] != b'btsnoop\x00':
            print(f"[!] Not a valid btsnoop file: {filename}")
            return

        print(f"[*] Parsing {filename}")
        print("=" * 80)

        packet_count = 0
        write_packets = []
        notify_packets = []
        cid_counts = {}

        while True:
            # Read packet record header (24 bytes)
            record_header = f.read(24)
            if len(record_header) < 24:
                break

            orig_len, inc_len, flags, drops, timestamp = struct.unpack('>IIIIQ', record_header)

            # Read packet data
            packet_data = f.read(inc_len)
            if len(packet_data) < inc_len:
                break

            packet_count += 1

            # Look for HCI ACL Data packets (0x02) - these contain L2CAP/ATT
            if len(packet_data) > 0 and packet_data[0] == 0x02:
                # Parse HCI ACL header
                if len(packet_data) < 5:
                    continue

                handle_flags = struct.unpack('<H', packet_data[1:3])[0]
                data_len = struct.unpack('<H', packet_data[3:5])[0]

                if len(packet_data) < 5 + data_len:
                    continue

                # Extract L2CAP data
                l2cap_data = packet_data[5:5+data_len]

                if len(l2cap_data) < 4:
                    continue

                l2cap_len = struct.unpack('<H', l2cap_data[0:2])[0]
                l2cap_cid = struct.unpack('<H', l2cap_data[2:4])[0]

                # Count CIDs
                cid_counts[l2cap_cid] = cid_counts.get(l2cap_cid, 0) + 1

                # CID 0x0004 is ATT protocol
                if l2cap_cid == 0x0004 and len(l2cap_data) > 4:
                    att_data = l2cap_data[4:]

                    if len(att_data) == 0:
                        continue

                    att_opcode = att_data[0]

                    # 0x12 = Write Request, 0x52 = Write Command
                    if att_opcode in [0x12, 0x52]:
                        handle = struct.unpack('<H', att_data[1:3])[0] if len(att_data) >= 3 else 0
                        value = att_data[3:] if len(att_data) > 3 else b''

                        opcode_name = "Write Request" if att_opcode == 0x12 else "Write Command"
                        write_packets.append({
                            'packet': packet_count,
                            'opcode': opcode_name,
                            'handle': handle,
                            'value': value
                        })

                    # 0x1B = Handle Value Notification
                    elif att_opcode == 0x1B:
                        handle = struct.unpack('<H', att_data[1:3])[0] if len(att_data) >= 3 else 0
                        value = att_data[3:] if len(att_data) > 3 else b''

                        notify_packets.append({
                            'packet': packet_count,
                            'handle': handle,
                            'value': value
                        })

        print(f"\n[*] Total packets: {packet_count}")
        print(f"[*] L2CAP CID distribution:")
        for cid, count in sorted(cid_counts.items()):
            print(f"    CID 0x{cid:04X}: {count} packets")
        print(f"[*] Write packets (commands to EcoFlow): {len(write_packets)}")
        print(f"[*] Notification packets (data from EcoFlow): {len(notify_packets)}")

        # Analyze write packets (commands from app)
        if write_packets:
            print("\n" + "=" * 80)
            print("WRITE PACKETS (App → EcoFlow Commands)")
            print("=" * 80)

            for i, pkt in enumerate(write_packets[:20]):  # Show first 20
                print(f"\n[WRITE] Packet #{pkt['packet']}: {pkt['opcode']}")
                print(f"   Handle: 0x{pkt['handle']:04X}")
                print(f"   Length: {len(pkt['value'])} bytes")

                # Print hex dump
                hex_str = ' '.join(f"{b:02X}" for b in pkt['value'])
                print(f"   Data: {hex_str}")

                # Look for 0x5A 0x5A header (EcoFlow protocol marker)
                if len(pkt['value']) >= 2 and pkt['value'][0] == 0x5A and pkt['value'][1] == 0x5A:
                    print(f"   [!] FOUND 0x5A 0x5A header! This is EcoFlow protocol V2")

                # Check if it's CCCD (Client Characteristic Configuration Descriptor)
                if pkt['handle'] in [0x2902] or len(pkt['value']) == 2:
                    val = struct.unpack('<H', pkt['value'])[0] if len(pkt['value']) == 2 else 0
                    if val == 0x0001:
                        print(f"   [+] Enable Notifications")
                    elif val == 0x0002:
                        print(f"   [+] Enable Indications")

        # Analyze notification packets (data from EcoFlow)
        if notify_packets:
            print("\n" + "=" * 80)
            print("NOTIFICATION PACKETS (EcoFlow → App Data)")
            print("=" * 80)

            for i, pkt in enumerate(notify_packets[:20]):  # Show first 20
                print(f"\n[NOTIFY] Packet #{pkt['packet']}")
                print(f"   Handle: 0x{pkt['handle']:04X}")
                print(f"   Length: {len(pkt['value'])} bytes")

                # Print hex dump (first 64 bytes)
                hex_str = ' '.join(f"{b:02X}" for b in pkt['value'][:64])
                if len(pkt['value']) > 64:
                    hex_str += " ..."
                print(f"   Data: {hex_str}")

                # Look for 0x5A 0x5A header
                if len(pkt['value']) >= 2 and pkt['value'][0] == 0x5A and pkt['value'][1] == 0x5A:
                    print(f"   [!] FOUND 0x5A 0x5A header! This is EcoFlow protocol V2")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python parse_ecoflow_ble.py <btsnoop_file>")
        sys.exit(1)

    parse_btsnoop(sys.argv[1])
