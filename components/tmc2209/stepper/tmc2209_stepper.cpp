#include "tmc2209_stepper.h"
#include "esphome/components/tmc2209/tmc2209_api_registers.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

#ifdef TMC2209_USE_STEP_TIMER
#include <esp_rom_sys.h>
#endif

namespace esphome {
namespace tmc2209 {

#ifdef TMC2209_USE_STEP_TIMER
bool IRAM_ATTR StepPulseStore::timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata,
                                         void *arg) {
  auto *s = static_cast<StepPulseStore *>(arg);
  const uint32_t interval = s->interval_us;
  const int32_t pos = *s->current_position;
  const int32_t target = *s->target_position;
  uint64_t next_alarm;

  if (interval == 0 || pos == target) {
    // Standstill: tick slowly so a new target is picked up within IDLE_TICK_US.
    s->next_step_at = 0;
    next_alarm = edata->count_value + IDLE_TICK_US;
  } else {
    const int8_t dir = (target > pos) ? 1 : -1;
    if (dir != s->last_dir) {
      s->dir_pin.digital_write(dir < 0);
      s->last_dir = dir;
    }
    if (s->dedge) {
      // DEDGE enabled (UART): every edge is one microstep, so just toggle.
      s->step_state = !s->step_state;
      s->step_pin.digital_write(s->step_state);
    } else {
      // Standalone (no UART): only rising edges step; emit a full pulse. The
      // driver minimum high time is ~100 ns, so 1 µs busy-wait is plenty.
      s->step_pin.digital_write(true);
      esp_rom_delay_us(1);
      s->step_pin.digital_write(false);
    }
    *s->current_position = pos + dir;

    // Schedule the next step relative to the previous due time (drift-free at
    // constant speed). After idle, or if we fell behind (interval shrank, loop
    // stall), restart from now instead of bursting catch-up steps.
    const uint64_t base = (s->next_step_at != 0) ? s->next_step_at : edata->count_value;
    s->next_step_at = base + interval;
    if (s->next_step_at <= edata->count_value)
      s->next_step_at = edata->count_value + interval;
    next_alarm = s->next_step_at;
  }

  // Keep a small margin so the new alarm can never land in the past (a missed
  // alarm would stop the timer chain entirely).
  const uint64_t min_next = edata->count_value + 5;
  if (next_alarm < min_next)
    next_alarm = min_next;

  gptimer_alarm_config_t alarm = {};
  alarm.alarm_count = next_alarm;
  gptimer_set_alarm_action(timer, &alarm);
  return false;
}
#endif  // TMC2209_USE_STEP_TIMER

void TMC2209Stepper::dump_config() {
  ESP_LOGCONFIG(TAG, "TMC2209 Stepper:");
  LOG_STEPPER(this);
  LOG_TMC2209(this);
}

void TMC2209Stepper::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TMC2209 Stepper...");
  TMC2209Component::setup();

  // If the driver did not answer on UART, don't try to configure or drive it.
  // The device still boots so the rest of the node (WiFi, API, logs) stays usable.
  if (this->is_failed()) {
    ESP_LOGE(TAG, "TMC2209 not responding on UART; stepper disabled (check wiring/baud_rate).");
    return;
  }

  this->high_freq_.start();

  this->write_field(VACTUAL_FIELD, 0);

