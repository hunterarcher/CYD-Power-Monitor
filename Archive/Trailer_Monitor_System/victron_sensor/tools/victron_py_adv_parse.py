#!/usr/bin/env python3
from binascii import hexlify
import victron_ble
from victron_ble.devices import battery_monitor, solar_charger, dc_energy_meter
from Crypto.Cipher import AES
from Crypto.Util import Counter

# Map MAC -> key if known. Update these with real keys if available.
KEYS = {
    # placeholder: fill with your device bind keys if you have them
    'BMV712': '6cb52976b1b82ab4d6bc4d24ee356c1b',
    'MPPT':   '7f8689f768ae1cb7018411538ae5fa85',
    'IP22':   'c9014b769559cb6fdf5a8a1361edf68a',
}

SAMPLES_ADV = {
    'mppt_dev': bytes.fromhex('100260A001E2B47FAF1CC58369EA4A62392B1FE7'),
    'bmv_dev': bytes.fromhex('100283A302EBDF6C99CC44A594E183FD6BFE37BED5E967'),
    'ac_dev': bytes.fromhex('10002EA3087217C937737880E1CA0D5F2E9A35A053'),
}

# Expected decrypted bytes from device serial logs (for quick parity check)
EXPECTED = {
    'mppt_dev': bytes.fromhex('B9FD38F2F4FF5181CF98B7D6871C'),
    'bmv_dev': bytes.fromhex('43D8A34271F254483F35AE69E8CBDC680C'),
    'ac_dev':  bytes.fromhex('9DCD30B945944F148DAADB0E4AA2DC'),
}

def try_decrypt_and_parse(name, adv_bytes, key_hex=None):
    print('\n---', name, hexlify(adv_bytes).decode(), 'len', len(adv_bytes))
    # adv format: [0x10][model_id(2)][mode][iv(2)][encrypted...]
    if len(adv_bytes) < 6:
        print(' adv too short')
        return
    rec_type = adv_bytes[0]
    model_id = adv_bytes[1] | (adv_bytes[2] << 8)
    mode = adv_bytes[3]
    iv = adv_bytes[4] | (adv_bytes[5] << 8)
    enc = adv_bytes[6:]
    print(' model=0x%04X mode=0x%02X iv=0x%04X enc_len=%d' % (model_id, mode, iv, len(enc)))

    # If key provided, try decrypting with it; otherwise, try keys in KEYS map
    candidate_keys = [key_hex] if key_hex else list(KEYS.values())
    if not candidate_keys:
        print(' No keys available in KEYS. Provide bind keys to test decryption parity.')
    for key in candidate_keys:
        try:
            keyBytes = bytes.fromhex(key)
            # Build IV block per spec: nonce (2 bytes LSB-first) into first two bytes of 16-byte IV
            ivBytes = bytearray(16)
            ivBytes[0] = iv & 0xFF
            ivBytes[1] = (iv >> 8) & 0xFF

            # Try a couple of counter setups. Pycryptodome's Counter.new supports little_endian flag.
            attempts = [
                ('ctr_be', False),
                ('ctr_le', True),
            ]
            for (name_e, le) in attempts:
                try:
                    init_val = int.from_bytes(ivBytes, byteorder='little' if le else 'big')
                    ctr = Counter.new(128, initial_value=init_val, little_endian=le)
                    cipher = AES.new(keyBytes, AES.MODE_CTR, counter=ctr)
                    decrypted = cipher.decrypt(enc)
                    print(f" key={key} mode={name_e} decrypted={hexlify(decrypted).decode()}")
                    if name in EXPECTED:
                        print('  matches expected:', decrypted == EXPECTED[name])

                    # Try PACKET parses on decrypted
                    try:
                        pkt = battery_monitor.BatteryMonitor.PACKET.parse(decrypted)
                        print('  battery_monitor parse OK ->', pkt)
                    except Exception as e:
                        print('  battery_monitor parse failed:', e)
                    try:
                        pkt = solar_charger.SolarCharger.PACKET.parse(decrypted)
                        print('  solar_charger parse OK ->', pkt)
                    except Exception as e:
                        print('  solar_charger parse failed:', e)
                    try:
                        pkt = dc_energy_meter.DcEnergyMeter.PACKET.parse(decrypted)
                        print('  dc_energy_meter parse OK ->', pkt)
                    except Exception as e:
                        print('  dc_energy_meter parse failed:', e)
                except Exception as e:
                    print('  AES attempt failed for mode', name_e, 'err=', e)
        except Exception as e:
            print(' decryption attempt failed with key=', key, 'err=', e)

def main():
    print('victron_ble version:', getattr(victron_ble, '__version__', 'unknown'))
    for name, adv in SAMPLES_ADV.items():
        try_decrypt_and_parse(name, adv)

if __name__ == '__main__':
    main()
