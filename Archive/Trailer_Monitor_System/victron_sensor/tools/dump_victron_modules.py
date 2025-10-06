import inspect
import victron_ble.devices.battery_monitor as bm
import victron_ble.devices.solar_charger as sc
import victron_ble.devices.dc_energy_meter as dc

modules = {'battery_monitor': bm, 'solar_charger': sc, 'dc_energy_meter': dc}
for name, mod in modules.items():
    try:
        src = inspect.getsource(mod)
    except Exception as e:
        src = f'Error: {e}'
    with open(f'{name}_source.txt', 'w', encoding='utf-8') as f:
        f.write(src)
    print('Wrote', name+'_source.txt')