  if (this->control_method_ == ControlMethod::PULSES_CONTROL) {
    this->write_field(MULTISTEP_FILT_FIELD, false);
    this->write_field(DEDGE_FIELD, true);
    // The DEDGE write above only reaches the driver when the UART bus is enabled.
    // In standalone mode it is a no-op, so the driver keeps DEDGE off and the loop
    // must emit a full STEP pulse per microstep (see loop()).
    this->dedge_active_ = this->bus_enabled();

#ifdef TMC2209_USE_STEP_TIMER
    // Pace STEP pulses from a hardware timer ISR instead of the main loop: a
    // loop pass regularly exceeds one step period (WiFi/API/sensors), which
    // makes loop-paced pulses visibly jerky. See StepPulseStore.
    this->sps_.step_pin = this->step_pin_->to_isr();
    this->sps_.dir_pin = this->dir_pin_->to_isr();
    this->sps_.current_position = &this->current_position;
    this->sps_.target_position = (volatile int32_t *) &this->target_position;
    this->sps_.dedge = this->dedge_active_;

    gptimer_config_t timer_config = {};
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    timer_config.direction = GPTIMER_COUNT_UP;
    timer_config.resolution_hz = 1000000;  // 1 tick = 1 µs

    esp_err_t err = gptimer_new_timer(&timer_config, &this->step_timer_);
    if (err == ESP_OK) {
      gptimer_event_callbacks_t cbs = {};
      cbs.on_alarm = StepPulseStore::timer_isr;
      err = gptimer_register_event_callbacks(this->step_timer_, &cbs, &this->sps_);
    }
    if (err == ESP_OK)
      err = gptimer_enable(this->step_timer_);
    if (err == ESP_OK) {
      gptimer_alarm_config_t alarm = {};
      alarm.alarm_count = StepPulseStore::IDLE_TICK_US;
      err = gptimer_set_alarm_action(this->step_timer_, &alarm);
    }
    if (err == ESP_OK)
      err = gptimer_start(this->step_timer_);
    this->step_timer_ok_ = (err == ESP_OK);
    if (!this->step_timer_ok_) {
      ESP_LOGE(TAG, "Step pulse timer setup failed (err %d); falling back to loop-paced stepping", (int) err);
    }
#endif
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
#ifdef TMC2209_USE_STEP_TIMER
    if (this->step_timer_ok_) {
      // Hardware-paced stepping: the GPTimer ISR emits the STEP/DIR edges with
      // µs precision; here we only run the accel ramp and publish the current
      // step period. current_speed_ is the pulse frequency in steps/s directly.
      uint32_t interval_us = 0;
      if (this->current_speed_ > 0.0f && this->current_direction != Direction::STANDSTILL) {
        interval_us = (uint32_t) (1e6f / this->current_speed_);
        if (interval_us == 0)
          interval_us = 1;
      }
      this->sps_.interval_us = interval_us;
    } else
#endif
    // Legacy fallback: generate STEP pulses from the (high-frequency) main loop.
    // current_speed_ is the pulse frequency in steps/s directly; DEDGE is enabled
    // so every edge is one microstep. Pulse timing then inherits main-loop jitter,
    // so this only runs when the hardware step timer is unavailable.
    if (this->current_speed_ > 0.0f && this->current_direction != Direction::STANDSTILL) {
      const time_t interval = (time_t) (1e6f / this->current_speed_);
      if ((now - this->last_step_) >= interval) {
        if (this->direction_ != this->current_direction) {
          this->dir_pin_->digital_write(this->current_direction == Direction::BACKWARD);
          this->direction_ = this->current_direction;
        }
        if (this->dedge_active_) {
          // DEDGE enabled (UART): every edge is one microstep, so just toggle.
          this->step_pin_->digital_write(this->step_state_);
          this->step_state_ = !this->step_state_;
        } else {
          // Standalone (no UART): DEDGE is off, so only rising edges step. Emit a
          // full pulse per microstep. The high time only needs to exceed the
          // driver minimum (~100 ns); 3 us is safe and negligible at these rates.
          this->step_pin_->digital_write(true);
          delayMicroseconds(3);
          this->step_pin_->digital_write(false);
        }
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
  int32_t sgresult = 0;
  // A failed SG_RESULT read returns 0 = "fully stalled"; treating a UART glitch
  // as a stall would end StallGuard homing at a wrong position. Only trust a
  // parsed reply (see TMC2209Component::is_stalled).
  if (!this->read_register_checked(SG_RESULT, &sgresult)) {
    return false;
  }
  return (sgthrs << 1) > sgresult;
}

}  // namespace tmc2209
}  // namespace esphome
