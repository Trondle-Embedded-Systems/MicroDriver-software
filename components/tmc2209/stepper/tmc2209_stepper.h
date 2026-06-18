#pragma once
#include "esphome/components/tmc2209/tmc2209_api_registers.h"
#include "esphome/components/tmc2209/tmc2209_api.h"
#include "esphome/components/tmc2209/tmc2209_component.h"
#include "esphome/components/tmc2209/events.h"

#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/stepper/stepper.h"

namespace esphome {
namespace tmc2209 {

using namespace esphome::stepper;

enum ControlMethod {
  CONTROL_UNSET,
  SERIAL_CONTROL,
  PULSES_CONTROL,
};

struct IndexPulseStore {
  int32_t *current_position_ptr{nullptr};
  Direction *direction_ptr{nullptr};
  static void IRAM_ATTR HOT pulse_isr(IndexPulseStore *arg) {
    (*(arg->current_position_ptr)) += (int8_t) (*(arg->direction_ptr));
  }
};

class TMC2209Stepper : public TMC2209Component, public Stepper {
 public:
  TMC2209Stepper() = default;
  TMC2209Stepper(uint8_t address) : TMC2209Component(address){};

  void dump_config() override;
  void setup() override;
  void IRAM_ATTR HOT loop() override;
  void on_shutdown() override;
  void stop() override;
  void enable(bool enable) override;
  // void enable(bool enable, bool recover_toff = true) override;
  void set_target(int32_t steps) override;
  bool is_stalled() override;

  void set_control_method(ControlMethod method) { this->control_method_ = method; }

  void set_auto_disable_ms(uint32_t ms) { this->auto_disable_ms_ = ms; }
  void add_home_position(int32_t pos) {
    if (this->home_position_count_ < MAX_HOME_POSITIONS) {
      this->home_positions_[this->home_position_count_++] = pos;
      this->homing_enabled_ = true;
    }
  }
  void set_home_speed(float speed) { this->home_speed_ = speed; }
  void set_homing_sgthrs(uint8_t v) { this->homing_sgthrs_ = v; }
  void set_homing_tcoolthrs(uint32_t v) { this->homing_tcoolthrs_ = v; }

 protected:
  HighFrequencyLoopRequester high_freq_;
  ControlMethod control_method_{ControlMethod::CONTROL_UNSET};

  /** Serial control */
  IndexPulseStore ips_;  // index pulse store
  volatile int32_t vactual_ = 0;
  /* */

  /** Pulses control */
  volatile bool step_state_ = false;
  volatile Direction direction_{Direction::STANDSTILL};
  volatile time_t last_step_{0};
  // DEDGE (one microstep per edge) can only be programmed over UART. Without a
  // hub it stays at the driver default (step on rising edge), so the loop must
  // emit a full pulse per microstep instead of toggling a single edge.
  bool dedge_active_{false};
  /* */

  // Auto-disable after reaching target
  uint32_t auto_disable_ms_{0};
  bool auto_disabled_{false};
  bool was_at_target_ad_{false};
  uint32_t target_reached_at_ad_ms_{0};

  // StallGuard homing on re-enable (up to MAX_HOME_POSITIONS end-stops)
  static constexpr uint8_t MAX_HOME_POSITIONS = 4;
  bool homing_enabled_{false};
  int32_t home_positions_[MAX_HOME_POSITIONS]{};
  uint8_t home_position_count_{0};
  float home_speed_{400.0f};
  uint8_t homing_sgthrs_{50};
  uint32_t homing_tcoolthrs_{300000};
  bool is_homing_{false};
  int32_t homing_pending_target_{0};
  float pre_homing_max_speed_{0.0f};
  uint32_t pre_homing_sgthrs_{0};
  uint32_t pre_homing_tcoolthrs_{0};
};

}  // namespace tmc2209
}  // namespace esphome
