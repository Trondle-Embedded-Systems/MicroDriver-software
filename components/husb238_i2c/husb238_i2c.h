#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace husb238_i2c {

// Forward declarations
class HUSB238;

// Sensor classes that inherit from both PollingComponent and sensor::Sensor
class OutputVoltageSensor : public PollingComponent, public sensor::Sensor, public Parented<HUSB238> {
 public:
  void update() override {}
};
class OutputCurrentSensor : public PollingComponent, public sensor::Sensor, public Parented<HUSB238> {
 public:
  void update() override {}
};
class InputVoltageSensor : public PollingComponent, public sensor::Sensor, public Parented<HUSB238> {
 public:
  void update() override {}
};

// SRC_PDO (0x08) selection codes. These live in the TOP 4 bits of the register
// and are NOT the same encoding as the negotiated-voltage codes in PD_STATUS0.
enum PdSelection : uint8_t {
  SEL_NONE = 0x0,
  SEL_5V = 0x1,
  SEL_9V = 0x2,
  SEL_12V = 0x3,
  SEL_15V = 0x8,
  SEL_18V = 0x9,
  SEL_20V = 0xA,
};

// Main controller class
class HUSB238 : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void update_status();

  // Sensor setters
  void set_output_voltage_sensor(OutputVoltageSensor *sensor) { output_voltage_sensor_ = sensor; }
  void set_output_current_sensor(OutputCurrentSensor *sensor) { output_current_sensor_ = sensor; }
  void set_input_voltage_sensor(InputVoltageSensor *sensor) { input_voltage_sensor_ = sensor; }

  // Configured target voltage. `code` is a PdSelection value; 0 (SEL_NONE)
  // disables auto-request and leaves whatever the chip negotiated on its own.
  void set_request_voltage(uint8_t code) { desired_selection_ = code; }

  // Request a voltage from the attached PD source at runtime. Returns true if
  // the I2C transaction succeeded (NOT whether the source accepted it -- that
  // shows up later in PD_STATUS0).
  bool request_voltage(uint8_t selection_code);

 private:
  // Real HUSB238 register map (Hynetek datasheet / Adafruit + SparkFun docs).
  static const uint8_t REG_PD_STATUS0 = 0x00;  // [7:4] negotiated voltage, [3:0] current
  static const uint8_t REG_PD_STATUS1 = 0x01;  // bit6 = ATTACH
  static const uint8_t REG_SRC_PDO_5V = 0x02;
  static const uint8_t REG_SRC_PDO_20V = 0x07;
  static const uint8_t REG_SRC_PDO = 0x08;      // [7:4] = requested PdSelection
  static const uint8_t REG_GO_COMMAND = 0x09;   // write 0x01 to request SRC_PDO

  static const uint8_t GO_REQUEST_PDO = 0x01;
  static const uint8_t PD_STATUS1_ATTACH = 0x40;  // bit 6

  bool read_register(uint8_t reg, uint8_t *data, size_t len);
  bool write_register(uint8_t reg, uint8_t value);

  // Decode helpers for PD_STATUS0.
  static float decode_voltage(uint8_t status0_high_nibble);
  static float decode_current(uint8_t status0_low_nibble);

  // Sensor pointers
  OutputVoltageSensor *output_voltage_sensor_{nullptr};
  OutputCurrentSensor *output_current_sensor_{nullptr};
  InputVoltageSensor *input_voltage_sensor_{nullptr};

  // Desired SRC_PDO selection code (0 = leave alone).
  uint8_t desired_selection_{SEL_NONE};

  // State tracking
  float last_output_voltage_{-1.0f};
  float last_output_current_{-1.0f};
};

}  // namespace husb238_i2c
}  // namespace esphome
