// Auto generated code by esphome
// ========== AUTO GENERATED INCLUDE BLOCK BEGIN ===========
#include "esphome.h"
using namespace esphome;
using std::isnan;
using std::min;
using std::max;
using namespace sensor;
using namespace text_sensor;
using namespace binary_sensor;
using namespace switch_;
logger::Logger *logger_logger_id;
esp32::ESP32InternalGPIOPin *esp32_esp32internalgpiopin_id;
status_led::StatusLED *status_led_statusled_id;
web_server_base::WebServerBase *web_server_base_webserverbase_id;
captive_portal::CaptivePortal *captive_portal_captiveportal_id;
wifi::WiFiComponent *wifi_wificomponent_id;
mdns::MDNSComponent *mdns_mdnscomponent_id;
esphome::ESPHomeOTAComponent *esphome_esphomeotacomponent_id;
web_server::WebServerOTAComponent *web_server_webserverotacomponent_id;
safe_mode::SafeModeComponent *safe_mode_safemodecomponent_id;
api::APIServer *api_apiserver_id;
using namespace api;
web_server::WebServer *web_server_webserver_id;
const uint8_t ESPHOME_WEBSERVER_INDEX_HTML[174] PROGMEM = {60, 33, 68, 79, 67, 84, 89, 80, 69, 32, 104, 116, 109, 108, 62, 60, 104, 116, 109, 108, 62, 60, 104, 101, 97, 100, 62, 60, 109, 101, 116, 97, 32, 99, 104, 97, 114, 115, 101, 116, 61, 85, 84, 70, 45, 56, 62, 60, 108, 105, 110, 107, 32, 114, 101, 108, 61, 105, 99, 111, 110, 32, 104, 114, 101, 102, 61, 100, 97, 116, 97, 58, 62, 60, 47, 104, 101, 97, 100, 62, 60, 98, 111, 100, 121, 62, 60, 101, 115, 112, 45, 97, 112, 112, 62, 60, 47, 101, 115, 112, 45, 97, 112, 112, 62, 60, 115, 99, 114, 105, 112, 116, 32, 115, 114, 99, 61, 34, 104, 116, 116, 112, 115, 58, 47, 47, 111, 105, 46, 101, 115, 112, 104, 111, 109, 101, 46, 105, 111, 47, 118, 50, 47, 119, 119, 119, 46, 106, 115, 34, 62, 60, 47, 115, 99, 114, 105, 112, 116, 62, 60, 47, 98, 111, 100, 121, 62, 60, 47, 104, 116, 109, 108, 62};
const size_t ESPHOME_WEBSERVER_INDEX_HTML_SIZE = 174;
using namespace json;
esp32_ble_tracker::ESP32BLETracker *esp32_ble_tracker_esp32bletracker_id;
victron_ble::VictronBle *shunt_device;
victron_ble::VictronBle *solar_device;
victron_ble::VictronSensor *battery_voltage;
victron_ble::VictronSensor *battery_current;
victron_ble::VictronSensor *consumed_ah;
victron_ble::VictronSensor *state_of_charge;
victron_ble::VictronSensor *time_to_go;
victron_ble::VictronSensor *solar_battery_voltage;
victron_ble::VictronSensor *solar_output_current;
victron_ble::VictronSensor *solar_pv_power;
victron_ble::VictronSensor *solar_yield_today;
uptime::UptimeSecondsSensor *uptime_uptimesecondssensor_id;
wifi_signal::WiFiSignalSensor *wifi_signal_wifisignalsensor_id;
victron_ble::VictronTextSensor *solar_state;
template_::TemplateTextSensor *system_status;
victron_ble::VictronBinarySensor *victron_ble_victronbinarysensor_id;
restart::RestartSwitch *restart_restartswitch_id;
interval::IntervalTrigger *interval_intervaltrigger_id;
Automation<> *automation_id;
LambdaAction<> *lambdaaction_id;
preferences::IntervalSyncer *preferences_intervalsyncer_id;
esp32_ble::ESP32BLE *esp32_ble_esp32ble_id;
#define yield() esphome::yield()
#define millis() esphome::millis()
#define micros() esphome::micros()
#define delay(x) esphome::delay(x)
#define delayMicroseconds(x) esphome::delayMicroseconds(x)
// ========== AUTO GENERATED INCLUDE BLOCK END ==========="

