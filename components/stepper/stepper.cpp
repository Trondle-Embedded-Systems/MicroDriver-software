#include "stepper.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <algorithm>

namespace esphome {
namespace stepper {

static const char *const TAG = "stepper";
// Longest time step the acceleration ramp integrates in one update (s).
static constexpr float MAX_ACCEL_DT = 0.005f;

void IRAM_ATTR HOT Stepper::calculate_speed_(uint32_t now = micros()) {
  // micros() wraps every ~71.6 minutes; unsigned subtraction preserves dt.
  float dt = uint32_t(now - this->last_calculation_) * 1e-6f;
  this->last_calculation_ = now;
  if (this->has_reached_target()) {
    this->current_speed_ = 0.0f;
    return;
  }

  int64_t distance = int64_t(this->target_position) - int64_t(this->current_position);
  int64_t num_steps = distance < 0 ? -distance : distance;
  // (v_0)^2 / 2*a
  float v_squared = this->current_speed_ * this->current_speed_;
  float steps_to_decelerate = v_squared / (2 * this->deceleration_);
  if (num_steps <= steps_to_decelerate) {
    // need to start decelerating. Brake as hard as it takes to still stop AT
    // the target: normally that is the configured deceleration, but a target
    // moved closer mid-move would otherwise be hit at speed (abrupt halt).
    const float needed = v_squared / (2.0f * static_cast<float>(num_steps));
    this->current_speed_ -= std::max(this->deceleration_, needed) * dt;
  } else if (this->current_speed_ > this->max_speed_) {
    // max speed was lowered mid-move: ramp down to it instead of jumping
    this->current_speed_ = std::max(this->max_speed_, this->current_speed_ - this->deceleration_ * dt);
  } else {
    // we can still accelerate. Cap the step of a single update: after a long
    // loop pass (WiFi, UART) a high acceleration would otherwise jump the step
    // rate by hundreds of steps/s at once, enough to make the motor lose steps.
    this->current_speed_ =
        std::min(this->max_speed_, this->current_speed_ + this->acceleration_ * std::min(dt, MAX_ACCEL_DT));
  }
  if (this->current_speed_ < 0.0f)
    this->current_speed_ = 0.0f;
}
Direction IRAM_ATTR HOT Stepper::should_step_(uint32_t now = micros()) {
  this->calculate_speed_(now);
  if (this->current_speed_ == 0.0f) {
    this->current_direction = Direction::STANDSTILL;
    return this->current_direction;
  }

  // assumes this method is called in a constant interval
  uint32_t dt = now - this->last_step_;
  if (dt >= (1 / this->current_speed_) * 1e6f) {
    const Direction dir_ = (this->target_position > this->current_position ? Direction::FORWARD : Direction::BACKWARD);
    this->current_direction = dir_;
    this->current_position += (int32_t) dir_;
    this->last_step_ = now;
    return dir_;
  }

  this->current_direction = Direction::STANDSTILL;
  return this->current_direction;
}

}  // namespace stepper
}  // namespace esphome
