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
using namespace sensor;
alignas(preferences::IntervalSyncer) static unsigned char preferences__preferences_intervalsyncer_id__pstorage[sizeof(preferences::IntervalSyncer)];
static preferences::IntervalSyncer *const preferences_intervalsyncer_id = reinterpret_cast<preferences::IntervalSyncer *>(preferences__preferences_intervalsyncer_id__pstorage);
using namespace i2c;
alignas(i2c::IDFI2CBus) static unsigned char i2c__bus_a__pstorage[sizeof(i2c::IDFI2CBus)];
static i2c::IDFI2CBus *const bus_a = reinterpret_cast<i2c::IDFI2CBus *>(i2c__bus_a__pstorage);
alignas(husb238_i2c::HUSB238) static unsigned char husb238_i2c__usb_pd_main__pstorage[sizeof(husb238_i2c::HUSB238)];
static husb238_i2c::HUSB238 *const usb_pd_main = reinterpret_cast<husb238_i2c::HUSB238 *>(husb238_i2c__usb_pd_main__pstorage);
alignas(husb238_i2c::OutputVoltageSensor) static unsigned char husb238_i2c__husb238_i2c_outputvoltagesensor_id__pstorage[sizeof(husb238_i2c::OutputVoltageSensor)];
static husb238_i2c::OutputVoltageSensor *const husb238_i2c_outputvoltagesensor_id = reinterpret_cast<husb238_i2c::OutputVoltageSensor *>(husb238_i2c__husb238_i2c_outputvoltagesensor_id__pstorage);
alignas(husb238_i2c::OutputCurrentSensor) static unsigned char husb238_i2c__husb238_i2c_outputcurrentsensor_id__pstorage[sizeof(husb238_i2c::OutputCurrentSensor)];
static husb238_i2c::OutputCurrentSensor *const husb238_i2c_outputcurrentsensor_id = reinterpret_cast<husb238_i2c::OutputCurrentSensor *>(husb238_i2c__husb238_i2c_outputcurrentsensor_id__pstorage);
static constexpr size_t ESPHOME_LOOPING_COMPONENT_COUNT = \
  (1 * HasLoopOverride<logger::Logger>::value) + \
  (1 * HasLoopOverride<preferences::IntervalSyncer>::value) + \
  (1 * HasLoopOverride<i2c::IDFI2CBus>::value) + \
  (1 * HasLoopOverride<husb238_i2c::HUSB238>::value);
namespace esphome {
static const char COMP_SRC_TABLE_STR_0[] PROGMEM = "logger";
static const char COMP_SRC_TABLE_STR_1[] PROGMEM = "preferences";
static const char COMP_SRC_TABLE_STR_2[] PROGMEM = "i2c";
static const char COMP_SRC_TABLE_STR_3[] PROGMEM = "husb238_i2c";
static const char *const COMP_SRC_TABLE[] PROGMEM = {COMP_SRC_TABLE_STR_0, COMP_SRC_TABLE_STR_1, COMP_SRC_TABLE_STR_2, COMP_SRC_TABLE_STR_3};
const LogString *component_source_lookup(uint8_t index) {
  if (index == 0 || index > 4) return LOG_STR("<unknown>");
  return reinterpret_cast<const LogString *>(
    progmem_read_ptr(&COMP_SRC_TABLE[index - 1]));
}
}  // namespace esphome
namespace esphome {
static const char *const ENTITY_UOM_TABLE[] PROGMEM = {"V", "mA"};
const char *entity_uom_lookup(uint8_t index) {
  if (index == 0 || index > 2) return "";
  return progmem_read_ptr(&ENTITY_UOM_TABLE[index - 1]);
}

}  // namespace esphome
// ========== AUTO GENERATED INCLUDE BLOCK END ==========="

