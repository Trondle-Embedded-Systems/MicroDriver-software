// Auto generated code by esphome
// ========== AUTO GENERATED INCLUDE BLOCK BEGIN ===========
#include "esphome.h"
using namespace esphome;
alignas(logger::Logger) static unsigned char logger__logger_logger_id__pstorage[sizeof(logger::Logger)];
static logger::Logger *const logger_logger_id = reinterpret_cast<logger::Logger *>(logger__logger_logger_id__pstorage);
#ifndef __PICOLIBC__
using std::isnan;
#endif
using std::min;
using std::max;
#include <new>
using namespace text_sensor;
using namespace sensor;
alignas(web_server_base::WebServerBase) static unsigned char web_server_base__web_server_base_webserverbase_id__pstorage[sizeof(web_server_base::WebServerBase)];
static web_server_base::WebServerBase *const web_server_base_webserverbase_id = reinterpret_cast<web_server_base::WebServerBase *>(web_server_base__web_server_base_webserverbase_id__pstorage);
alignas(wifi::WiFiComponent) static unsigned char wifi__wifi_wificomponent_id__pstorage[sizeof(wifi::WiFiComponent)];
static wifi::WiFiComponent *const wifi_wificomponent_id = reinterpret_cast<wifi::WiFiComponent *>(wifi__wifi_wificomponent_id__pstorage);
alignas(mdns::MDNSComponent) static unsigned char mdns__mdns_mdnscomponent_id__pstorage[sizeof(mdns::MDNSComponent)];
static mdns::MDNSComponent *const mdns_mdnscomponent_id = reinterpret_cast<mdns::MDNSComponent *>(mdns__mdns_mdnscomponent_id__pstorage);
alignas(esphome::ESPHomeOTAComponent) static unsigned char esphome__esphome_esphomeotacomponent_id__pstorage[sizeof(esphome::ESPHomeOTAComponent)];
static esphome::ESPHomeOTAComponent *const esphome_esphomeotacomponent_id = reinterpret_cast<esphome::ESPHomeOTAComponent *>(esphome__esphome_esphomeotacomponent_id__pstorage);
alignas(preferences::IntervalSyncer) static unsigned char preferences__preferences_intervalsyncer_id__pstorage[sizeof(preferences::IntervalSyncer)];
static preferences::IntervalSyncer *const preferences_intervalsyncer_id = reinterpret_cast<preferences::IntervalSyncer *>(preferences__preferences_intervalsyncer_id__pstorage);
alignas(safe_mode::SafeModeComponent) static unsigned char safe_mode__safe_mode_safemodecomponent_id__pstorage[sizeof(safe_mode::SafeModeComponent)];
static safe_mode::SafeModeComponent *const safe_mode_safemodecomponent_id = reinterpret_cast<safe_mode::SafeModeComponent *>(safe_mode__safe_mode_safemodecomponent_id__pstorage);
alignas(api::APIServer) static unsigned char api__api_apiserver_id__pstorage[sizeof(api::APIServer)];
static api::APIServer *const api_apiserver_id = reinterpret_cast<api::APIServer *>(api__api_apiserver_id__pstorage);
using namespace api;
alignas(web_server::WebServer) static unsigned char web_server__web_server_webserver_id__pstorage[sizeof(web_server::WebServer)];
static web_server::WebServer *const web_server_webserver_id = reinterpret_cast<web_server::WebServer *>(web_server__web_server_webserver_id__pstorage);
constexpr uint8_t ESPHOME_WEBSERVER_INDEX_HTML[174] PROGMEM = {60, 33, 68, 79, 67, 84, 89, 80, 69, 32, 104, 116, 109, 108, 62, 60, 104, 116, 109, 108, 62, 60, 104, 101, 97, 100, 62, 60, 109, 101, 116, 97, 32, 99, 104, 97, 114, 115, 101, 116, 61, 85, 84, 70, 45, 56, 62, 60, 108, 105, 110, 107, 32, 114, 101, 108, 61, 105, 99, 111, 110, 32, 104, 114, 101, 102, 61, 100, 97, 116, 97, 58, 62, 60, 47, 104, 101, 97, 100, 62, 60, 98, 111, 100, 121, 62, 60, 101, 115, 112, 45, 97, 112, 112, 62, 60, 47, 101, 115, 112, 45, 97, 112, 112, 62, 60, 115, 99, 114, 105, 112, 116, 32, 115, 114, 99, 61, 34, 104, 116, 116, 112, 115, 58, 47, 47, 111, 105, 46, 101, 115, 112, 104, 111, 109, 101, 46, 105, 111, 47, 118, 50, 47, 119, 119, 119, 46, 106, 115, 34, 62, 60, 47, 115, 99, 114, 105, 112, 116, 62, 60, 47, 98, 111, 100, 121, 62, 60, 47, 104, 116, 109, 108, 62};
constexpr size_t ESPHOME_WEBSERVER_INDEX_HTML_SIZE = 174;
using namespace i2c;
alignas(i2c::IDFI2CBus) static unsigned char i2c__i2c_bus__pstorage[sizeof(i2c::IDFI2CBus)];
static i2c::IDFI2CBus *const i2c_bus = reinterpret_cast<i2c::IDFI2CBus *>(i2c__i2c_bus__pstorage);
using namespace json;
alignas(version::VersionTextSensor) static unsigned char version__version_versiontextsensor_id__pstorage[sizeof(version::VersionTextSensor)];
static version::VersionTextSensor *const version_versiontextsensor_id = reinterpret_cast<version::VersionTextSensor *>(version__version_versiontextsensor_id__pstorage);
using namespace output;
alignas(gpio::GPIOBinaryOutput) static unsigned char gpio__status_led__pstorage[sizeof(gpio::GPIOBinaryOutput)];
static gpio::GPIOBinaryOutput *const status_led = reinterpret_cast<gpio::GPIOBinaryOutput *>(gpio__status_led__pstorage);
alignas(esp32::ESP32InternalGPIOPin) static unsigned char esp32__esp32_esp32internalgpiopin_id__pstorage[sizeof(esp32::ESP32InternalGPIOPin)];
static esp32::ESP32InternalGPIOPin *const esp32_esp32internalgpiopin_id = reinterpret_cast<esp32::ESP32InternalGPIOPin *>(esp32__esp32_esp32internalgpiopin_id__pstorage);
alignas(husb238_i2c::HUSB238) static unsigned char husb238_i2c__usb_pd__pstorage[sizeof(husb238_i2c::HUSB238)];
static husb238_i2c::HUSB238 *const usb_pd = reinterpret_cast<husb238_i2c::HUSB238 *>(husb238_i2c__usb_pd__pstorage);
alignas(husb238_i2c::OutputVoltageSensor) static unsigned char husb238_i2c__husb238_i2c_outputvoltagesensor_id__pstorage[sizeof(husb238_i2c::OutputVoltageSensor)];
static husb238_i2c::OutputVoltageSensor *const husb238_i2c_outputvoltagesensor_id = reinterpret_cast<husb238_i2c::OutputVoltageSensor *>(husb238_i2c__husb238_i2c_outputvoltagesensor_id__pstorage);
static constexpr size_t ESPHOME_LOOPING_COMPONENT_COUNT = \
  (1 * HasLoopOverride<logger::Logger>::value) + \
  (1 * HasLoopOverride<wifi::WiFiComponent>::value) + \
  (1 * HasLoopOverride<mdns::MDNSComponent>::value) + \
  (1 * HasLoopOverride<esphome::ESPHomeOTAComponent>::value) + \
  (1 * HasLoopOverride<preferences::IntervalSyncer>::value) + \
  (1 * HasLoopOverride<safe_mode::SafeModeComponent>::value) + \
  (1 * HasLoopOverride<api::APIServer>::value) + \
  (1 * HasLoopOverride<web_server::WebServer>::value) + \
  (1 * HasLoopOverride<i2c::IDFI2CBus>::value) + \
  (1 * HasLoopOverride<version::VersionTextSensor>::value) + \
  (1 * HasLoopOverride<gpio::GPIOBinaryOutput>::value) + \
  (1 * HasLoopOverride<husb238_i2c::HUSB238>::value);
