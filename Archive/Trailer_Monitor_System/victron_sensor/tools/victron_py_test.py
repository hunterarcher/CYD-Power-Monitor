"""
Quick test harness to run the `victron_ble` Python library against captured decrypted payloads.
Place this file in `victron_sensor/tools/` and run with the project's Python (system python) after installing the package:

pip install victron-ble==0.4.0
python victron_py_test.py

This script will attempt to import the library, introspect available functions, and call detection/parsing helpers where present.
"""

from binascii import hexlify

SAMPLES = {
    'bmv_a': bytes.fromhex('0F8D87895CA2095480A0F4FBAB5290FB3C'),
    'bmv_b': bytes.fromhex('39926B591105B487316FF8AFF2B44E064D'),
    'mppt_a': bytes.fromhex('10F832FA52E41B2C5091725AFB97'),
    'mppt_b': bytes.fromhex('75398D054AFD1428F88C8F31CB0C'),
    'ac_a': bytes.fromhex('8A7ED2951B951F80ACFE8888674455'),
    'ac_b': bytes.fromhex('33C270EAB6F158AB8D43826336F33C')
}

print('Samples:')
for k, v in SAMPLES.items():
    print(f' - {k}: {len(v)} bytes {hexlify(v).decode()}')

try:
    import victron_ble
    print('\nImported victron_ble, version:', getattr(victron_ble, '__version__', 'unknown'))
    # Try to import devices helpers
    try:
        from victron_ble import devices
        print('victron_ble.devices module loaded')
    except Exception as e:
        devices = None
        print('Unable to import victron_ble.devices:', e)

    # If devices module exposes helpers, try to use them
    if devices:
        funcs = [name for name in dir(devices) if not name.startswith('_')]
        print('\nAvailable names in victron_ble.devices:', funcs)

        for name, sample in SAMPLES.items():
            print('\n---', name, '---')
            try:
                # try detect_device_type if available
                if hasattr(devices, 'detect_device_type'):
                    dt = devices.detect_device_type(sample)
                    print('detect_device_type ->', dt)
                else:
                    print('detect_device_type not present')

                # try parse_decrypted variant
                if hasattr(devices, 'parse_decrypted'):
                    parsed = devices.parse_decrypted(sample)
                    print('parse_decrypted ->', parsed)
                else:
                    # fallback: try device classes
                    print('parse_decrypted not present; listing device parsers...')
                    # attempt to call any callables with "parse" in name
                    for n in funcs:
                        if 'parse' in n.lower() and callable(getattr(devices, n)):
                            try:
                                out = getattr(devices, n)(sample)
                                print(f'{n} ->', out)
                            except Exception:
                                pass
            except Exception as e:
                print('Error parsing sample:', e)

except Exception as exc:
    print('Failed to import victron_ble library:', exc)
    print('You may need to pip install victron-ble==0.4.0')

print('\nDone')