void setup() {
  // ========== AUTO GENERATED CODE BEGIN ===========
  // logger:
  //   level: DEBUG
  //   id: logger_logger_id
  //   baud_rate: 115200
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
  logger_logger_id->set_log_level(ESPHOME_LOG_LEVEL_DEBUG);
  // esphome:
  //   name: usb-pd-controller
  //   friendly_name: USB PD Controller
  //   min_version: 2026.5.3
  //   build_path: build\usb-pd-controller
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
  App.pre_setup("usb-pd-controller", 17, "USB PD Controller", 17);
  App.looping_components_.init(ESPHOME_LOOPING_COMPONENT_COUNT);
  // sensor:
  App.register_component_(logger_logger_id, 1);
  // preferences:
  //   id: preferences_intervalsyncer_id
  //   flash_write_interval: 60s
  new(preferences_intervalsyncer_id) preferences::IntervalSyncer();
  preferences_intervalsyncer_id->set_write_interval(60000);
  App.register_component_(preferences_intervalsyncer_id, 2);
  // i2c:
  //   id: bus_a
  //   sda: 8
  //   scl: 9
  //   frequency: 100000.0
  //   sda_pullup_enabled: true
  //   scl_pullup_enabled: true
  //   scan: true
  new(bus_a) i2c::IDFI2CBus();
  App.register_component_(bus_a, 3);
  bus_a->set_sda_pin(8);
  bus_a->set_sda_pullup_enabled(true);
  bus_a->set_scl_pin(9);
  bus_a->set_scl_pullup_enabled(true);
  bus_a->set_frequency(100000);
  bus_a->set_scan(true);
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
  //       enable_lwip_dhcp_server: false
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
  //       enable_ota_rollback: false
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
  // external_components:
  //   - source:
  //       path: C:\Users\Nico\Documents\GitHub\MicroDriver-software\components\husb238_i2c\..
  //       type: local
  //     components:
  //       - husb238_i2c
  //     refresh: 1d
  // husb238_i2c:
  //   id: usb_pd_main
  //   i2c_id: bus_a
  //   address: 0x08
  //   update_interval: 1s
  //   request_voltage: 20
  //   output_voltage:
  //     name: USB PD Negotiated Voltage
  //     disabled_by_default: false
  //     force_update: false
  //     id: husb238_i2c_outputvoltagesensor_id
  //     unit_of_measurement: V
  //     accuracy_decimals: 2
  //     state_class: measurement
  //   output_current:
  //     name: USB PD Max Current
  //     disabled_by_default: false
  //     force_update: false
  //     id: husb238_i2c_outputcurrentsensor_id
  //     unit_of_measurement: mA
  //     accuracy_decimals: 0
  //     state_class: measurement
  new(usb_pd_main) husb238_i2c::HUSB238();
  usb_pd_main->set_update_interval(1000);
  App.register_component_(usb_pd_main, 4);
  usb_pd_main->set_i2c_bus(bus_a);
  usb_pd_main->set_i2c_address(0x08);
  new(husb238_i2c_outputvoltagesensor_id) husb238_i2c::OutputVoltageSensor();
  husb238_i2c_outputvoltagesensor_id->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  husb238_i2c_outputvoltagesensor_id->set_accuracy_decimals(2);
  App.register_sensor(husb238_i2c_outputvoltagesensor_id, "USB PD Negotiated Voltage", 757788800, 256);  // uom:V
  husb238_i2c_outputvoltagesensor_id->set_parent(usb_pd_main);
  usb_pd_main->set_output_voltage_sensor(husb238_i2c_outputvoltagesensor_id);
  new(husb238_i2c_outputcurrentsensor_id) husb238_i2c::OutputCurrentSensor();
  husb238_i2c_outputcurrentsensor_id->set_state_class(sensor::STATE_CLASS_MEASUREMENT);
  husb238_i2c_outputcurrentsensor_id->set_accuracy_decimals(0);
  App.register_sensor(husb238_i2c_outputcurrentsensor_id, "USB PD Max Current", 3235245381UL, 512);  // uom:mA
  husb238_i2c_outputcurrentsensor_id->set_parent(usb_pd_main);
  usb_pd_main->set_output_current_sensor(husb238_i2c_outputcurrentsensor_id);
  usb_pd_main->set_request_voltage(10);
  // =========== AUTO GENERATED CODE END ============
  App.setup();
}

void loop() {
  App.loop();
}