void setup() {
  // ========== AUTO GENERATED CODE BEGIN ===========
  // network:
  //   enable_ipv6: false
  //   min_ipv6_addr_count: 0
  // async_tcp:
  //   {}
  // esphome:
  //   name: trailer-monitor
  //   friendly_name: Trailer Victron Monitor
  //   min_version: 2025.2.0
  //   build_path: build\trailer-monitor
  //   platformio_options: {}
  //   includes: []
  //   libraries: []
  //   name_add_mac_suffix: false
  //   debug_scheduler: false
  //   areas: []
  //   devices: []
  App.pre_setup("trailer-monitor", "Trailer Victron Monitor", "", __DATE__ ", " __TIME__, false);
  // sensor:
  // text_sensor:
  // binary_sensor:
  // switch:
  // logger:
  //   level: INFO
  //   id: logger_logger_id
  //   baud_rate: 115200
  //   tx_buffer_size: 512
  //   deassert_rts_dtr: false
  //   task_log_buffer_size: 768
  //   hardware_uart: UART0
  //   logs: {}
  logger_logger_id = new logger::Logger(115200, 512);
  logger_logger_id->create_pthread_key();
  logger_logger_id->init_log_buffer(768);
  logger_logger_id->set_log_level(ESPHOME_LOG_LEVEL_INFO);
  logger_logger_id->set_uart_selection(logger::UART_SELECTION_UART0);
  logger_logger_id->pre_setup();
  logger_logger_id->set_component_source(LOG_STR("logger"));
  App.register_component(logger_logger_id);
  // status_led:
  //   pin:
  //     number: 2
  //     inverted: true
  //     mode:
  //       output: true
  //       input: false
  //       open_drain: false
  //       pullup: false
  //       pulldown: false
  //     id: esp32_esp32internalgpiopin_id
  //     ignore_pin_validation_error: false
  //     ignore_strapping_warning: false
  //     drive_strength: 20.0
  //   id: status_led_statusled_id
  esp32_esp32internalgpiopin_id = new esp32::ESP32InternalGPIOPin();
  esp32_esp32internalgpiopin_id->set_pin(::GPIO_NUM_2);
  esp32_esp32internalgpiopin_id->set_inverted(true);
  esp32_esp32internalgpiopin_id->set_drive_strength(::GPIO_DRIVE_CAP_2);
  esp32_esp32internalgpiopin_id->set_flags(gpio::Flags::FLAG_OUTPUT);
  status_led_statusled_id = new status_led::StatusLED(esp32_esp32internalgpiopin_id);
  status_led_statusled_id->set_component_source(LOG_STR("status_led"));
  App.register_component(status_led_statusled_id);
  status_led_statusled_id->pre_setup();
  // web_server_base:
  //   id: web_server_base_webserverbase_id
  web_server_base_webserverbase_id = new web_server_base::WebServerBase();
  web_server_base_webserverbase_id->set_component_source(LOG_STR("web_server_base"));
  App.register_component(web_server_base_webserverbase_id);
  web_server_base::global_web_server_base = web_server_base_webserverbase_id;
  // captive_portal:
  //   id: captive_portal_captiveportal_id
  //   web_server_base_id: web_server_base_webserverbase_id
  captive_portal_captiveportal_id = new captive_portal::CaptivePortal(web_server_base_webserverbase_id);
  captive_portal_captiveportal_id->set_component_source(LOG_STR("captive_portal"));
  App.register_component(captive_portal_captiveportal_id);
  // wifi:
  //   ap:
  //     ssid: Trailer-Monitor-Setup
  //     password: trailer123
  //     id: wifi_wifiap_id
  //     ap_timeout: 1min
  //   id: wifi_wificomponent_id
  //   domain: .local
  //   reboot_timeout: 15min
  //   power_save_mode: LIGHT
  //   fast_connect: false
  //   passive_scan: false
  //   enable_on_boot: true
  //   networks:
  //     - ssid: !secret 'wifi_ssid'
  //       password: !secret 'wifi_password'
  //       id: wifi_wifiap_id_2
  //       priority: 0.0
  //   use_address: trailer-monitor.local
  wifi_wificomponent_id = new wifi::WiFiComponent();
  wifi_wificomponent_id->set_use_address("trailer-monitor.local");
  {
  wifi::WiFiAP wifi_wifiap_id_2 = wifi::WiFiAP();
  wifi_wifiap_id_2.set_ssid("Rocket");
  wifi_wifiap_id_2.set_password("Ed1nburgh2015!");
  wifi_wifiap_id_2.set_priority(0.0f);
  wifi_wificomponent_id->add_sta(wifi_wifiap_id_2);
  }
  {
  wifi::WiFiAP wifi_wifiap_id = wifi::WiFiAP();
  wifi_wifiap_id.set_ssid("Trailer-Monitor-Setup");
  wifi_wifiap_id.set_password("trailer123");
  wifi_wificomponent_id->set_ap(wifi_wifiap_id);
  }
  wifi_wificomponent_id->set_ap_timeout(60000);
  wifi_wificomponent_id->set_reboot_timeout(900000);
  wifi_wificomponent_id->set_power_save_mode(wifi::WIFI_POWER_SAVE_LIGHT);
  wifi_wificomponent_id->set_fast_connect(false);
  wifi_wificomponent_id->set_passive_scan(false);
  wifi_wificomponent_id->set_enable_on_boot(true);
  wifi_wificomponent_id->set_component_source(LOG_STR("wifi"));
  App.register_component(wifi_wificomponent_id);
  // mdns:
  //   id: mdns_mdnscomponent_id
  //   disabled: false
  //   services: []
  mdns_mdnscomponent_id = new mdns::MDNSComponent();
  mdns_mdnscomponent_id->set_component_source(LOG_STR("mdns"));
  App.register_component(mdns_mdnscomponent_id);
  // ota:
  // ota.esphome:
  //   platform: esphome
  //   password: TrailerMonitor2025
  //   id: esphome_esphomeotacomponent_id
  //   version: 2
  //   port: 3232
  esphome_esphomeotacomponent_id = new esphome::ESPHomeOTAComponent();
  esphome_esphomeotacomponent_id->set_port(3232);
  esphome_esphomeotacomponent_id->set_auth_password("TrailerMonitor2025");
  esphome_esphomeotacomponent_id->set_component_source(LOG_STR("esphome.ota"));
  App.register_component(esphome_esphomeotacomponent_id);
  // ota.web_server:
  //   platform: web_server
  //   id: web_server_webserverotacomponent_id
  web_server_webserverotacomponent_id = new web_server::WebServerOTAComponent();
  // safe_mode:
  //   id: safe_mode_safemodecomponent_id
  //   boot_is_good_after: 1min
  //   disabled: false
  //   num_attempts: 10
  //   reboot_timeout: 5min
  safe_mode_safemodecomponent_id = new safe_mode::SafeModeComponent();
  safe_mode_safemodecomponent_id->set_component_source(LOG_STR("safe_mode"));
  App.register_component(safe_mode_safemodecomponent_id);
  if (safe_mode_safemodecomponent_id->should_enter_safe_mode(10, 300000, 60000)) return;
  web_server_webserverotacomponent_id->set_component_source(LOG_STR("web_server.ota"));
  App.register_component(web_server_webserverotacomponent_id);
  // api:
  //   id: api_apiserver_id
  //   port: 6053
  //   password: ''
  //   reboot_timeout: 15min
  //   batch_delay: 100ms
  //   custom_services: false
  //   homeassistant_services: false
  //   homeassistant_states: false
  api_apiserver_id = new api::APIServer();
  api_apiserver_id->set_component_source(LOG_STR("api"));
  App.register_component(api_apiserver_id);
  api_apiserver_id->set_port(6053);
  api_apiserver_id->set_reboot_timeout(900000);
  api_apiserver_id->set_batch_delay(100);
  // web_server:
  //   port: 80
  //   id: web_server_webserver_id
  //   version: 2
  //   enable_private_network_access: true
  //   web_server_base_id: web_server_base_webserverbase_id
  //   include_internal: false
  //   log: true
  //   css_url: ''
  //   js_url: https:oi.esphome.io/v2/www.js
  web_server_webserver_id = new web_server::WebServer(web_server_base_webserverbase_id);
  web_server_webserver_id->set_component_source(LOG_STR("web_server"));
  App.register_component(web_server_webserver_id);
  web_server_base_webserverbase_id->set_port(80);
  web_server_webserver_id->set_expose_log(true);
  web_server_webserver_id->set_include_internal(false);
  // json:
  //   {}
  // esp32:
  //   board: esp32dev
  //   framework:
  //     version: 3.2.1
  //     advanced:
  //       ignore_efuse_custom_mac: false
  //     source: pioarduino/framework-arduinoespressif32@https:github.com/espressif/arduino-esp32/releases/download/3.2.1/esp32-3.2.1.zip
  //     platform_version: https:github.com/pioarduino/platform-espressif32/releases/download/54.03.21-2/platform-espressif32.zip
  //     type: arduino
  //   flash_size: 4MB
  //   variant: ESP32
  //   cpu_frequency: 160MHZ
  setCpuFrequencyMhz(160);
  // esp32_ble_tracker:
  //   scan_parameters:
  //     active: false
  //     interval: 320ms
  //     window: 30ms
  //     duration: 5min
  //     continuous: true
  //   id: esp32_ble_tracker_esp32bletracker_id
  //   ble_id: esp32_ble_esp32ble_id
  //   max_connections: 3
  //   software_coexistence: true
  esp32_ble_tracker_esp32bletracker_id = new esp32_ble_tracker::ESP32BLETracker();
  esp32_ble_tracker_esp32bletracker_id->set_component_source(LOG_STR("esp32_ble_tracker"));
  App.register_component(esp32_ble_tracker_esp32bletracker_id);
  // external_components:
  //   - source:
  //       url: https:github.com/Fabian-Schmidt/esphome-victron_ble.git
  //       type: git
  //     refresh: 1d
  //     components: all
  // victron_ble:
  //   id: shunt_device
  //   mac_address: C0:3B:98:39:E6:FE
  //   bindkey: !secret 'shunt_bindkey'
  //   esp32_ble_id: esp32_ble_tracker_esp32bletracker_id
  shunt_device = new victron_ble::VictronBle();
  shunt_device->set_component_source(LOG_STR("victron_ble"));
  App.register_component(shunt_device);
  esp32_ble_tracker_esp32bletracker_id->register_listener(shunt_device);
  shunt_device->set_address(0xC03B9839E6FEULL);
  shunt_device->set_bindkey({0x8E, 0x85, 0x27, 0x35, 0x57, 0x31, 0x4E, 0x1E, 0xB8, 0x3A, 0x94, 0x84, 0x3D, 0x7C, 0x62, 0x65});
  // victron_ble:
  //   id: solar_device
  //   mac_address: E8:86:01:5D:79:38
  //   bindkey: !secret 'solar_bindkey'
  //   esp32_ble_id: esp32_ble_tracker_esp32bletracker_id
  solar_device = new victron_ble::VictronBle();
  solar_device->set_component_source(LOG_STR("victron_ble"));
  App.register_component(solar_device);
  esp32_ble_tracker_esp32bletracker_id->register_listener(solar_device);
  solar_device->set_address(0xE886015D7938ULL);
  solar_device->set_bindkey({0xD0, 0x46, 0x52, 0xF1, 0x0B, 0x4C, 0x5A, 0xD5, 0x06, 0x6E, 0x34, 0xE3, 0x32, 0xAF, 0x69, 0x19});
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: shunt_device
  //   type: BATTERY_VOLTAGE
  //   name: Battery Voltage
  //   id: battery_voltage
  //   accuracy_decimals: 2
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: V
  //   icon: mdi:battery
  //   device_class: voltage
  battery_voltage = new victron_ble::VictronSensor(shunt_device, victron_ble::VICTRON_SENSOR_TYPE::BATTERY_VOLTAGE);
  App.register_sensor(battery_voltage);
  battery_voltage->set_name("Battery Voltage");
  battery_voltage->set_object_id("battery_voltage");
  battery_voltage->set_disabled_by_default(false);
  battery_voltage->set_icon("mdi:battery");
  battery_voltage->set_device_class("voltage");
  battery_voltage->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  battery_voltage->set_unit_of_measurement("V");
  battery_voltage->set_accuracy_decimals(2);
  battery_voltage->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: shunt_device
  //   type: BATTERY_CURRENT
  //   name: Battery Current
  //   id: battery_current
  //   accuracy_decimals: 2
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: A
  //   icon: mdi:battery
  //   device_class: current
  battery_current = new victron_ble::VictronSensor(shunt_device, victron_ble::VICTRON_SENSOR_TYPE::BATTERY_CURRENT);
  App.register_sensor(battery_current);
  battery_current->set_name("Battery Current");
  battery_current->set_object_id("battery_current");
  battery_current->set_disabled_by_default(false);
  battery_current->set_icon("mdi:battery");
  battery_current->set_device_class("current");
  battery_current->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  battery_current->set_unit_of_measurement("A");
  battery_current->set_accuracy_decimals(2);
  battery_current->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: shunt_device
  //   type: CONSUMED_AH
  //   name: Consumed Ah
  //   id: consumed_ah
  //   accuracy_decimals: 1
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: Ah
  //   icon: mdi:battery
  consumed_ah = new victron_ble::VictronSensor(shunt_device, victron_ble::VICTRON_SENSOR_TYPE::CONSUMED_AH);
  App.register_sensor(consumed_ah);
  consumed_ah->set_name("Consumed Ah");
  consumed_ah->set_object_id("consumed_ah");
  consumed_ah->set_disabled_by_default(false);
  consumed_ah->set_icon("mdi:battery");
  consumed_ah->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  consumed_ah->set_unit_of_measurement("Ah");
  consumed_ah->set_accuracy_decimals(1);
  consumed_ah->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: shunt_device
  //   type: STATE_OF_CHARGE
  //   name: State of Charge
  //   id: state_of_charge
  //   accuracy_decimals: 0
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: '%'
  //   icon: mdi:battery
  //   device_class: battery
  state_of_charge = new victron_ble::VictronSensor(shunt_device, victron_ble::VICTRON_SENSOR_TYPE::STATE_OF_CHARGE);
  App.register_sensor(state_of_charge);
  state_of_charge->set_name("State of Charge");
  state_of_charge->set_object_id("state_of_charge");
  state_of_charge->set_disabled_by_default(false);
  state_of_charge->set_icon("mdi:battery");
  state_of_charge->set_device_class("battery");
  state_of_charge->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  state_of_charge->set_unit_of_measurement("%");
  state_of_charge->set_accuracy_decimals(0);
  state_of_charge->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: shunt_device
  //   type: TIME_TO_GO
  //   name: Time to Go
  //   id: time_to_go
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: min
  //   icon: mdi:battery
  //   accuracy_decimals: 0
  //   device_class: duration
  time_to_go = new victron_ble::VictronSensor(shunt_device, victron_ble::VICTRON_SENSOR_TYPE::TIME_TO_GO);
  App.register_sensor(time_to_go);
  time_to_go->set_name("Time to Go");
  time_to_go->set_object_id("time_to_go");
  time_to_go->set_disabled_by_default(false);
  time_to_go->set_icon("mdi:battery");
  time_to_go->set_device_class("duration");
  time_to_go->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  time_to_go->set_unit_of_measurement("min");
  time_to_go->set_accuracy_decimals(0);
  time_to_go->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: solar_device
  //   type: BATTERY_VOLTAGE
  //   name: Solar Battery Voltage
  //   id: solar_battery_voltage
  //   accuracy_decimals: 2
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: V
  //   icon: mdi:battery
  //   device_class: voltage
  solar_battery_voltage = new victron_ble::VictronSensor(solar_device, victron_ble::VICTRON_SENSOR_TYPE::BATTERY_VOLTAGE);
  App.register_sensor(solar_battery_voltage);
  solar_battery_voltage->set_name("Solar Battery Voltage");
  solar_battery_voltage->set_object_id("solar_battery_voltage");
  solar_battery_voltage->set_disabled_by_default(false);
  solar_battery_voltage->set_icon("mdi:battery");
  solar_battery_voltage->set_device_class("voltage");
  solar_battery_voltage->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  solar_battery_voltage->set_unit_of_measurement("V");
  solar_battery_voltage->set_accuracy_decimals(2);
  solar_battery_voltage->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: solar_device
  //   type: BATTERY_CURRENT
  //   name: Solar Output Current
  //   id: solar_output_current
  //   accuracy_decimals: 2
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: A
  //   icon: mdi:battery
  //   device_class: current
  solar_output_current = new victron_ble::VictronSensor(solar_device, victron_ble::VICTRON_SENSOR_TYPE::BATTERY_CURRENT);
  App.register_sensor(solar_output_current);
  solar_output_current->set_name("Solar Output Current");
  solar_output_current->set_object_id("solar_output_current");
  solar_output_current->set_disabled_by_default(false);
  solar_output_current->set_icon("mdi:battery");
  solar_output_current->set_device_class("current");
  solar_output_current->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  solar_output_current->set_unit_of_measurement("A");
  solar_output_current->set_accuracy_decimals(2);
  solar_output_current->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: solar_device
  //   type: PV_POWER
  //   name: Solar PV Power
  //   id: solar_pv_power
  //   accuracy_decimals: 0
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: measurement
  //   unit_of_measurement: W
  //   icon: mdi:power
  //   device_class: power
  solar_pv_power = new victron_ble::VictronSensor(solar_device, victron_ble::VICTRON_SENSOR_TYPE::PV_POWER);
  App.register_sensor(solar_pv_power);
  solar_pv_power->set_name("Solar PV Power");
  solar_pv_power->set_object_id("solar_pv_power");
  solar_pv_power->set_disabled_by_default(false);
  solar_pv_power->set_icon("mdi:power");
  solar_pv_power->set_device_class("power");
  solar_pv_power->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  solar_pv_power->set_unit_of_measurement("W");
  solar_pv_power->set_accuracy_decimals(0);
  solar_pv_power->set_force_update(false);
  // sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: solar_device
  //   type: YIELD_TODAY
  //   name: Solar Yield Today
  //   id: solar_yield_today
  //   accuracy_decimals: 0
  //   disabled_by_default: false
  //   force_update: false
  //   state_class: total_increasing
  //   unit_of_measurement: kWh
  //   icon: mdi:power
  //   device_class: energy
  solar_yield_today = new victron_ble::VictronSensor(solar_device, victron_ble::VICTRON_SENSOR_TYPE::YIELD_TODAY);
  App.register_sensor(solar_yield_today);
  solar_yield_today->set_name("Solar Yield Today");
  solar_yield_today->set_object_id("solar_yield_today");
  solar_yield_today->set_disabled_by_default(false);
  solar_yield_today->set_icon("mdi:power");
  solar_yield_today->set_device_class("energy");
  solar_yield_today->set_state_class(sensor::STATE_CLASS_TOTAL_INCREASING);
  solar_yield_today->set_unit_of_measurement("kWh");
  solar_yield_today->set_accuracy_decimals(0);
  solar_yield_today->set_force_update(false);
  // sensor.uptime:
  //   platform: uptime
  //   name: Uptime
  //   update_interval: 60s
  //   disabled_by_default: false
  //   force_update: false
  //   id: uptime_uptimesecondssensor_id
  //   unit_of_measurement: s
  //   icon: mdi:timer-outline
  //   accuracy_decimals: 0
  //   device_class: duration
  //   state_class: total_increasing
  //   entity_category: diagnostic
  //   type: seconds
  uptime_uptimesecondssensor_id = new uptime::UptimeSecondsSensor();
  App.register_sensor(uptime_uptimesecondssensor_id);
  uptime_uptimesecondssensor_id->set_name("Uptime");
  uptime_uptimesecondssensor_id->set_object_id("uptime");
  uptime_uptimesecondssensor_id->set_disabled_by_default(false);
  uptime_uptimesecondssensor_id->set_icon("mdi:timer-outline");
  uptime_uptimesecondssensor_id->set_entity_category(::ENTITY_CATEGORY_DIAGNOSTIC);
  uptime_uptimesecondssensor_id->set_device_class("duration");
  uptime_uptimesecondssensor_id->set_state_class(sensor::STATE_CLASS_TOTAL_INCREASING);
  uptime_uptimesecondssensor_id->set_unit_of_measurement("s");
  uptime_uptimesecondssensor_id->set_accuracy_decimals(0);
  uptime_uptimesecondssensor_id->set_force_update(false);
  uptime_uptimesecondssensor_id->set_update_interval(60000);
  uptime_uptimesecondssensor_id->set_component_source(LOG_STR("uptime.sensor"));
  App.register_component(uptime_uptimesecondssensor_id);
  // sensor.wifi_signal:
  //   platform: wifi_signal
  //   name: WiFi Signal
  //   update_interval: 60s
  //   disabled_by_default: false
  //   force_update: false
  //   id: wifi_signal_wifisignalsensor_id
  //   unit_of_measurement: dBm
  //   accuracy_decimals: 0
  //   device_class: signal_strength
  //   state_class: measurement
  //   entity_category: diagnostic
  wifi_signal_wifisignalsensor_id = new wifi_signal::WiFiSignalSensor();
  App.register_sensor(wifi_signal_wifisignalsensor_id);
  wifi_signal_wifisignalsensor_id->set_name("WiFi Signal");
  wifi_signal_wifisignalsensor_id->set_object_id("wifi_signal");
  wifi_signal_wifisignalsensor_id->set_disabled_by_default(false);
  wifi_signal_wifisignalsensor_id->set_entity_category(::ENTITY_CATEGORY_DIAGNOSTIC);
  wifi_signal_wifisignalsensor_id->set_device_class("signal_strength");
  wifi_signal_wifisignalsensor_id->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  wifi_signal_wifisignalsensor_id->set_unit_of_measurement("dBm");
  wifi_signal_wifisignalsensor_id->set_accuracy_decimals(0);
  wifi_signal_wifisignalsensor_id->set_force_update(false);
  wifi_signal_wifisignalsensor_id->set_update_interval(60000);
  wifi_signal_wifisignalsensor_id->set_component_source(LOG_STR("wifi_signal.sensor"));
  App.register_component(wifi_signal_wifisignalsensor_id);
  // text_sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: solar_device
  //   type: DEVICE_STATE
  //   name: Solar Charger State
  //   id: solar_state
  //   disabled_by_default: false
  solar_state = new victron_ble::VictronTextSensor(solar_device, victron_ble::VICTRON_TEXT_SENSOR_TYPE::DEVICE_STATE);
  App.register_text_sensor(solar_state);
  solar_state->set_name("Solar Charger State");
  solar_state->set_object_id("solar_charger_state");
  solar_state->set_disabled_by_default(false);
  // text_sensor.template:
  //   platform: template
  //   name: System Status
  //   id: system_status
  //   update_interval: 30s
  //   lambda: !lambda |-
  //     return {"Running"};
  //   disabled_by_default: false
  system_status = new template_::TemplateTextSensor();
  App.register_text_sensor(system_status);
  system_status->set_name("System Status");
  system_status->set_object_id("system_status");
  system_status->set_disabled_by_default(false);
  system_status->set_update_interval(30000);
  system_status->set_component_source(LOG_STR("template.text_sensor"));
  App.register_component(system_status);
  system_status->set_template([=]() -> esphome::optional<std::string> {
      #line 160 "trailer_monitor.yaml"
      return {"Running"};
  });
  // binary_sensor.victron_ble:
  //   platform: victron_ble
  //   victron_ble_id: solar_device
  //   type: DEVICE_STATE_LOAD_DETECT
  //   name: Solar Load Detected
  //   disabled_by_default: false
  //   id: victron_ble_victronbinarysensor_id
  victron_ble_victronbinarysensor_id = new victron_ble::VictronBinarySensor(solar_device, victron_ble::VICTRON_BINARY_SENSOR_TYPE::DEVICE_STATE_LOAD_DETECT);
  App.register_binary_sensor(victron_ble_victronbinarysensor_id);
  victron_ble_victronbinarysensor_id->set_name("Solar Load Detected");
  victron_ble_victronbinarysensor_id->set_object_id("solar_load_detected");
  victron_ble_victronbinarysensor_id->set_disabled_by_default(false);
  victron_ble_victronbinarysensor_id->set_trigger_on_initial_state(false);
  // switch.restart:
  //   platform: restart
  //   name: Restart
  //   disabled_by_default: false
  //   restore_mode: ALWAYS_OFF
  //   id: restart_restartswitch_id
  //   entity_category: config
  //   icon: mdi:restart
  restart_restartswitch_id = new restart::RestartSwitch();
  App.register_switch(restart_restartswitch_id);
  restart_restartswitch_id->set_name("Restart");
  restart_restartswitch_id->set_object_id("restart");
  restart_restartswitch_id->set_disabled_by_default(false);
  restart_restartswitch_id->set_icon("mdi:restart");
  restart_restartswitch_id->set_entity_category(::ENTITY_CATEGORY_CONFIG);
  restart_restartswitch_id->set_restore_mode(switch_::SWITCH_ALWAYS_OFF);
  restart_restartswitch_id->set_component_source(LOG_STR("restart.switch"));
  App.register_component(restart_restartswitch_id);
  // interval:
  //   - interval: 30s
  //     then:
  //       - logger.log:
  //           format: 'Battery: %.2fV/%.2fA (%.0f%%) | Solar: %.0fW/%.2fA | State: %s'
  //           args:
  //             - !lambda |-
  //               id(battery_voltage).state
  //             - !lambda |-
  //               id(battery_current).state
  //             - !lambda |-
  //               id(state_of_charge).state
  //             - !lambda |-
  //               id(solar_pv_power).state
  //             - !lambda |-
  //               id(solar_output_current).state
  //             - !lambda |-
  //               id(solar_state).state.c_str()
  //           level: INFO
  //           logger_id: logger_logger_id
  //           tag: main
  //         type_id: lambdaaction_id
  //     trigger_id: trigger_id
  //     automation_id: automation_id
  //     id: interval_intervaltrigger_id
  //     startup_delay: 0s
  interval_intervaltrigger_id = new interval::IntervalTrigger();
  interval_intervaltrigger_id->set_component_source(LOG_STR("interval"));
  App.register_component(interval_intervaltrigger_id);
  automation_id = new Automation<>(interval_intervaltrigger_id);
  lambdaaction_id = new LambdaAction<>([=]() -> void {
      ESP_LOGI("main", "Battery: %.2fV/%.2fA (%.0f%%) | Solar: %.0fW/%.2fA | State: %s", battery_voltage->state, battery_current->state, state_of_charge->state, solar_pv_power->state, solar_output_current->state, solar_state->state.c_str());
  });
  automation_id->add_actions({lambdaaction_id});
  interval_intervaltrigger_id->set_update_interval(30000);
  interval_intervaltrigger_id->set_startup_delay(0);
  // preferences:
  //   id: preferences_intervalsyncer_id
  //   flash_write_interval: 60s
  preferences_intervalsyncer_id = new preferences::IntervalSyncer();
  preferences_intervalsyncer_id->set_write_interval(60000);
  preferences_intervalsyncer_id->set_component_source(LOG_STR("preferences"));
  App.register_component(preferences_intervalsyncer_id);
  // socket:
  //   implementation: bsd_sockets
  // md5:
  // esp32_ble:
  //   id: esp32_ble_esp32ble_id
  //   io_capability: none
  //   enable_on_boot: true
  //   advertising: false
  //   advertising_cycle_time: 10s
  esp32_ble_esp32ble_id = new esp32_ble::ESP32BLE();
  esp32_ble_esp32ble_id->set_enable_on_boot(true);
  esp32_ble_esp32ble_id->set_io_capability(esp32_ble::IO_CAP_NONE);
  esp32_ble_esp32ble_id->set_advertising_cycle_time(10000);
  esp32_ble_esp32ble_id->set_component_source(LOG_STR("esp32_ble"));
  App.register_component(esp32_ble_esp32ble_id);
  esp32_ble_esp32ble_id->register_gap_event_handler(esp32_ble_tracker_esp32bletracker_id);
  esp32_ble_esp32ble_id->register_gap_scan_event_handler(esp32_ble_tracker_esp32bletracker_id);
  esp32_ble_esp32ble_id->register_gattc_event_handler(esp32_ble_tracker_esp32bletracker_id);
  esp32_ble_esp32ble_id->register_ble_status_event_handler(esp32_ble_tracker_esp32bletracker_id);
  esp32_ble_tracker_esp32bletracker_id->set_parent(esp32_ble_esp32ble_id);
  esp32_ble_tracker_esp32bletracker_id->set_scan_duration(300);
  esp32_ble_tracker_esp32bletracker_id->set_scan_interval(512);
  esp32_ble_tracker_esp32bletracker_id->set_scan_window(48);
  esp32_ble_tracker_esp32bletracker_id->set_scan_active(false);
  esp32_ble_tracker_esp32bletracker_id->set_scan_continuous(true);
  // =========== AUTO GENERATED CODE END ============
  App.setup();
}

void loop() {
  App.loop();
}
