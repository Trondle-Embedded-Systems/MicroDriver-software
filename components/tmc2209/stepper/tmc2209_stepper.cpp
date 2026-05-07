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
    this->ips_.current_position_ptr = &this->current_position;
    this->ips_.direction_ptr = &this->current_direction;
    this->index_pin_->attach_interrupt(IndexPulseStore::pulse_isr, &this->ips_, gpio::INTERRUPT_ANY_EDGE);
  }

  this->enable(true);

#if defined(USE_ESP32) && CONFIG_FREERTOS_UNICORE == 0
  if (this->control_method_ == ControlMethod::PULSES_CONTROL) {
    this->step_task_running_ = true;
    xTaskCreatePinnedToCore(TMC2209Stepper::step_task_, "tmc2209_step", 2048, this,
                            configMAX_PRIORITIES - 1, &this->step_task_handle_, 0);
  }
#endif

  ESP_LOGCONFIG(TAG, "TMC2209 Stepper setup done.");
}

void TMC2209Stepper::on_shutdown() {
#ifdef USE_ESP32
  this->step_task_running_ = false;
#endif
  this->stop();
}

void IRAM_ATTR HOT TMC2209Stepper::loop() {
  TMC2209Component::loop();

  // Compute speed and direction
  const time_t now = micros();
  this->calculate_speed_(now);
  const int32_t to_target = (this->target_position - this->current_position);
  this->current_direction = (to_target != 0 ? (Direction) (to_target / abs(to_target)) : Direction::STANDSTILL);

  int32_t new_vactual = this->speed_to_vactual(this->current_speed_);

  if (this->control_method_ == ControlMethod::SERIAL_CONTROL) {
    new_vactual *= this->current_direction;
    if (this->vactual_ != new_vactual) {
      this->write_field(VACTUAL_FIELD, new_vactual);
      this->vactual_ = new_vactual;
    }
  }

  if (this->control_method_ == ControlMethod::PULSES_CONTROL) {
    this->vactual_ = new_vactual;
#if !(defined(USE_ESP32) && CONFIG_FREERTOS_UNICORE == 0)
    const time_t dt = now - this->last_step_;
    if (new_vactual != 0 && dt >= (1 / (float) new_vactual) * 1e6f) {
      if (this->direction_ != this->current_direction) {
        this->dir_pin_->digital_write(this->current_direction == Direction::BACKWARD);
        this->direction_ = this->current_direction;
      }
      this->step_pin_->digital_write(this->step_state_);
      this->step_state_ = !this->step_state_;
      this->current_position += (int32_t) this->current_direction;
      this->last_step_ = now;
    }
#endif
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

#if defined(USE_ESP32) && CONFIG_FREERTOS_UNICORE == 0
void IRAM_ATTR HOT TMC2209Stepper::step_task_(void *arg) {
  TMC2209Stepper *self = static_cast<TMC2209Stepper *>(arg);
  while (self->step_task_running_) {
    const int32_t vactual = self->vactual_;
    if (vactual != 0) {
      const time_t now = micros();
      const time_t interval = 1000000UL / (uint32_t) abs(vactual);
      if ((now - self->last_step_) >= interval) {
        const Direction dir = self->current_direction;
        if (self->direction_ != dir) {
          self->dir_pin_->digital_write(dir == Direction::BACKWARD);
          self->direction_ = dir;
        }
        self->step_pin_->digital_write(self->step_state_);
        self->step_state_ = !self->step_state_;
        if (dir == Direction::FORWARD)
          self->current_position++;
        else if (dir == Direction::BACKWARD)
          self->current_position--;
        self->last_step_ = now;
      }
    }
    taskYIELD();
  }
  vTaskDelete(nullptr);
}
#endif

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
