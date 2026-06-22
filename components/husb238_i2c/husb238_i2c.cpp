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

  // Debug: dump raw PD_STATUS1 every cycle so we can see whether ATTACH (bit 6,
  // 0x40) is actually being read as set. Remove once attach behaviour is confirmed.
  uint8_t dbg_status1;
  if (read_register(REG_PD_STATUS1, &dbg_status1, 1))
    ESP_LOGD(TAG, "PD_STATUS1 = 0x%02X (ATTACH=%d)", dbg_status1, (dbg_status1 & PD_STATUS1_ATTACH) ? 1 : 0);
  else
    ESP_LOGD(TAG, "PD_STATUS1 read failed");

  if (desired_selection_ == SEL_NONE)
    return;

  // Only act if a source is attached; GO_COMMAND against an unattached port wedges
  // the bus. Track attach edges so we (re)evaluate the target on each fresh attach.
  uint8_t status1;
  if (!read_register(REG_PD_STATUS1, &status1, 1))
    return;
  if (!(status1 & PD_STATUS1_ATTACH)) {
    source_attached_ = false;
    return;
  }

  if (!source_attached_) {
    source_attached_ = true;
    request_attempts_ = 0;

    // Report exactly which PDOs the source advertises, so the configured
    // request_voltage can be chosen with certainty (chargers often skip 12 V).
    // Per the HUSB238 datasheet, each SRC_PDO register (0x02..0x07) reports in
    // bit 7 whether the source advertises that voltage (the "voltage detected"
    // bit, == Adafruit's isVoltageDetected(pd)), and in bits [3:0] the maximum
    // current the source offers at that voltage. Log every detected PDO with its
    // proposed current so all available power profiles are visible in the logs.
    static const struct {
      const char *name;
      uint8_t reg;
    } ALL_PDOS[] = {{"5V", REG_SRC_PDO_5V},   {"9V", REG_SRC_PDO_9V},   {"12V", REG_SRC_PDO_12V},
                    {"15V", REG_SRC_PDO_15V}, {"18V", REG_SRC_PDO_18V}, {"20V", REG_SRC_PDO_20V}};
    ESP_LOGI(TAG, "PD source available voltages (HUSB238 isVoltageDetected):");
    uint8_t detected_count = 0;
    for (auto &p : ALL_PDOS) {
      uint8_t v;
      if (!read_register(p.reg, &v, 1))
        continue;
      if (v & 0x80) {  // bit 7 set -> source advertises this PDO (isVoltageDetected)
        detected_count++;
        ESP_LOGI(TAG, "  [%u] %-3s  up to %.2f A", detected_count, p.name,
                 decode_current(v & 0x0F));
      }
    }
    if (detected_count == 0)
      ESP_LOGW(TAG, "  (none - source advertises no PDOs)");

    // Request the highest voltage the source actually advertises, capped at desired.
    // Requesting an UNADVERTISED voltage (e.g. 20 V from a 15 V brick) just makes the
    // source ignore us or renegotiate every cycle -> VBUS dips -> ESP bootloop.
    target_selection_ = best_available_selection(desired_selection_);
    if (target_selection_ == SEL_NONE) {
      ESP_LOGW(TAG, "PD source advertises no usable PDO <= request; leaving default voltage");
    } else if (target_selection_ != desired_selection_) {
      ESP_LOGI(TAG, "Requested selection 0x%X not offered; falling back to highest available 0x%X",
               desired_selection_, target_selection_);
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
