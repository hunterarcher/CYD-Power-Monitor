from binascii import hexlify
import victron_ble
from victron_ble.devices import battery_monitor, solar_charger, dc_energy_meter

SAMPLES = {
    'bmv_a': bytes.fromhex('0F8D87895CA2095480A0F4FBAB5290FB3C'),
    'bmv_b': bytes.fromhex('39926B591105B487316FF8AFF2B44E064D'),
    'mppt_a': bytes.fromhex('10F832FA52E41B2C5091725AFB97'),
    'mppt_b': bytes.fromhex('75398D054AFD1428F88C8F31CB0C'),
    'ac_a': bytes.fromhex('8A7ED2951B951F80ACFE8888674455'),
    'ac_b': bytes.fromhex('33C270EAB6F158AB8d43826336F33C'.replace(' ', ''))
}

print('victron_ble version:', getattr(victron_ble, '__version__', 'unknown'))
print('Trying PACKET.parse on decrypted payload samples\n')

for name, data in SAMPLES.items():
    print('---', name, hexlify(data).decode(), 'len', len(data))
    # battery monitor
    try:
        pkt = battery_monitor.BatteryMonitor.PACKET.parse(data)
        aux_mode = battery_monitor.AuxMode(pkt.current & 0b11)
        parsed = {
            'remaining_mins': pkt.remaining_mins,
            'aux_mode': aux_mode.name,
            'current_A': (pkt.current >> 2) / 1000,
            'voltage_V': pkt.voltage / 100,
            'consumed_Ah': pkt.consumed_ah / 10,
            'soc_pct': ((pkt.soc & 0x3FFF) >> 4) / 10,
        }
        print(' battery_monitor ->', parsed)
    except Exception as e:
        print(' battery_monitor parse failed:', e)
    # solar charger
    try:
        pkt = solar_charger.SolarCharger.PACKET.parse(data)
        parsed = {
            'charge_state': pkt.charge_state,
            'battery_voltage_V': pkt.battery_voltage / 100,
            'battery_charging_current_A': pkt.battery_charging_current / 10,
            'yield_today_Wh': pkt.yield_today * 10,
            'solar_power_W': pkt.solar_power,
            'external_device_load': pkt.external_device_load,
        }
        print(' solar_charger ->', parsed)
    except Exception as e:
        print(' solar_charger parse failed:', e)
    # dc energy meter
    try:
        pkt = dc_energy_meter.DcEnergyMeter.PACKET.parse(data)
        parsed = {
            'meter_type': pkt.meter_type,
            'voltage_V': pkt.voltage / 100,
            'current_A': (pkt.current >> 2) / 1000,
        }
        print(' dc_energy_meter ->', parsed)
    except Exception as e:
        print(' dc_energy_meter parse failed:', e)

print('\nDone')
