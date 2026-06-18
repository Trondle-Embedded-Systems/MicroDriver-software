#include "husb238_i2c.h"
#include "esphome/core/log.h"

namespace esphome {
namespace husb238_i2c {

static const char *TAG = "husb238.i2c";

void HUSB238::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HUSB238 USB Power Delivery sink controller...");

  // PD_STATUS0 must be readable for the device to be considered present.
  uint8_t status0;
  if (!read_register(REG_PD_STATUS0, &status0, 1)) {
    ESP_LOGE(TAG, "Failed to read PD_STATUS0 - is the HUSB238 on the bus at 0x%02X?", this->address_);
    this->mark_failed();
    return;
  }

  ESP_LOGCONFIG(TAG, "HUSB238 found. PD_STATUS0 = 0x%02X", status0);

  // The actual voltage request happens in update(), once the source is
  // attached and negotiation has settled. Requesting too early (before the
  // adapter has advertised its PDOs) just gets ignored.
}

void HUSB238::update() {
  update_status();

  if (desired_selection_ == SEL_NONE)
    return;

  // Only request if a source is attached; otherwise the write is pointless and
  // GO_COMMAND against an unattached port is what tends to wedge the bus.
  uint8_t status1;
  if (!read_register(REG_PD_STATUS1, &status1, 1))
    return;
  if (!(status1 & PD_STATUS1_ATTACH)) {
    ESP_LOGD(TAG, "No PD source attached yet; deferring request (SRC_PDO selection 0x%X)",
             desired_selection_);
    return;
  }

  // If we are already negotiated at the desired voltage, don't keep re-requesting.
  uint8_t status0;
  if (!read_register(REG_PD_STATUS0, &status0, 1))
    return;
  uint8_t negotiated = (status0 >> 4) & 0x0F;

  // Map negotiated voltage code -> selection code to compare.
  uint8_t negotiated_as_selection = SEL_NONE;
  switch (negotiated) {
    case 0x1: negotiated_as_selection = SEL_5V; break;
    case 0x2: negotiated_as_selection = SEL_9V; break;
    case 0x3: negotiated_as_selection = SEL_12V; break;
    case 0x4: negotiated_as_selection = SEL_15V; break;
    case 0x5: negotiated_as_selection = SEL_18V; break;
    case 0x6: negotiated_as_selection = SEL_20V; break;
    default: break;
  }

  if (negotiated_as_selection == desired_selection_)
    return;  // already there

  request_voltage(desired_selection_);
}

bool HUSB238::request_voltage(uint8_t selection_code) {
  ESP_LOGI(TAG, "Requesting PD voltage (SRC_PDO selection 0x%X)", selection_code);

  // SRC_PDO holds the selection in the top 4 bits.
  if (!write_register(REG_SRC_PDO, (uint8_t) (selection_code << 4))) {
    ESP_LOGW(TAG, "Failed to write SRC_PDO");
    return false;
  }

  // GO_COMMAND = 0x01 triggers the request to the source.
  if (!write_register(REG_GO_COMMAND, GO_REQUEST_PDO)) {
    ESP_LOGW(TAG, "Failed to write GO_COMMAND");
    return false;
  }

  return true;
}

void HUSB238::update_status() {
  uint8_t status0;
  if (!read_register(REG_PD_STATUS0, &status0, 1)) {
    ESP_LOGW(TAG, "Failed to read PD_STATUS0");
    return;
  }

  float voltage = decode_voltage((status0 >> 4) & 0x0F);
  float current = decode_current(status0 & 0x0F);

  if (output_voltage_sensor_ && voltage != last_output_voltage_) {
    output_voltage_sensor_->publish_state(voltage);
    last_output_voltage_ = voltage;
    ESP_LOGD(TAG, "Negotiated voltage: %.0f V", voltage);
  }
  if (output_current_sensor_ && current != last_output_current_) {
    output_current_sensor_->publish_state(current * 1000.0f);  // report in mA
    last_output_current_ = current;
    ESP_LOGD(TAG, "Max current: %.2f A", current);
  }
  // The HUSB238 has no separate input-voltage register; input == negotiated
  // VBUS, so we publish the same value if that sensor is configured.
  if (input_voltage_sensor_) {
    input_voltage_sensor_->publish_state(voltage);
  }
}

// PD_STATUS0 high nibble -> volts.
float HUSB238::decode_voltage(uint8_t code) {
  switch (code) {
    case 0x1: return 5.0f;
    case 0x2: return 9.0f;
    case 0x3: return 12.0f;
    case 0x4: return 15.0f;
    case 0x5: return 18.0f;
    case 0x6: return 20.0f;
    default: return 0.0f;  // unattached / no contract
  }
}

// PD_STATUS0 low nibble -> amps.
float HUSB238::decode_current(uint8_t code) {
  static const float TABLE[16] = {0.5f, 0.7f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.25f,
                                   2.5f, 2.75f, 3.0f, 3.25f, 3.5f, 4.0f, 4.5f, 5.0f};
  return TABLE[code & 0x0F];
}

bool HUSB238::read_register(uint8_t reg, uint8_t *data, size_t len) {
  const i2c::ErrorCode ret = i2c::I2CDevice::read_register(reg, data, len);
  return ret == i2c::ErrorCode::NO_ERROR;
}

bool HUSB238::write_register(uint8_t reg, uint8_t value) {
  const i2c::ErrorCode ret = i2c::I2CDevice::write_register(reg, &value, 1);
  return ret == i2c::ErrorCode::NO_ERROR;
}

void HUSB238::dump_config() {
  ESP_LOGCONFIG(TAG, "HUSB238 USB PD Sink Controller");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);

  if (output_voltage_sensor_)
    LOG_SENSOR("  ", "Output Voltage", output_voltage_sensor_);
  if (output_current_sensor_)
    LOG_SENSOR("  ", "Output Current", output_current_sensor_);
  if (input_voltage_sensor_)
    LOG_SENSOR("  ", "Input Voltage", input_voltage_sensor_);

  if (desired_selection_ != SEL_NONE)
    ESP_LOGCONFIG(TAG, "  Requested voltage selection: 0x%X", desired_selection_);
  else
    ESP_LOGCONFIG(TAG, "  Requested voltage: auto (no request)");
}

}  // namespace husb238_i2c
}  // namespace esphome
