#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace husb238_i2c {

// Forward declarations
class HUSB238;

// Sensor classes that inherit from both PollingComponent and sensor::Sensor
class OutputVoltageSensor : public PollingComponent, public sensor::Sensor, public Parented<HUSB238> {};
class OutputCurrentSensor : public PollingComponent, public sensor::Sensor, public Parented<HUSB238> {};
class InputVoltageSensor : public PollingComponent, public sensor::Sensor, public Parented<HUSB238> {};

// Main controller class
class HUSB238 : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void update_output_voltage();
  void update_output_current();
  void update_input_voltage();

  // Sensor setters
  void set_output_voltage_sensor(OutputVoltageSensor *sensor) { output_voltage_sensor_ = sensor; }
  void set_output_current_sensor(OutputCurrentSensor *sensor) { output_current_sensor_ = sensor; }
  void set_input_voltage_sensor(InputVoltageSensor *sensor) { input_voltage_sensor_ = sensor; }

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
  static const uint8_t REG_VOLTAGE = 0x05;
  static const uint8_t REG_CURRENT = 0x06;
  static const uint8_t REG_POWER = 0x04;

  // Internal methods
  bool read_register(uint8_t reg, uint8_t *data, size_t len);

  // Sensor pointers
  OutputVoltageSensor *output_voltage_sensor_{nullptr};
  OutputCurrentSensor *output_current_sensor_{nullptr};
  InputVoltageSensor *input_voltage_sensor_{nullptr};

  // PDO configuration arrays
  uint8_t pdo_voltages_[5] = {5, 9, 15, 20, 0};
  uint8_t pdo_currents_[5] = {3, 2, 2, 1, 0};

  // State tracking
  float last_output_voltage_{0.0f};
  float last_output_current_{0.0f};
  float last_input_voltage_{0.0f};
};

}  // namespace husb238_i2c
}  // namespace esphome
