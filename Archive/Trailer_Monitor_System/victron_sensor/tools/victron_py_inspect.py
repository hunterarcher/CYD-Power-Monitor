import victron_ble
from victron_ble import devices
import inspect

print('victron_ble package:', victron_ble)
print('devices module file:', getattr(devices, '__file__', 'no file'))

names = [n for n in dir(devices) if not n.startswith('_')]
print('\nExported names:', names)

for n in names:
    obj = getattr(devices, n)
    print('\n----', n, '----')
    print('type:', type(obj))
    try:
        if inspect.isclass(obj) or inspect.isfunction(obj):
            print('signature:', inspect.signature(obj))
            print('doc:', (obj.__doc__ or '').split('\n')[0])
        else:
            print('repr:', repr(obj)[:400])
    except Exception as e:
        print('inspection error:', e)
