#!/usr/bin/env python3
from binascii import hexlify
import victron_ble
from victron_ble.devices import battery_monitor, solar_charger
from Crypto.Cipher import AES
from Crypto.Util import Counter

KEYS = [
    '6cb52976b1b82ab4d6bc4d24ee356c1b',
    '7f8689f768ae1cb7018411538ae5fa85',
    'c9014b769559cb6fdf5a8a1361edf68a',
]

SAMPLES_ADV = {
    'mppt_dev': bytes.fromhex('100260A001E2B47FAF1CC58369EA4A62392B1FE7'),
    'bmv_dev': bytes.fromhex('100283A302EBDF6C99CC44A594E183FD6BFE37BED5E967'),
    'ac_dev': bytes.fromhex('10002EA3087217C937737880E1CA0D5F2E9A35A053'),
}

def decrypt_with_key_and_iv(enc_bytes, iv, key_hex, le_flag):
    key = bytes.fromhex(key_hex)
    ivBytes = bytearray(16)
    ivBytes[0] = iv & 0xFF
    ivBytes[1] = (iv >> 8) & 0xFF
    init_val = int.from_bytes(ivBytes, byteorder='little' if le_flag else 'big')
    ctr = Counter.new(128, initial_value=init_val, little_endian=le_flag)
    cipher = AES.new(key, AES.MODE_CTR, counter=ctr)
    return cipher.decrypt(enc_bytes)

def plausible(voltage_v, current_a):
    # Heuristics for plausibility: voltage 10..150 V, current magnitude < 1000 A
    return (10.0 <= voltage_v <= 150.0) and (abs(current_a) <= 1000.0)

def try_offsets():
    print('victron_ble version:', getattr(victron_ble, '__version__', 'unknown'))
    for name, adv in SAMPLES_ADV.items():
        print('\n== sample', name, hexlify(adv).decode())
        if len(adv) < 6:
            print(' adv too short')
            continue
        model_id = adv[1] | (adv[2] << 8)
        mode = adv[3]
        iv = adv[4] | (adv[5] << 8)
        enc_full = adv[6:]
        for offset in range(0, min(7, len(enc_full))):
            enc = enc_full[offset:]
            for key in KEYS:
                for le in (False, True):
                    try:
                        dec = decrypt_with_key_and_iv(enc, iv, key, le)
                    except Exception as e:
                        continue
                    # Try battery monitor parse
                    try:
                        pkt = battery_monitor.BatteryMonitor.PACKET.parse(dec)
                        voltage = pkt.voltage / 100.0
                        current = (pkt.current >> 2) / 1000.0
                        if plausible(voltage, current):
                            print('GOOD', name, 'offset', offset, 'key', key, 'le', le, 'V=', voltage, 'I=', current)
                    except Exception:
                        pass
                    # Try solar charger parse
                    try:
                        pkt = solar_charger.SolarCharger.PACKET.parse(dec)
                        voltage = pkt.battery_voltage / 100.0
                        current = pkt.battery_charging_current / 10.0
                        if plausible(voltage, current):
                            print('GOOD(SC)', name, 'offset', offset, 'key', key, 'le', le, 'V=', voltage, 'I=', current)
                    except Exception:
                        pass

if __name__ == '__main__':
    try_offsets()
