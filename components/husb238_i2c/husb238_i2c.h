#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace husb238_i2c {

class HUSB238 : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  // Sensor setters
  void set_output_voltage_sensor(sensor::Sensor *sensor) { output_voltage_sensor_ = sensor; }
  void set_output_current_sensor(sensor::Sensor *sensor) { output_current_sensor_ = sensor; }
  void set_input_voltage_sensor(sensor::Sensor *sensor) { input_voltage_sensor_ = sensor; }

  // Switch setters
  void set_enable_output_switch(switch_::Switch *switch_) { enable_output_switch_ = switch_; }

  // Number setters
  void set_selected_pdo_number(number::Number *number) { selected_pdo_number_ = number; }
  void set_select_voltage_number(number::Number *number) { select_voltage_number_ = number; }
  void set_max_current_number(number::Number *number) { max_current_number_ = number; }

  // PDO configuration
  void set_pdo_voltage(uint8_t pdo_index, uint8_t voltage) {
    if (pdo_index < 5) {
      pdo_voltages_[pdo_index] = voltage;
    }
  }

  void set_pdo_current(uint8_t pdo_index, uint8_t current) {
    if (pdo_index < 5) {
      pdo_currents_[pdo_index] = current;
    }
  }

 private:
  // I2C Register addresses (HUSB238 typical registers)
  static const uint8_t REG_CHIP_ID = 0x00;
  static const uint8_t REG_FIRMWARE_VERSION = 0x01;
  static const uint8_t REG_CONFIGURATION = 0x02;
  static const uint8_t REG_STATUS = 0x03;
  static const uint8_t REG_POWER = 0x04;
  static const uint8_t REG_VOLTAGE = 0x05;
  static const uint8_t REG_CURRENT = 0x06;
  static const uint8_t REG_AVAILABLE_PDOS = 0x07;
  static const uint8_t REG_SELECTED_PDO = 0x08;
  static const uint8_t REG_OUTPUT_ENABLE = 0x09;
  static const uint8_t REG_CABLE_PLUG = 0x0A;

  // PDO configuration register addresses
  static const uint8_t REG_PDO_BASE = 0x20;
  static const uint8_t REG_PDO_INTERVAL = 0x02;

  // Status flags
  static const uint8_t STATUS_PDO_CHANGE = 0x01;
  static const uint8_t STATUS_PLUG_IN = 0x02;
  static const uint8_t STATUS_NEGOTIATION = 0x04;

  // Internal methods
  bool read_register(uint8_t reg, uint8_t *data, size_t len);
  bool write_register(uint8_t reg, const uint8_t *data, size_t len);
  bool write_register_byte(uint8_t reg, uint8_t data);

  void update_output_voltage_();
  void update_output_current_();
  void update_input_voltage_();
  void update_pdo_status_();
  void handle_pdo_selection_(float voltage);
  void handle_enable_output_(bool enable);

  // Sensor pointers
  sensor::Sensor *output_voltage_sensor_{nullptr};
  sensor::Sensor *output_current_sensor_{nullptr};
  sensor::Sensor *input_voltage_sensor_{nullptr};

  // Switch pointers
  switch_::Switch *enable_output_switch_{nullptr};

  // Number pointers
  number::Number *selected_pdo_number_{nullptr};
  number::Number *select_voltage_number_{nullptr};
  number::Number *max_current_number_{nullptr};

  // PDO configuration arrays
  uint8_t pdo_voltages_[5] = {5, 9, 15, 20, 0};
  uint8_t pdo_currents_[5] = {3, 2, 2, 1, 0};

  // State tracking
  float last_output_voltage_{0.0f};
  float last_output_current_{0.0f};
  float last_input_voltage_{0.0f};
  uint8_t last_available_pdos_{0};
  uint8_t last_selected_pdo_{0};
};

}  // namespace husb238_i2c
}  // namespace esphome
