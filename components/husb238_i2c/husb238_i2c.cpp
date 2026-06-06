#include "husb238_i2c.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace husb238_i2c {

static const char *TAG = "husb238.i2c";

void HUSB238::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HUSB238 USB Power Delivery Controller...");

  // Read chip ID to verify communication
  uint8_t chip_id;
  if (!read_register(REG_CHIP_ID, &chip_id, 1)) {
    ESP_LOGE(TAG, "Failed to read HUSB238 chip ID");
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "HUSB238 Chip ID: 0x%02X", chip_id);

  // Read firmware version
  uint8_t fw_version;
  if (!read_register(REG_FIRMWARE_VERSION, &fw_version, 1)) {
    ESP_LOGW(TAG, "Failed to read firmware version");
  } else {
    ESP_LOGCONFIG(TAG, "HUSB238 Firmware Version: 0x%02X", fw_version);
  }

  // Initialize enable output switch if configured
  if (enable_output_switch_) {
    enable_output_switch_->add_on_state_callback([this](bool state) {
      this->handle_enable_output_(state);
    });
  }

  // Initialize select voltage number if configured
  if (select_voltage_number_) {
    select_voltage_number_->add_on_state_callback([this](float value) {
      this->handle_pdo_selection_(value);
    });
  }

  ESP_LOGCONFIG(TAG, "HUSB238 setup complete!");
}

void HUSB238::update() {
  // Update output voltage
  update_output_voltage_();

  // Update output current
  update_output_current_();

  // Update input voltage
  update_input_voltage_();

  // Update PDO status
  update_pdo_status_();
}

void HUSB238::update_output_voltage_() {
  if (!output_voltage_sensor_) {
    return;
  }

  uint8_t voltage_raw;
  if (!read_register(REG_VOLTAGE, &voltage_raw, 1)) {
    ESP_LOGW(TAG, "Failed to read output voltage");
    return;
  }

  // Convert register value to voltage (typical: register value in 0.1V steps)
  float voltage = voltage_raw * 0.1f;

  if (voltage != last_output_voltage_) {
    output_voltage_sensor_->publish_state(voltage);
    last_output_voltage_ = voltage;
    ESP_LOGD(TAG, "Output voltage: %.2f V", voltage);
  }
}

void HUSB238::update_output_current_() {
  if (!output_current_sensor_) {
    return;
  }

  uint8_t current_raw;
  if (!read_register(REG_CURRENT, &current_raw, 1)) {
    ESP_LOGW(TAG, "Failed to read output current");
    return;
  }

  // Convert register value to current (typical: register value in 10mA steps)
  float current = current_raw * 10.0f;

  if (current != last_output_current_) {
    output_current_sensor_->publish_state(current);
    last_output_current_ = current;
    ESP_LOGD(TAG, "Output current: %.0f mA", current);
  }
}

void HUSB238::update_input_voltage_() {
  if (!input_voltage_sensor_) {
    return;
  }

  uint8_t status;
  if (!read_register(REG_STATUS, &status, 1)) {
    ESP_LOGW(TAG, "Failed to read status");
    return;
  }

  // For input voltage, we derive it from status or a specific register
  // This is a simplified implementation - actual register may vary
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

void HUSB238::update_pdo_status_() {
  // Read available PDOs
  uint8_t available_pdos;
  if (!read_register(REG_AVAILABLE_PDOS, &available_pdos, 1)) {
    ESP_LOGW(TAG, "Failed to read available PDOs");
    return;
  }

  if (available_pdos != last_available_pdos_) {
    ESP_LOGD(TAG, "Available PDOs: %d", available_pdos);
    last_available_pdos_ = available_pdos;
  }

  // Read currently selected PDO
  uint8_t selected_pdo;
  if (!read_register(REG_SELECTED_PDO, &selected_pdo, 1)) {
    ESP_LOGW(TAG, "Failed to read selected PDO");
    return;
  }

  if (selected_pdo != last_selected_pdo_) {
    ESP_LOGD(TAG, "Selected PDO: %d", selected_pdo);
    last_selected_pdo_ = selected_pdo;

    if (selected_pdo_number_) {
      selected_pdo_number_->publish_state(selected_pdo);
    }
  }
}

void HUSB238::handle_pdo_selection_(float voltage) {
  // Find the PDO index that matches the requested voltage
  uint8_t pdo_index = 0;
  bool found = false;

  for (uint8_t i = 0; i < 5; i++) {
    if (pdo_voltages_[i] == (uint8_t)voltage) {
      pdo_index = i;
      found = true;
      break;
    }
  }

  if (!found) {
    ESP_LOGW(TAG, "Requested voltage %.0f V not found in PDO configuration", voltage);
    return;
  }

  // Send PDO selection command to device
  if (!write_register_byte(REG_SELECTED_PDO, pdo_index)) {
    ESP_LOGE(TAG, "Failed to set PDO selection to %d", pdo_index);
    return;
  }

  ESP_LOGD(TAG, "PDO selection set to index %d (%.0f V)", pdo_index, voltage);
}

void HUSB238::handle_enable_output_(bool enable) {
  uint8_t enable_val = enable ? 0x01 : 0x00;

  if (!write_register_byte(REG_OUTPUT_ENABLE, enable_val)) {
    ESP_LOGE(TAG, "Failed to set output enable to %d", enable);
    return;
  }

  ESP_LOGD(TAG, "Output %s", enable ? "enabled" : "disabled");
}

bool HUSB238::read_register(uint8_t reg, uint8_t *data, size_t len) {
  const i2c::ErrorCode ret = i2c::I2CDevice::read_register(reg, data, len);
  return (ret == i2c::ErrorCode::NO_ERROR || ret == i2c::ErrorCode::ERROR_OK);
}

bool HUSB238::write_register(uint8_t reg, const uint8_t *data, size_t len) {
  const i2c::ErrorCode ret = i2c::I2CDevice::write_register(reg, data, len);
  return (ret == i2c::ErrorCode::NO_ERROR || ret == i2c::ErrorCode::ERROR_OK);
}

bool HUSB238::write_register_byte(uint8_t reg, uint8_t data) {
  const i2c::ErrorCode ret = i2c::I2CDevice::write_register(reg, &data, 1);
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

  if (enable_output_switch_) {
    ESP_LOGCONFIG(TAG, "  Enable Output Switch: %s", enable_output_switch_->get_name().c_str());
  }

  if (selected_pdo_number_) {
    ESP_LOGCONFIG(TAG, "  Selected PDO Number: %s", selected_pdo_number_->get_name().c_str());
  }

  if (select_voltage_number_) {
    ESP_LOGCONFIG(TAG, "  Select Voltage Number: %s", select_voltage_number_->get_name().c_str());
  }

  if (max_current_number_) {
    ESP_LOGCONFIG(TAG, "  Max Current Number: %s", max_current_number_->get_name().c_str());
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