namespace esphome {
static const char COMP_SRC_TABLE_STR_0[] PROGMEM = "logger";
static const char COMP_SRC_TABLE_STR_1[] PROGMEM = "wifi";
static const char COMP_SRC_TABLE_STR_2[] PROGMEM = "mdns";
static const char COMP_SRC_TABLE_STR_3[] PROGMEM = "esphome.ota";
static const char COMP_SRC_TABLE_STR_4[] PROGMEM = "preferences";
static const char COMP_SRC_TABLE_STR_5[] PROGMEM = "safe_mode";
static const char COMP_SRC_TABLE_STR_6[] PROGMEM = "api";
static const char COMP_SRC_TABLE_STR_7[] PROGMEM = "web_server";
static const char COMP_SRC_TABLE_STR_8[] PROGMEM = "i2c";
static const char COMP_SRC_TABLE_STR_9[] PROGMEM = "version.text_sensor";
static const char COMP_SRC_TABLE_STR_10[] PROGMEM = "gpio.output";
static const char COMP_SRC_TABLE_STR_11[] PROGMEM = "husb238_i2c";
static const char *const COMP_SRC_TABLE[] PROGMEM = {COMP_SRC_TABLE_STR_0, COMP_SRC_TABLE_STR_1, COMP_SRC_TABLE_STR_2, COMP_SRC_TABLE_STR_3, COMP_SRC_TABLE_STR_4, COMP_SRC_TABLE_STR_5, COMP_SRC_TABLE_STR_6, COMP_SRC_TABLE_STR_7, COMP_SRC_TABLE_STR_8, COMP_SRC_TABLE_STR_9, COMP_SRC_TABLE_STR_10, COMP_SRC_TABLE_STR_11};
const LogString *component_source_lookup(uint8_t index) {
  if (index == 0 || index > 12) return LOG_STR("<unknown>");
  return reinterpret_cast<const LogString *>(
    progmem_read_ptr(&COMP_SRC_TABLE[index - 1]));
}
}  // namespace esphome
namespace esphome {
static const char *const ENTITY_UOM_TABLE[] PROGMEM = {"V"};
const char *entity_uom_lookup(uint8_t index) {
  if (index == 0 || index > 1) return "";
  return progmem_read_ptr(&ENTITY_UOM_TABLE[index - 1]);
}

static const char ENTITY_ICON_TABLE_STR_0[] PROGMEM = "mdi:new-box";
static const char ENTITY_ICON_TABLE_EMPTY[] PROGMEM = "";
static const char *const ENTITY_ICON_TABLE[] PROGMEM = {ENTITY_ICON_TABLE_STR_0};
const char *entity_icon_lookup(uint8_t index) {
  if (index == 0 || index > 1) return ENTITY_ICON_TABLE_EMPTY;
  return progmem_read_ptr(&ENTITY_ICON_TABLE[index - 1]);
}

}  // namespace esphome
// ========== AUTO GENERATED INCLUDE BLOCK END ==========="

