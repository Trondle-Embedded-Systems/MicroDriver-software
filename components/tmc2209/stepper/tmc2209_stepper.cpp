#include "tmc2209_stepper.h"
#include "esphome/components/tmc2209/tmc2209_api_registers.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace tmc2209 {

void TMC2209Stepper::dump_config() {
  ESP_LOGCONFIG(TAG, "TMC2209 Stepper:");
  LOG_STEPPER(this);
  LOG_TMC2209(this);
}

void TMC2209Stepper::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TMC2209 Stepper...");
  TMC2209Component::setup();

  this->high_freq_.start();

  this->write_field(VACTUAL_FIELD, 0);

  if (this->control_method_ == ControlMethod::PULSES_CONTROL) {
    this->write_field(MULTISTEP_FILT_FIELD, false);
    this->write_field(DEDGE_FIELD, true);
  }

  if (this->control_method_ == ControlMethod::SERIAL_CONTROL) {
    /* Configure INDEX for pulse feedback from the driver */
    // Check mux from figure 15.1 from datasheet rev1.09
    this->write_field(DEDGE_FIELD, false);
    this->write_field(INDEX_OTPW_FIELD, false);
    this->write_field(INDEX_STEP_FIELD, true);
    this->ips_.current_position_ptr = const_cast<int32_t*>(&this->current_position);
    this->ips_.direction_ptr = const_cast<Direction*>(&this->current_direction);
    this->index_pin_->attach_interrupt(IndexPulseStore::pulse_isr, &this->ips_, gpio::INTERRUPT_ANY_EDGE);
  }

  this->enable(true);

  ESP_LOGCONFIG(TAG, "TMC2209 Stepper setup done.");
}

void TMC2209Stepper::on_shutdown() { this->stop(); }

void IRAM_ATTR HOT TMC2209Stepper::loop() {
  TMC2209Component::loop();

  // Compute speed and direction
  const time_t now = micros();
  this->calculate_speed_(now);
  const int32_t to_target = (this->target_position - this->current_position);
  this->current_direction = (to_target != 0 ? (Direction) (to_target / abs(to_target)) : Direction::STANDSTILL);

  if (this->control_method_ == ControlMethod::SERIAL_CONTROL) {
    // The driver's internal pulse generator (VACTUAL) is expressed in clock-based
    // units, so convert steps/s to VACTUAL units here.
    const int32_t new_vactual = this->speed_to_vactual(this->current_speed_) * this->current_direction;
    if (this->vactual_ != new_vactual) {
      this->write_field(VACTUAL_FIELD, new_vactual);
      this->vactual_ = new_vactual;
    }
  }

  if (this->control_method_ == ControlMethod::PULSES_CONTROL) {
    // Generate STEP pulses from the (high-frequency) main loop. current_speed_ is the
    // pulse frequency in steps/s directly; DEDGE is enabled so every edge is one
    // microstep. Doing this in loop() instead of a dedicated max-priority FreeRTOS
    // task keeps the scheduler/watchdog fed and avoids starving WiFi and the main loop.
    if (this->current_speed_ > 0.0f && this->current_direction != Direction::STANDSTILL) {
      const time_t interval = (time_t) (1e6f / this->current_speed_);
      if ((now - this->last_step_) >= interval) {
        if (this->direction_ != this->current_direction) {
          this->dir_pin_->digital_write(this->current_direction == Direction::BACKWARD);
          this->direction_ = this->current_direction;
        }
        this->step_pin_->digital_write(this->step_state_);
        this->step_state_ = !this->step_state_;
        this->current_position += (int32_t) this->current_direction;
        this->last_step_ = now;
      }
    }
  }

  // Auto-disable after settling at target (Case 2: no stepper_closed_loop).
  if (this->auto_disable_ms_ > 0 && !this->is_homing_) {
    const bool at_target = this->has_reached_target();
    if (at_target && !this->was_at_target_ad_) {
      this->target_reached_at_ad_ms_ = millis();
    }
    this->was_at_target_ad_ = at_target;
    if (at_target && !this->auto_disabled_ &&
        (millis() - this->target_reached_at_ad_ms_) >= this->auto_disable_ms_) {
      this->enable(false);
      this->auto_disabled_ = true;
      ESP_LOGD(TAG, "Auto-disabled after %u ms settle", this->auto_disable_ms_);
    }
  }

  // StallGuard homing: stall at end-stop means we have found home.
  if (this->is_homing_ && this->is_stalled()) {
    const int32_t confirmed_home = this->target_position;  // the end-stop we aimed for
    this->current_position = confirmed_home;
    this->set_max_speed(this->pre_homing_max_speed_);
    this->write_register(SGTHRS, this->pre_homing_sgthrs_);
    this->write_register(TCOOLTHRS, this->pre_homing_tcoolthrs_);
    this->is_homing_ = false;
    Stepper::set_target(this->homing_pending_target_);
    ESP_LOGI(TAG, "Homing complete at %d, proceeding to %d", confirmed_home, this->homing_pending_target_);
  }
}

void TMC2209Stepper::set_target(int32_t steps) {
  if (this->control_method_ == ControlMethod::CONTROL_UNSET) {
    ESP_LOGE(TAG, "Control method not set!");
  }

  if (!this->is_enabled_) {
    if (this->homing_enabled_ && this->auto_disabled_) {
      // Pick the nearest configured home position.
      int32_t nearest = this->home_positions_[0];
      int32_t nearest_dist = abs(this->current_position - nearest);
      for (uint8_t i = 1; i < this->home_position_count_; i++) {
        int32_t dist = abs(this->current_position - this->home_positions_[i]);
        if (dist < nearest_dist) {
          nearest_dist = dist;
          nearest = this->home_positions_[i];
        }
      }
      // Skip homing if already at the nearest home.
      if (this->current_position != nearest) {
        this->homing_pending_target_ = steps;
        this->is_homing_ = true;
        this->pre_homing_max_speed_ = this->max_speed_;
        this->set_max_speed(this->home_speed_);
        this->pre_homing_sgthrs_ = this->read_register(SGTHRS);
        this->pre_homing_tcoolthrs_ = this->read_register(TCOOLTHRS);
        this->write_register(SGTHRS, this->homing_sgthrs_);
        this->write_register(TCOOLTHRS, this->homing_tcoolthrs_);
        this->enable(true);
        this->auto_disabled_ = false;
        Stepper::set_target(nearest);
        ESP_LOGI(TAG, "StallGuard homing: nearest end-stop %d at %.0f steps/s", nearest, this->home_speed_);
        return;
      }
    }
    this->enable(true);
  }
  this->auto_disabled_ = false;
  Stepper::set_target(steps);
}

void TMC2209Stepper::stop() {
  Stepper::stop();
  if (this->control_method_ == ControlMethod::SERIAL_CONTROL) {
    this->write_field(VACTUAL_FIELD, 0);
  }
}

void TMC2209Stepper::enable(bool enable) {
  if (!enable) {
    this->stop();
  }
  TMC2209Component::enable(enable);
}

bool TMC2209Stepper::is_stalled() {
  if (this->current_direction == Direction::STANDSTILL) {
    return false;
  }

  const int32_t sgthrs = this->read_register(SGTHRS);
  const int32_t sgresult = this->read_register(SG_RESULT);
  return (sgthrs << 1) > sgresult;
}

}  // namespace tmc2209
}  // namespace esphome
