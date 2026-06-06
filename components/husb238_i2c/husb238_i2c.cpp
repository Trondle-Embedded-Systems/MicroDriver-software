#include "husb238_i2c.h"
#include "esphome/core/log.h"

namespace esphome {
namespace husb238_i2c {

static const char *TAG = "husb238.i2c";

void HUSB238::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HUSB238 USB Power Delivery Controller...");

  uint8_t chip_id;
  if (!read_register(REG_CHIP_ID, &chip_id, 1)) {
    ESP_LOGE(TAG, "Failed to read HUSB238 chip ID");
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "HUSB238 Chip ID: 0x%02X", chip_id);

  uint8_t fw_version;
  if (!read_register(REG_FIRMWARE_VERSION, &fw_version, 1)) {
    ESP_LOGW(TAG, "Failed to read firmware version");
  } else {
    ESP_LOGCONFIG(TAG, "HUSB238 Firmware Version: 0x%02X", fw_version);
  }

  ESP_LOGCONFIG(TAG, "HUSB238 setup complete!");
}

void HUSB238::update() {
  update_output_voltage();
  update_output_current();
  update_input_voltage();
}

void HUSB238::update_output_voltage() {
  if (!output_voltage_sensor_) return;

  uint8_t voltage_raw;
  if (!read_register(REG_VOLTAGE, &voltage_raw, 1)) {
    ESP_LOGW(TAG, "Failed to read output voltage");
    return;
  }

  float voltage = voltage_raw * 0.1f;
  if (voltage != last_output_voltage_) {
    output_voltage_sensor_->publish_state(voltage);
    last_output_voltage_ = voltage;
    ESP_LOGD(TAG, "Output voltage: %.2f V", voltage);
  }
}

void HUSB238::update_output_current() {
  if (!output_current_sensor_) return;

  uint8_t current_raw;
  if (!read_register(REG_CURRENT, &current_raw, 1)) {
    ESP_LOGW(TAG, "Failed to read output current");
    return;
  }

  float current = current_raw * 10.0f;
  if (current != last_output_current_) {
    output_current_sensor_->publish_state(current);
    last_output_current_ = current;
    ESP_LOGD(TAG, "Output current: %.0f mA", current);
  }
}

void HUSB238::update_input_voltage() {
  if (!input_voltage_sensor_) return;

  uint8_t input_voltage_raw;
  if (!read_register(REG_POWER, &input_voltage_raw, 1)) {
    ESP_LOGW(TAG, "Failed to read input voltage");
    return;
  }

  float voltage = input_voltage_raw * 0.1f;
  if (voltage != last_input_voltage_) {
    input_voltage_sensor_->publish_state(voltage);
    last_input_voltage_ = voltage;
    ESP_LOGD(TAG, "Input voltage: %.2f V", voltage);
  }
}

bool HUSB238::read_register(uint8_t reg, uint8_t *data, size_t len) {
  const i2c::ErrorCode ret = i2c::I2CDevice::read_register(reg, data, len);
  return (ret == i2c::ErrorCode::NO_ERROR || ret == i2c::ErrorCode::ERROR_OK);
}

void HUSB238::dump_config() {
  ESP_LOGCONFIG(TAG, "HUSB238 USB PD Controller");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);

  if (output_voltage_sensor_) {
    LOG_SENSOR("  ", "Output Voltage", output_voltage_sensor_);
  }
  if (output_current_sensor_) {
    LOG_SENSOR("  ", "Output Current", output_current_sensor_);
  }
  if (input_voltage_sensor_) {
    LOG_SENSOR("  ", "Input Voltage", input_voltage_sensor_);
  }

  ESP_LOGCONFIG(TAG, "  PDO Configuration:");
  for (uint8_t i = 0; i < 5; i++) {
    if (pdo_voltages_[i] > 0) {
      ESP_LOGCONFIG(TAG, "    PDO%d: %dV, %dA", i + 1, pdo_voltages_[i], pdo_currents_[i]);
    }
  }
}

}  // namespace husb238_i2c
}  // namespace esphome
