#include "tmc2208_stepper.h"
#include "esphome/components/tmc2208/tmc2208_api_registers.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace tmc2208 {

void TMC2208Stepper::dump_config() {
  ESP_LOGCONFIG(TAG, "TMC2208 Stepper:");
  LOG_STEPPER(this);
  LOG_TMC2208(this);
}

void TMC2208Stepper::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TMC2208 Stepper...");
  TMC2208Component::setup();

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
    xTaskCreatePinnedToCore(TMC2208Stepper::step_task_, "tmc2208_step", 2048, this,
                            configMAX_PRIORITIES - 1, &this->step_task_handle_, 0);
  }
#endif

  ESP_LOGCONFIG(TAG, "TMC2208 Stepper setup done.");
}

void TMC2208Stepper::on_shutdown() {
#ifdef USE_ESP32
  this->step_task_running_ = false;
#endif
  this->stop();
}

void IRAM_ATTR HOT TMC2208Stepper::loop() {
  TMC2208Component::loop();

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
}

#if defined(USE_ESP32) && CONFIG_FREERTOS_UNICORE == 0
void IRAM_ATTR HOT TMC2208Stepper::step_task_(void *arg) {
  TMC2208Stepper *self = static_cast<TMC2208Stepper *>(arg);
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

void TMC2208Stepper::set_target(int32_t steps) {
  if (this->control_method_ == ControlMethod::CONTROL_UNSET) {
    ESP_LOGE(TAG, "Control method not set!");
  }

  if (!this->is_enabled_) {
    this->enable(true);
  }
  Stepper::set_target(steps);
}

void TMC2208Stepper::stop() {
  Stepper::stop();
  if (this->control_method_ == ControlMethod::SERIAL_CONTROL) {
    this->write_field(VACTUAL_FIELD, 0);
  }
}

void TMC2208Stepper::enable(bool enable) {
  if (!enable) {
    this->stop();
  }
  TMC2208Component::enable(enable);
}

}  // namespace tmc2208
}  // namespace esphome
