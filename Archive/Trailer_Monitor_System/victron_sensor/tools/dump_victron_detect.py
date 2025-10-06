import inspect
from victron_ble import devices

try:
    src = inspect.getsource(devices.detect_device_type)
except Exception as e:
    src = f'Error getting source: {e}'

with open('victron_detect_source.txt', 'w', encoding='utf-8') as f:
    f.write(src)

print('Wrote victron_detect_source.txt')
