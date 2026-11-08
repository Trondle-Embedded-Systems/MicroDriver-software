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

uint8_t HUSB238::negotiated_to_selection(uint8_t negotiated_code) {
  switch (negotiated_code) {
    case 0x1: return SEL_5V;
    case 0x2: return SEL_9V;
    case 0x3: return SEL_12V;
    case 0x4: return SEL_15V;
    case 0x5: return SEL_18V;
    case 0x6: return SEL_20V;
    default: return SEL_NONE;
  }
}

uint8_t HUSB238::volts_to_selection(int volts) {
  if (volts >= 20)
    return SEL_20V;
  if (volts >= 18)
    return SEL_18V;
  if (volts >= 15)
    return SEL_15V;
  if (volts >= 12)
    return SEL_12V;
  if (volts >= 9)
    return SEL_9V;
  if (volts >= 5)
    return SEL_5V;
  return SEL_NONE;
}

const char *HUSB238::selection_to_str(uint8_t selection_code) {
  switch (selection_code) {
    case SEL_5V: return "5V";
    case SEL_9V: return "9V";
    case SEL_12V: return "12V";
    case SEL_15V: return "15V";
    case SEL_18V: return "18V";
    case SEL_20V: return "20V";
    default: return "NONE";
  }
}

void HUSB238::set_request_voltage_volts(int volts) {
  const uint8_t code = volts_to_selection(volts);
  ESP_LOGI(TAG, "Setting desired PD voltage to %d V (selection 0x%X = %s)", volts, code, selection_to_str(code));
  this->desired_selection_ = code;
  this->target_selection_ = SEL_NONE;
  this->request_attempts_ = 0;
  this->cap_refresh_sent_ = false;
  this->source_attached_ = false;
  if (!this->is_failed()) {
    this->update();
  }
}

void HUSB238::log_advertised_pdos_() {
  static const struct {
    const char *name;
    uint8_t reg;
  } ALL_PDOS[] = {
      {"5V", REG_SRC_PDO_5V},   {"9V", REG_SRC_PDO_9V},   {"12V", REG_SRC_PDO_12V},
      {"15V", REG_SRC_PDO_15V}, {"18V", REG_SRC_PDO_18V}, {"20V", REG_SRC_PDO_20V},
  };
  ESP_LOGI(TAG, "PD source advertised capabilities:");
  uint8_t detected_count = 0;
  for (const auto &p : ALL_PDOS) {
    uint8_t v = 0;
    if (!read_register(p.reg, &v, 1))
      continue;
    if (v & 0x80) {
      detected_count++;
      ESP_LOGI(TAG, "  [%u] %-3s up to %.2f A (raw=0x%02X)", detected_count, p.name, decode_current(v & 0x0F), v);
    }
  }
  if (detected_count == 0) {
    ESP_LOGW(TAG, "  (none - source advertises no PDOs yet)");
  }
}

uint8_t HUSB238::best_available_selection(uint8_t desired) {
  // SRC_PDO availability registers, ascending in voltage. Bit 7 set = the source
  // advertises that PDO. We pick the highest advertised voltage that is <= desired.
  struct Pdo {
    uint8_t selection;
    uint8_t reg;
  };
  static const Pdo PDOS[] = {
      {SEL_5V, REG_SRC_PDO_5V},   {SEL_9V, REG_SRC_PDO_9V},   {SEL_12V, REG_SRC_PDO_12V},
      {SEL_15V, REG_SRC_PDO_15V}, {SEL_18V, REG_SRC_PDO_18V}, {SEL_20V, REG_SRC_PDO_20V},
  };
  const uint8_t count = sizeof(PDOS) / sizeof(PDOS[0]);

  // How far up the list we are allowed to go (inclusive). Unknown desired -> allow all.
  uint8_t limit = count - 1;
  for (uint8_t i = 0; i < count; i++) {
    if (PDOS[i].selection == desired) {
      limit = i;
      break;
    }
  }

  uint8_t best = SEL_NONE;
  for (uint8_t i = 0; i <= limit; i++) {
    uint8_t v;
    if (!read_register(PDOS[i].reg, &v, 1))
      continue;
    if (v & 0x80)  // bit 7 = this PDO is offered by the source
      best = PDOS[i].selection;  // ascending list -> last match is the highest
  }
  return best;
}

