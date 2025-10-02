import inspect
from binascii import hexlify
import victron_ble
from victron_ble import devices
import victron_ble.devices.battery_monitor as bm
import victron_ble.devices.solar_charger as sc
import victron_ble.devices.dc_energy_meter as dc

SAMPLES = {
    'bmv_a': bytes.fromhex('0F8D87895CA2095480A0F4FBAB5290FB3C'),
    'bmv_b': bytes.fromhex('39926B591105B487316FF8AFF2B44E064D'),
    'mppt_a': bytes.fromhex('10F832FA52E41B2C5091725AFB97'),
    'mppt_b': bytes.fromhex('75398D054AFD1428F88C8F31CB0C'),
    'ac_a': bytes.fromhex('8A7ED2951B951F80ACFE8888674455'),
    'ac_b': bytes.fromhex('33C270EAB6F158AB8D43826336F33C')
}

print('victron_ble version:', getattr(victron_ble, '__version__', 'unknown'))
print('devices.detect_device_type signature:', inspect.signature(devices.detect_device_type))

modules = [('battery_monitor', bm), ('solar_charger', sc), ('dc_energy_meter', dc)]
for modname, mod in modules:
    print('\n--- Module:', modname, 'file:', getattr(mod, '__file__', 'n/a'))
    try:
        src = inspect.getsource(mod)
        print('module source length:', len(src))
    except Exception as e:
        print('could not get source:', e)
    names = [n for n in dir(mod) if not n.startswith('_')]
    print('exports:', names)


for name, sample in SAMPLES.items():
    print('\n=== Sample', name, hexlify(sample).decode(), 'len', len(sample), '===')
    for offset in range(0,5):
        data = bytes([0]*offset) + sample
        try:
            dt = devices.detect_device_type(data)
        except Exception as e:
            dt = f'error: {e}'
        print(f' offset {offset}: detect_device_type ->', dt)
        # Try module-specific parsers
        for modname, mod in modules:
            for n in dir(mod):
                if 'parse' in n.lower() and callable(getattr(mod, n)):
                    func = getattr(mod, n)
                    try:
                        out = func(data)
                        print(f'   {modname}.{n} succeeded ->', out)
                    except Exception as e:
                        # print(f'   {modname}.{n} error ->', e)
                        pass

print('\nDone')
