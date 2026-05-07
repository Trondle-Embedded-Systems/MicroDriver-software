#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/stepper/stepper.h"

namespace esphome {
namespace stepper_closed_loop {

using namespace esphome::stepper;

class StepperClosedLoop : public PollingComponent {
 public:
  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void update() override;
  void dump_config() override;

  void set_stepper(Stepper *stepper) { this->stepper_ = stepper; }
  void set_position_sensor(sensor::Sensor *s) { this->position_sensor_ = s; }
  void set_error_sensor(sensor::Sensor *s) { this->error_sensor_ = s; }
  void set_steps_per_revolution(int32_t v) { this->steps_per_rev_ = v; }
  void set_encoder_counts(int32_t v) { this->encoder_counts_ = v; }
  void set_correction_threshold(int32_t v) { this->correction_threshold_ = v; }
  void set_max_correction(int32_t v) { this->max_correction_ = v; }
  void set_initial_sync(bool v) { this->initial_sync_ = v; }
  void set_settle_window(uint32_t ms) { this->settle_window_ms_ = ms; }
  void set_auto_disable(bool v) { this->auto_disable_ = v; }

 protected:
  Stepper *stepper_{nullptr};
  sensor::Sensor *position_sensor_{nullptr};
  sensor::Sensor *error_sensor_{nullptr};

  int32_t steps_per_rev_{200};
  int32_t encoder_counts_{4096};
  int32_t correction_threshold_{10};
  int32_t max_correction_{500};
  bool initial_sync_{true};
  uint32_t settle_window_ms_{0};

  // Auto-disable
  bool auto_disable_{false};
  bool motor_disabled_{false};
  int32_t last_target_{0};

  // Multi-turn tracking state
  float last_encoder_pos_{0.0f};
  int32_t turn_count_{0};
  bool initialized_{false};

  // Settle-window tracking
  bool was_at_target_{false};
  uint32_t target_reached_at_ms_{0};

  float steps_per_count_() const { return (float) this->steps_per_rev_ / (float) this->encoder_counts_; }
  int32_t encoder_to_steps_(float encoder_pos) const;
};

}  // namespace stepper_closed_loop
}  // namespace esphome