void setup() {
  // ========== AUTO GENERATED CODE BEGIN ===========
  // logger:
  //   level: INFO
  //   baud_rate: 115200
  //   id: logger_logger_id
  //   tx_buffer_size: 512
  //   deassert_rts_dtr: false
  //   task_log_buffer_size: 768
  //   hardware_uart: USB_SERIAL_JTAG
  //   logs: {}
  //   runtime_tag_levels: false
  new(logger_logger_id) logger::Logger(115200);
  logger_logger_id->create_pthread_key();
  logger_logger_id->set_uart_selection(logger::UART_SELECTION_USB_SERIAL_JTAG);
  logger_logger_id->pre_setup();
  logger_logger_id->set_log_level(ESPHOME_LOG_LEVEL_INFO);
  // network:
  //   enable_ipv6: false
  //   min_ipv6_addr_count: 0
  // esphome:
  //   name: microdriver
  //   friendly_name: MicroDriver Controller
  //   min_version: 2026.5.3
  //   build_path: build\microdriver
  //   platformio_options: {}
  //   environment_variables: {}
  //   includes: []
  //   includes_c: []
  //   libraries: []
  //   name_add_mac_suffix: false
  //   debug_scheduler: false
  //   areas: []
  //   devices: []
  new (&App) Application();
  App.pre_setup("microdriver", 11, "MicroDriver Controller", 22);
  App.looping_components_.init(ESPHOME_LOOPING_COMPONENT_COUNT);
  // text_sensor:
  // sensor:
  App.register_component_(logger_logger_id, 1);
  // web_server_base:
  //   id: web_server_base_webserverbase_id
  new(web_server_base_webserverbase_id) web_server_base::WebServerBase();
  web_server_base::global_web_server_base = web_server_base_webserverbase_id;
  // wifi:
  //   ap:
  //     ssid: microdriver_AP
  //     password: !secret 'ap_password'
  //     id: wifi_wifiap_id
  //     ap_timeout: 90s
  //   id: wifi_wificomponent_id
  //   domain: .local
  //   reboot_timeout: 15min
  //   power_save_mode: LIGHT
  //   fast_connect: false
  //   enable_btm: false
  //   enable_rrm: false
  //   passive_scan: false
  //   enable_on_boot: true
  //   post_connect_roaming: true
  //   min_auth_mode: WPA2
  //   networks:
  //     - ssid: !secret 'wifi_ssid'
  //       password: !secret 'wifi_password'
  //       id: wifi_wifiap_id_2
  //       priority: 0
  //   use_address: microdriver.local
  new(wifi_wificomponent_id) wifi::WiFiComponent();
  wifi_wificomponent_id->set_use_address("microdriver.local");
  wifi_wificomponent_id->init_sta(1);
  {
  wifi::WiFiAP wifi_wifiap_id_2 = wifi::WiFiAP();
  wifi_wifiap_id_2.set_ssid("tsu3");
  wifi_wifiap_id_2.set_password("greatScott$z4a");
  wifi_wifiap_id_2.set_priority(0);
  wifi_wificomponent_id->add_sta(wifi_wifiap_id_2);
  }
  {
  wifi::WiFiAP wifi_wifiap_id = wifi::WiFiAP();
  wifi_wifiap_id.set_ssid("microdriver_AP");
  wifi_wifiap_id.set_password("HNA.axk0pue5acq7bmk");
  wifi_wificomponent_id->set_ap(wifi_wifiap_id);
  }
  wifi_wificomponent_id->set_ap_timeout(90000);
  wifi_wificomponent_id->set_reboot_timeout(900000);
  wifi_wificomponent_id->set_power_save_mode(wifi::WIFI_POWER_SAVE_LIGHT);
  wifi_wificomponent_id->set_min_auth_mode(wifi::WIFI_MIN_AUTH_MODE_WPA2);
  App.register_component_(wifi_wificomponent_id, 2);
  // mdns:
  //   id: mdns_mdnscomponent_id
  //   disabled: false
  //   services: []
  new(mdns_mdnscomponent_id) mdns::MDNSComponent();
  App.register_component_(mdns_mdnscomponent_id, 3);
  // ota:
  // ota.esphome:
  //   platform: esphome
  //   password: !secret 'ota_password'
  //   id: esphome_esphomeotacomponent_id
  //   version: 2
  //   port: 3232
  //   allow_partition_access: false
  new(esphome_esphomeotacomponent_id) esphome::ESPHomeOTAComponent();
  esphome_esphomeotacomponent_id->set_port(3232);
  esphome_esphomeotacomponent_id->set_auth_password("juj-kaz-htq0cxd1HAE");
  App.register_component_(esphome_esphomeotacomponent_id, 4);
  // preferences:
  //   id: preferences_intervalsyncer_id
  //   flash_write_interval: 60s
  new(preferences_intervalsyncer_id) preferences::IntervalSyncer();
  preferences_intervalsyncer_id->set_write_interval(60000);
  App.register_component_(preferences_intervalsyncer_id, 5);
  // safe_mode:
  //   id: safe_mode_safemodecomponent_id
  //   boot_is_good_after: 1min
  //   disabled: false
  //   num_attempts: 10
  //   reboot_timeout: 5min
  new(safe_mode_safemodecomponent_id) safe_mode::SafeModeComponent();
  App.register_component_(safe_mode_safemodecomponent_id, 6);
  if (safe_mode_safemodecomponent_id->should_enter_safe_mode(10, 300000, 60000)) return;
  // api:
  //   encryption:
  //     key: !secret 'api_key'
  //   id: api_apiserver_id
  //   port: 6053
  //   reboot_timeout: 15min
  //   batch_delay: 100ms
  //   custom_services: false
  //   homeassistant_services: false
  //   homeassistant_states: false
  //   listen_backlog: 4
  //   max_connections: 5
  //   max_send_queue: 8
  new(api_apiserver_id) api::APIServer();
  App.register_component_(api_apiserver_id, 7);
  api_apiserver_id->set_port(6053);
  api_apiserver_id->set_reboot_timeout(900000);
  api_apiserver_id->set_batch_delay(100);
  api_apiserver_id->set_listen_backlog(4);
  api_apiserver_id->set_noise_psk({163, 177, 176, 226, 237, 235, 206, 123, 159, 168, 184, 70, 116, 158, 252, 65, 166, 139, 95, 186, 100, 92, 6, 243, 52, 29, 53, 71, 123, 175, 53, 241});
  // web_server:
  //   port: 80
  //   id: web_server_webserver_id
  //   version: 2
  //   enable_private_network_access: true
  //   web_server_base_id: web_server_base_webserverbase_id
  //   include_internal: false
  //   log: true
  //   compression: gzip
  //   css_url: ''
  //   js_url: https:oi.esphome.io/v2/www.js
  new(web_server_webserver_id) web_server::WebServer(web_server_base_webserverbase_id);
  App.register_component_(web_server_webserver_id, 8);
  web_server_base_webserverbase_id->set_port(80);
  web_server_webserver_id->set_expose_log(true);
  web_server_webserver_id->set_include_internal(false);
  // i2c:
  //   id: i2c_bus
  //   sda: 17
  //   scl: 21
  //   scan: true
  //   sda_pullup_enabled: true
  //   scl_pullup_enabled: true
  //   frequency: 50000.0
  new(i2c_bus) i2c::IDFI2CBus();
  App.register_component_(i2c_bus, 9);
  i2c_bus->set_sda_pin(17);
  i2c_bus->set_sda_pullup_enabled(true);
  i2c_bus->set_scl_pin(21);
  i2c_bus->set_scl_pullup_enabled(true);
  i2c_bus->set_frequency(50000);
  i2c_bus->set_scan(true);
  // json:
  //   {}
  // substitutions:
  //   device_name: microdriver
  //   friendly_name: MicroDriver Controller
  // esp32:
  //   board: esp32-s3-devkitc-1
  //   variant: ESP32S3
  //   framework:
  //     type: esp-idf
  //     version: 5.5.4
  //     sdkconfig_options: {}
  //     log_level: ERROR
  //     advanced:
  //       compiler_optimization: SIZE
  //       enable_idf_experimental_features: false
  //       enable_lwip_assert: true
  //       ignore_efuse_custom_mac: false
  //       ignore_efuse_mac_crc: false
  //       sram1_as_iram: false
  //       enable_lwip_mdns_queries: true
  //       enable_lwip_bridge_interface: false
  //       enable_lwip_tcpip_core_locking: true
  //       enable_lwip_check_thread_safety: true
  //       disable_libc_locks_in_iram: true
  //       disable_vfs_support_termios: true
  //       disable_vfs_support_select: true
  //       disable_vfs_support_dir: true
  //       freertos_in_iram: false
  //       ringbuf_in_iram: false
  //       heap_in_iram: false
  //       execute_from_psram: false
  //       loop_task_stack_size: 8192
  //       enable_ota_rollback: true
  //       use_full_certificate_bundle: false
  //       include_builtin_idf_components: []
  //       enable_full_printf: false
  //       disable_debug_stubs: true
  //       disable_ocd_aware: true
  //       disable_usb_serial_jtag_secondary: true
  //       disable_dev_null_vfs: true
  //       disable_mbedtls_peer_cert: true
  //       disable_mbedtls_pkcs7: true
  //       disable_regi2c_in_iram: true
  //       adc_oneshot_in_iram: false
  //       disable_fatfs: true
  //     components: []
  //     platform_version: https:github.com/pioarduino/platform-espressif32/releases/download/55.03.38-1/platform-espressif32.zip
  //     source: pioarduino/framework-espidf@https:github.com/pioarduino/esp-idf/releases/download/v5.5.4/esp-idf-v5.5.4.tar.xz
  //   flash_size: 4MB
  //   watchdog_timeout: 5s
  //   cpu_frequency: 240MHZ
  // text_sensor.version:
  //   platform: version
  //   name: ESPHome Version
  //   disabled_by_default: false
  //   icon: mdi:new-box
  //   entity_category: diagnostic
  //   id: version_versiontextsensor_id
  //   hide_hash: false
  //   hide_timestamp: false
  new(version_versiontextsensor_id) version::VersionTextSensor();
  App.register_text_sensor(version_versiontextsensor_id, "ESPHome Version", 2314437119UL, 134283264);  // category:diagnostic, icon:mdi:new-box
  App.register_component_(version_versiontextsensor_id, 10);
  version_versiontextsensor_id->set_hide_hash(false);
  version_versiontextsensor_id->set_hide_timestamp(false);
  // output:
  // output.gpio:
  //   platform: gpio
  //   pin:
  //     number: 18
  //     mode:
  //       output: true
  //       input: false
  //       open_drain: false
  //       pullup: false
  //       pulldown: false
  //     id: esp32_esp32internalgpiopin_id
  //     inverted: false
  //     ignore_pin_validation_error: false
  //     ignore_strapping_warning: false
  //     drive_strength: 20.0
  //   id: status_led
  //   inverted: false
  new(status_led) gpio::GPIOBinaryOutput();
  status_led->set_inverted(false);
  App.register_component_(status_led, 11);
  new(esp32_esp32internalgpiopin_id) esp32::ESP32InternalGPIOPin();
  esp32_esp32internalgpiopin_id->set_pin(::GPIO_NUM_18);
  esp32_esp32internalgpiopin_id->set_drive_strength(::GPIO_DRIVE_CAP_2);
  esp32_esp32internalgpiopin_id->set_flags(gpio::Flags::FLAG_OUTPUT);
  status_led->set_pin(esp32_esp32internalgpiopin_id);
  // husb238_i2c:
  //   id: usb_pd
  //   output_voltage:
  //     name: USB Voltage
  //     disabled_by_default: false
  //     force_update: false
  //     id: husb238_i2c_outputvoltagesensor_id
  //     unit_of_measurement: V
  //     accuracy_decimals: 2
  //     state_class: measurement
  //   pdo1_voltage: 5
  //   pdo1_current: 3
  //   update_interval: 1s
  //   i2c_id: i2c_bus
  //   address: 0x08
  new(usb_pd) husb238_i2c::HUSB238();
  usb_pd->set_update_interval(1000);
  App.register_component_(usb_pd, 12);
  usb_pd->set_i2c_bus(i2c_bus);
  usb_pd->set_i2c_address(0x08);
  new(husb238_i2c_outputvoltagesensor_id) husb238_i2c::OutputVoltageSensor();
  husb238_i2c_outputvoltagesensor_id->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  husb238_i2c_outputvoltagesensor_id->set_accuracy_decimals(2);
  App.register_sensor(husb238_i2c_outputvoltagesensor_id, "USB Voltage", 1558306530, 256);  // uom:V
  husb238_i2c_outputvoltagesensor_id->set_parent(usb_pd);
  usb_pd->set_output_voltage_sensor(husb238_i2c_outputvoltagesensor_id);
  usb_pd->set_pdo_voltage(0, 5);
  usb_pd->set_pdo_current(0, 3);
  // external_components:
  //   - source:
  //       url: https:github.com/Trondle-Embedded-Systems/MicroDriver-software.git
  //       type: git
  //     components:
  //       - tmc2209_hub
  //       - tmc2209
  //       - stepper
  //       - husb238_i2c
  //     refresh: 0s
  // md5:
  // sha256:
  //   {}
  // socket:
  //   implementation: bsd_sockets
  // web_server_idf:
  //   {}
  // =========== AUTO GENERATED CODE END ============
  App.setup();
}

void loop() {
  App.loop();
}
