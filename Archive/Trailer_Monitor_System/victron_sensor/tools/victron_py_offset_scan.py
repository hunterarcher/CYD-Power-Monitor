from binascii import hexlify
import victron_ble
from victron_ble.devices import battery_monitor, solar_charger, dc_energy_meter

SAMPLES = {
    'bmv_a': bytes.fromhex('0F8D87895CA2095480A0F4FBAB5290FB3C'),
    'bmv_b': bytes.fromhex('39926B591105B487316FF8AFF2B44E064D'),
    'mppt_a': bytes.fromhex('10F832FA52E41B2C5091725AFB97'),
    'mppt_b': bytes.fromhex('75398D054AFD1428F88C8F31CB0C'),
    'ac_a': bytes.fromhex('8A7ED2951B951F80ACFE8888674455'),
    'ac_b': bytes.fromhex('33C270EAB6F158AB8D43826336F33C')
}

print('Offset scan using victron_ble PACKET parsing (0..6 byte skips)')
for name, sample in SAMPLES.items():
    print('\n===', name, hexlify(sample).decode(), 'len', len(sample), '===')
    for offset in range(0,7):
        data = sample[offset:]
        note = ''
        try:
            pkt = battery_monitor.BatteryMonitor.PACKET.parse(data)
            voltage = pkt.voltage / 100
            current = (pkt.current >> 2) / 1000
            note += f'BMV voltage={voltage:.2f}V current={current:.3f}A'
        except Exception:
            note += 'BMV parse fail'
        try:
            pkt = solar_charger.SolarCharger.PACKET.parse(data)
            sv = pkt.battery_voltage / 100
            scur = pkt.battery_charging_current / 10
            note += f' | SOLAR V={sv:.2f}V I={scur:.2f}A'
        except Exception:
            note += ' | SOLAR fail'
        try:
            pkt = dc_energy_meter.DcEnergyMeter.PACKET.parse(data)
            dv = pkt.voltage / 100
            dcur = (pkt.current >> 2) / 1000
            note += f' | DC V={dv:.2f}V I={dcur:.3f}A'
        except Exception:
            note += ' | DC fail'
        print(f' offset {offset}: {note}')
print('\nDone')