void HUSB238::update() {
  update_status();

  // Only act if a source is attached; GO_COMMAND against an unattached port wedges
  // the bus. Track attach edges so we (re)evaluate the target on each fresh attach.
  uint8_t status1;
  if (!read_register(REG_PD_STATUS1, &status1, 1)) {
    ESP_LOGD(TAG, "PD_STATUS1 read failed");
    return;
  }
  const uint8_t pd_response = (status1 >> 3) & 0x07;
  ESP_LOGD(TAG, "PD_STATUS1 = 0x%02X (ATTACH=%d, RESPONSE=%u)", status1, (status1 & PD_STATUS1_ATTACH) ? 1 : 0,
           pd_response);

  if (!(status1 & PD_STATUS1_ATTACH)) {
    source_attached_ = false;
    cap_refresh_sent_ = false;
    target_selection_ = SEL_NONE;
    return;
  }

  if (!source_attached_) {
    source_attached_ = true;
    request_attempts_ = 0;
    cap_refresh_sent_ = false;
    log_advertised_pdos_();
  }

  if (desired_selection_ == SEL_NONE)
    return;

  // Re-evaluate the best available PDO whenever we haven't locked onto the desired
  // voltage yet (e.g. if the HUSB238 was still receiving 15V/18V/20V capabilities
  // on the first attach tick, or after requesting Get_SRC_Cap).
  if (target_selection_ != desired_selection_) {
    const uint8_t new_target = best_available_selection(desired_selection_);
    if (new_target != target_selection_) {
      target_selection_ = new_target;
      request_attempts_ = 0;
      if (target_selection_ == SEL_NONE) {
        ESP_LOGW(TAG, "PD source advertises no usable PDO <= %s; leaving default voltage",
                 selection_to_str(desired_selection_));
      } else if (target_selection_ != desired_selection_) {
        ESP_LOGI(TAG, "Requested %s (0x%X) not offered yet; highest available is %s (0x%X)",
                 selection_to_str(desired_selection_), desired_selection_,
                 selection_to_str(target_selection_), target_selection_);
      } else {
        ESP_LOGI(TAG, "Target PD voltage %s (0x%X) is advertised by source",
                 selection_to_str(target_selection_), target_selection_);
      }
    }

    // If the charger still doesn't show our desired PDO, ask HUSB238 once to
    // refresh source capabilities (GO_COMMAND = 0b00100) in case the initial
    // capability table was incomplete.
    if (target_selection_ != desired_selection_ && !cap_refresh_sent_) {
      cap_refresh_sent_ = true;
      ESP_LOGI(TAG, "Requesting fresh Source_Capabilities (GO_GET_SRC_CAP) looking for %s",
               selection_to_str(desired_selection_));
      write_register(REG_GO_COMMAND, GO_GET_SRC_CAP);
    }
  }

  if (target_selection_ == SEL_NONE)
    return;

  // Already negotiated at the target? Then stop — do not keep re-requesting.
  uint8_t status0;
  if (!read_register(REG_PD_STATUS0, &status0, 1))
    return;
  if (negotiated_to_selection((status0 >> 4) & 0x0F) == target_selection_) {
    request_attempts_ = 0;
    return;
  }

  // Not there yet. Issue the request, but only a bounded number of times: a source
  // that never converges must not make us hammer GO_COMMAND forever (each request
  // can trigger a PD renegotiation that briefly collapses VBUS).
  if (request_attempts_ < MAX_REQUEST_ATTEMPTS) {
    request_voltage(target_selection_);
    request_attempts_++;
  }
}

bool HUSB238::request_voltage(uint8_t selection_code) {
  ESP_LOGI(TAG, "Requesting PD voltage %s (SRC_PDO selection 0x%X)", selection_to_str(selection_code), selection_code);

  // SRC_PDO holds the selection in the top 4 bits; preserve the lower 4 bits.
  uint8_t src_pdo = 0;
  read_register(REG_SRC_PDO, &src_pdo, 1);
  src_pdo = static_cast<uint8_t>((src_pdo & 0x0F) | ((selection_code & 0x0F) << 4));
  if (!write_register(REG_SRC_PDO, src_pdo)) {
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
