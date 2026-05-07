#include "stepper_closed_loop.h"
#include "esphome/core/log.h"

namespace esphome {
namespace stepper_closed_loop {

static const char *const TAG = "stepper_closed_loop";

void StepperClosedLoop::setup() {
  this->last_target_ = this->stepper_->target_position;
}

void StepperClosedLoop::dump_config() {
  ESP_LOGCONFIG(TAG, "Stepper Closed Loop:");
  ESP_LOGCONFIG(TAG, "  Steps/revolution: %d", this->steps_per_rev_);
  ESP_LOGCONFIG(TAG, "  Encoder counts/revolution: %d", this->encoder_counts_);
  ESP_LOGCONFIG(TAG, "  Correction threshold: %d steps", this->correction_threshold_);
  ESP_LOGCONFIG(TAG, "  Max correction: %d steps", this->max_correction_);
  ESP_LOGCONFIG(TAG, "  Initial sync: %s", TRUEFALSE(this->initial_sync_));
  ESP_LOGCONFIG(TAG, "  Settle window: %u ms", this->settle_window_ms_);
  ESP_LOGCONFIG(TAG, "  Auto-disable: %s", TRUEFALSE(this->auto_disable_));
}

int32_t StepperClosedLoop::encoder_to_steps_(float encoder_pos) const {
  float abs_counts = (float) this->turn_count_ * (float) this->encoder_counts_ + encoder_pos;
  return (int32_t) (abs_counts * this->steps_per_count_());
}

void StepperClosedLoop::update() {
  if (!this->position_sensor_->has_state())
    return;

  const float encoder_pos = this->position_sensor_->get_state();

  if (!this->initialized_) {
    this->last_encoder_pos_ = encoder_pos;
    this->initialized_ = true;

    if (this->initial_sync_) {
      const int32_t initial_steps = this->encoder_to_steps_(encoder_pos);
      this->stepper_->current_position = initial_steps;
      this->stepper_->target_position = initial_steps;
      this->last_target_ = initial_steps;
      ESP_LOGI(TAG, "Initial sync: encoder=%.0f -> steps=%d", encoder_pos, initial_steps);
    } else {
      this->last_target_ = this->stepper_->target_position;
    }
    return;
  }

  // Detect wrap-around for multi-turn position tracking.
  // A jump of more than half a revolution means the encoder wrapped.
  const float half_counts = (float) this->encoder_counts_ * 0.5f;
  const float delta = encoder_pos - this->last_encoder_pos_;
  if (delta > half_counts) {
    this->turn_count_--;  // wrapped backwards (e.g. 4090 -> 10)
  } else if (delta < -half_counts) {
    this->turn_count_++;  // wrapped forwards (e.g. 10 -> 4090)
  }
  this->last_encoder_pos_ = encoder_pos;

  // If auto-disabled: wait for a new target command, then re-enable and re-sync from encoder.
  if (this->motor_disabled_) {
    if (this->stepper_->target_position != this->last_target_) {
      const int32_t synced = this->encoder_to_steps_(encoder_pos);
      this->stepper_->current_position = synced;
      this->stepper_->enable(true);
      this->motor_disabled_ = false;
      this->was_at_target_ = false;
      this->last_target_ = this->stepper_->target_position;
      ESP_LOGI(TAG, "Re-enabled: position synced to %d steps from encoder", synced);
    }
    return;
  }

  const int32_t actual_steps = this->encoder_to_steps_(encoder_pos);
  const int32_t motor_steps = this->stepper_->current_position;
  const int32_t error = actual_steps - motor_steps;

  if (this->error_sensor_ != nullptr)
    this->error_sensor_->publish_state((float) error);

  // Manage settle window: allow corrections for settle_window_ms_ after the motor
  // reaches its target, then stop (and optionally disable) to avoid chasing encoder noise.
  const bool at_target = this->stepper_->has_reached_target();
  if (at_target && !this->was_at_target_) {
    this->target_reached_at_ms_ = millis();
  }
  this->was_at_target_ = at_target;

  if (at_target) {
    const uint32_t elapsed = millis() - this->target_reached_at_ms_;
    if (elapsed > this->settle_window_ms_) {
      if (this->auto_disable_) {
        this->stepper_->enable(false);
        this->motor_disabled_ = true;
        this->last_target_ = this->stepper_->target_position;
        ESP_LOGD(TAG, "Motor auto-disabled after settle");
      }
      return;
    }
  }

  if (abs(error) <= this->correction_threshold_)
    return;

  // Discard suspiciously large errors — likely a noisy/invalid encoder reading.
  if (abs(error) > this->max_correction_) {
    ESP_LOGW(TAG, "Encoder error %d steps exceeds max_correction (%d), skipping (noise?)", error,
             this->max_correction_);
    return;
  }

  // Re-sync: tell the stepper where it actually is.
  // Its existing acceleration profile will then drive it from here to target_position.
  ESP_LOGD(TAG, "Correcting: motor=%d encoder=%d error=%d", motor_steps, actual_steps, error);
  this->stepper_->current_position = actual_steps;
}

}  // namespace stepper_closed_loop
}  // namespace esphome
