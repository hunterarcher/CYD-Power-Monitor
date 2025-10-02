// Auto generated code by esphome
// ========== AUTO GENERATED INCLUDE BLOCK BEGIN ===========
#include "esphome.h"
using namespace esphome;
using std::isnan;
using std::min;
using std::max;
using namespace text_sensor;
using namespace sensor;
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
web_server::WebServer *web_server_webserver_id;
const uint8_t ESPHOME_WEBSERVER_INDEX_HTML[174] PROGMEM = {60, 33, 68, 79, 67, 84, 89, 80, 69, 32, 104, 116, 109, 108, 62, 60, 104, 116, 109, 108, 62, 60, 104, 101, 97, 100, 62, 60, 109, 101, 116, 97, 32, 99, 104, 97, 114, 115, 101, 116, 61, 85, 84, 70, 45, 56, 62, 60, 108, 105, 110, 107, 32, 114, 101, 108, 61, 105, 99, 111, 110, 32, 104, 114, 101, 102, 61, 100, 97, 116, 97, 58, 62, 60, 47, 104, 101, 97, 100, 62, 60, 98, 111, 100, 121, 62, 60, 101, 115, 112, 45, 97, 112, 112, 62, 60, 47, 101, 115, 112, 45, 97, 112, 112, 62, 60, 115, 99, 114, 105, 112, 116, 32, 115, 114, 99, 61, 34, 104, 116, 116, 112, 115, 58, 47, 47, 111, 105, 46, 101, 115, 112, 104, 111, 109, 101, 46, 105, 111, 47, 118, 50, 47, 119, 119, 119, 46, 106, 115, 34, 62, 60, 47, 115, 99, 114, 105, 112, 116, 62, 60, 47, 98, 111, 100, 121, 62, 60, 47, 104, 116, 109, 108, 62};
const size_t ESPHOME_WEBSERVER_INDEX_HTML_SIZE = 174;
using namespace json;
esp32_ble_tracker::ESP32BLETracker *esp32_ble_tracker_esp32bletracker_id;
template_::TemplateTextSensor *system_status;
uptime::UptimeSecondsSensor *uptime_uptimesecondssensor_id;
preferences::IntervalSyncer *preferences_intervalsyncer_id;
esp32_ble::ESP32BLE *esp32_ble_esp32ble_id;
esp32_ble_tracker::ESPBTAdvertiseTrigger *esp32_ble_tracker_espbtadvertisetrigger_id;
Automation<const esp32_ble_tracker::ESPBTDevice &> *automation_id;
LambdaAction<const esp32_ble_tracker::ESPBTDevice &> *lambdaaction_id;
esp32_ble_tracker::ESPBTAdvertiseTrigger *esp32_ble_tracker_espbtadvertisetrigger_id_2;
Automation<const esp32_ble_tracker::ESPBTDevice &> *automation_id_2;
LambdaAction<const esp32_ble_tracker::ESPBTDevice &> *lambdaaction_id_2;
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
  //   name: victron-simple
  //   friendly_name: Victron BLE Simple
  //   min_version: 2025.9.1
  //   build_path: build\victron-simple
  //   platformio_options: {}
  //   includes: []
  //   libraries: []
  //   name_add_mac_suffix: false
  //   debug_scheduler: false
  //   areas: []
  //   devices: []
  App.pre_setup("victron-simple", "Victron BLE Simple", "", __DATE__ ", " __TIME__, false);
  // text_sensor:
  // sensor:
  // logger:
  //   level: DEBUG
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
  logger_logger_id->set_log_level(ESPHOME_LOG_LEVEL_DEBUG);
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
  //     ssid: Victron-Simple
  //     password: '12345678'
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
  //     - ssid: Rocket
  //       password: Ed1nburgh2015!
  //       id: wifi_wifiap_id_2
  //       priority: 0.0
  //   use_address: victron-simple.local
  wifi_wificomponent_id = new wifi::WiFiComponent();
  wifi_wificomponent_id->set_use_address("victron-simple.local");
  {
  wifi::WiFiAP wifi_wifiap_id_2 = wifi::WiFiAP();
  wifi_wifiap_id_2.set_ssid("Rocket");
  wifi_wifiap_id_2.set_password("Ed1nburgh2015!");
  wifi_wifiap_id_2.set_priority(0.0f);
  wifi_wificomponent_id->add_sta(wifi_wifiap_id_2);
  }
  {
  wifi::WiFiAP wifi_wifiap_id = wifi::WiFiAP();
  wifi_wifiap_id.set_ssid("Victron-Simple");
  wifi_wifiap_id.set_password("12345678");
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
  //   password: VictronTest2025
  //   id: esphome_esphomeotacomponent_id
  //   version: 2
  //   port: 3232
  esphome_esphomeotacomponent_id = new esphome::ESPHomeOTAComponent();
  esphome_esphomeotacomponent_id->set_port(3232);
  esphome_esphomeotacomponent_id->set_auth_password("VictronTest2025");
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
  //     interval: 1100ms
  //     window: 1100ms
  //     active: true
  //     duration: 5min
  //     continuous: true
  //   on_ble_advertise:
  //     - mac_address:
  //         - C0:3B:98:39:E6:FE
  //       then:
  //         - logger.log:
  //             format: Found SHUNT device!
  //             tag: main
  //             logger_id: logger_logger_id
  //             level: DEBUG
  //             args: []
  //           type_id: lambdaaction_id
  //       automation_id: automation_id
  //       trigger_id: esp32_ble_tracker_espbtadvertisetrigger_id
  //     - mac_address:
  //         - E8:86:01:5D:79:38
  //       then:
  //         - logger.log:
  //             format: Found SOLAR device!
  //             tag: main
  //             logger_id: logger_logger_id
  //             level: DEBUG
  //             args: []
  //           type_id: lambdaaction_id_2
  //       automation_id: automation_id_2
  //       trigger_id: esp32_ble_tracker_espbtadvertisetrigger_id_2
  //   id: esp32_ble_tracker_esp32bletracker_id
  //   ble_id: esp32_ble_esp32ble_id
  //   max_connections: 3
  //   software_coexistence: true
  esp32_ble_tracker_esp32bletracker_id = new esp32_ble_tracker::ESP32BLETracker();
  esp32_ble_tracker_esp32bletracker_id->set_component_source(LOG_STR("esp32_ble_tracker"));
  App.register_component(esp32_ble_tracker_esp32bletracker_id);
  // text_sensor.template:
  //   platform: template
  //   name: System Status
  //   id: system_status
  //   update_interval: 30s
  //   lambda: !lambda |-
  //     return {"System Running"};
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
      #line 62 "victron_simple.yaml"
      return {"System Running"};
  });
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
  // preferences:
  //   id: preferences_intervalsyncer_id
  //   flash_write_interval: 60s
  preferences_intervalsyncer_id = new preferences::IntervalSyncer();
  preferences_intervalsyncer_id->set_write_interval(60000);
  preferences_intervalsyncer_id->set_component_source(LOG_STR("preferences"));
  App.register_component(preferences_intervalsyncer_id);
  // md5:
  // socket:
  //   implementation: bsd_sockets
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
  esp32_ble_tracker_esp32bletracker_id->set_scan_interval(1760);
  esp32_ble_tracker_esp32bletracker_id->set_scan_window(1760);
  esp32_ble_tracker_esp32bletracker_id->set_scan_active(true);
  esp32_ble_tracker_esp32bletracker_id->set_scan_continuous(true);
  esp32_ble_tracker_espbtadvertisetrigger_id = new esp32_ble_tracker::ESPBTAdvertiseTrigger(esp32_ble_tracker_esp32bletracker_id);
  esp32_ble_tracker_espbtadvertisetrigger_id->set_addresses({0xC03B9839E6FEULL});
  automation_id = new Automation<const esp32_ble_tracker::ESPBTDevice &>(esp32_ble_tracker_espbtadvertisetrigger_id);
  lambdaaction_id = new LambdaAction<const esp32_ble_tracker::ESPBTDevice &>([=](const esp32_ble_tracker::ESPBTDevice & x) -> void {
      ESP_LOGD("main", "Found SHUNT device!");
  });
  automation_id->add_actions({lambdaaction_id});
  esp32_ble_tracker_espbtadvertisetrigger_id_2 = new esp32_ble_tracker::ESPBTAdvertiseTrigger(esp32_ble_tracker_esp32bletracker_id);
  esp32_ble_tracker_espbtadvertisetrigger_id_2->set_addresses({0xE886015D7938ULL});
  automation_id_2 = new Automation<const esp32_ble_tracker::ESPBTDevice &>(esp32_ble_tracker_espbtadvertisetrigger_id_2);
  lambdaaction_id_2 = new LambdaAction<const esp32_ble_tracker::ESPBTDevice &>([=](const esp32_ble_tracker::ESPBTDevice & x) -> void {
      ESP_LOGD("main", "Found SOLAR device!");
  });
  automation_id_2->add_actions({lambdaaction_id_2});
  // =========== AUTO GENERATED CODE END ============
  App.setup();
}

void loop() {
  App.loop();
}
