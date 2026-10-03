#include "tmc2209_stepper.h"
#include "esphome/components/tmc2209/tmc2209_api_registers.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

#include <algorithm>
#include <cmath>
#include <limits>

#ifdef TMC2209_USE_STEP_TIMER
#include <esp_rom_sys.h>
#endif

namespace esphome {
namespace tmc2209 {

#ifdef TMC2209_USE_STEP_TIMER
bool IRAM_ATTR StepPulseStore::timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata,
                                         void *arg) {
  auto *s = static_cast<StepPulseStore *>(arg);
  portENTER_CRITICAL_ISR(&s->lock);
  if (s->interval_us != 0 && edata->count_value - s->serviced_at > 250000) {
    s->interval_us = 0;
    s->timed_out = true;
  }
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
      esp_rom_delay_us(1);  // DIR must settle before the STEP edge.
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

  portEXIT_CRITICAL_ISR(&s->lock);
  // Keep a small margin so the new alarm can never land in the past (a missed
  // alarm would stop the timer chain entirely).
  uint64_t current_count = edata->count_value;
  gptimer_get_raw_count(timer, &current_count);
  const uint64_t min_next = current_count + 20;
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

  // A driver reset (supply dip) drops steps and reverts DEDGE/microstepping
  // until the registers are replayed, so the step count can't be trusted after.
  this->add_on_driver_status_callback([this](DriverStatusEvent event) {
    if (event == RESET && this->position_known_) {
      this->position_known_ = false;
      ESP_LOGW(TAG, "Driver reset: door position no longer known, run Home +/- to locate it again");
    }
  });

  // Targets explicitly enable the motor after on_boot configures its current.
  this->enable(false);

  ESP_LOGCONFIG(TAG, "TMC2209 Stepper setup done.");
}

void TMC2209Stepper::on_shutdown() { this->enable(false); }

void IRAM_ATTR HOT TMC2209Stepper::loop() {
  TMC2209Component::loop();

  // Compute speed and direction
  const uint32_t now = micros();
  this->calculate_speed_(now);
  const int32_t position = this->current_position;
  this->current_direction = this->target_position > position ? Direction::FORWARD :
      (this->target_position < position ? Direction::BACKWARD : Direction::STANDSTILL);

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
      uint64_t serviced_at = 0;
      gptimer_get_raw_count(this->step_timer_, &serviced_at);
      portENTER_CRITICAL(&this->sps_.lock);
      const bool timed_out = this->sps_.timed_out;
      this->sps_.serviced_at = serviced_at;
      this->sps_.interval_us = timed_out ? 0 : interval_us;
      portEXIT_CRITICAL(&this->sps_.lock);
      if (timed_out) {
        this->enable(false);
        this->on_driver_status_callback_.call(DRIVER_ERROR);
        ESP_LOGE(TAG, "Motion stopped: main loop unresponsive for 250 ms");
        return;
      }
    } else
#endif
    // Legacy fallback: generate STEP pulses from the (high-frequency) main loop.
    // current_speed_ is the pulse frequency in steps/s directly; DEDGE is enabled
    // so every edge is one microstep. Pulse timing then inherits main-loop jitter,
    // so this only runs when the hardware step timer is unavailable.
    if (this->current_speed_ > 0.0f && this->current_direction != Direction::STANDSTILL) {
      const uint32_t interval = (uint32_t) (1e6f / this->current_speed_);
      if (uint32_t(now - this->last_step_) >= interval) {
        if (this->direction_ != this->current_direction) {
          this->dir_pin_->digital_write(this->current_direction == Direction::BACKWARD);
          delayMicroseconds(1);
          this->direction_ = this->current_direction;
        }
        if (this->dedge_active_) {
          // DEDGE enabled (UART): every edge is one microstep, so just toggle.
          this->step_state_ = !this->step_state_;
          this->step_pin_->digital_write(this->step_state_);
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

  // Controlled stop finished: continue with whatever was requested meanwhile.
  if (this->braking_ && this->has_reached_target()) {
    this->braking_ = false;
    this->current_speed_ = 0.0f;  // start the queued move from rest
    if (this->has_pending_seek_) {
      this->has_pending_seek_ = false;
      const PendingSeek p = this->pending_seek_;
      this->start_endstop_seek(p.direction, p.travel_length, p.fast_speed, p.slow_speed, p.endpoint_position);
    } else if (this->has_pending_target_) {
      this->has_pending_target_ = false;
      this->set_target(this->pending_target_);
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
    this->set_target_locked_(this->homing_pending_target_);
    ESP_LOGI(TAG, "Homing complete at %d, proceeding to %d", confirmed_home, this->homing_pending_target_);
  }

  // Obstacle detection for ordinary moves. Same rules as the seek's fast
  // phase: arm at cruise speed, keep watching while still reasonably fast.
  if (this->obstacle_detection_ && this->endstop_seek_phase_ == EndstopSeekPhase::IDLE && !this->braking_ &&
      !this->is_homing_ && this->is_moving_()) {
    if (!this->obstacle_armed_) {
      if (this->current_speed_ >= this->max_speed_ * CRUISE_ARM_SPEED_RATIO) {
        this->arm_stall_detection_();
        this->obstacle_armed_ = true;
        this->obstacle_min_speed_ = this->current_speed_ * 0.5f;
      }
    } else if (this->current_speed_ >= this->obstacle_min_speed_ && this->stall_confirmed_(this->homing_sgthrs_)) {
      // Blocked: stop right away rather than ramping down against the obstacle.
      ESP_LOGW(TAG, "Obstacle: motor stalled at position %d, stopped", (int) this->current_position);
      this->halt_();
      this->position_known_ = false;
    }
  } else if (!this->is_moving_()) {
    this->obstacle_armed_ = false;
  }

  switch (this->endstop_seek_phase_) {
    case EndstopSeekPhase::IDLE:
      break;

    case EndstopSeekPhase::PROBE: {
      // A confirmed stall in any seek phase is the homing endpoint.
      const int64_t travelled = std::abs(static_cast<int64_t>(this->current_position) -
                                         static_cast<int64_t>(this->endstop_seek_start_position_));
      if (this->stall_confirmed_(this->homing_sgthrs_)) {
        this->finish_endstop_seek_(true);
      } else if (travelled >= ENDSTOP_PROBE_STEPS) {
        if (travelled < this->endstop_seek_travel_length_) {
          this->enter_endstop_fast_travel_();
        } else {
          this->enter_endstop_slow_approach_();
        }
      }
      break;
    }

    case EndstopSeekPhase::FAST_TRAVEL: {
      bool stalled = false;
      if (!this->endstop_seek_fast_armed_) {
        if (this->current_speed_ >= this->max_speed_ * CRUISE_ARM_SPEED_RATIO) {
          this->arm_stall_detection_();
          this->endstop_seek_fast_armed_ = true;
        }
      } else if (this->current_speed_ >= std::min(this->endstop_seek_slow_speed_, this->max_speed_)) {
        // Keep watching through the final deceleration while the speed is still
        // in the range StallGuard was tuned for (homing slow speed and above).
        stalled = this->stall_confirmed_(this->homing_sgthrs_);
      }

      if (stalled) {
        this->finish_endstop_seek_(true);
      } else if (this->has_reached_target()) {
        ESP_LOGI(TAG, "End-stop seek: fast travel complete, approaching at %.0f steps/s",
                 this->endstop_seek_slow_speed_);
        this->enter_endstop_slow_approach_();
      }
      break;
    }

    case EndstopSeekPhase::SLOW_APPROACH:
      if (this->stall_confirmed_(this->homing_sgthrs_)) {
        this->finish_endstop_seek_(true);
      } else if (this->has_reached_target()) {
        // This should only be reachable after an implausibly long movement or at
        // the int32 position limit. Stop instead of wrapping the position counter.
        ESP_LOGE(TAG, "End-stop seek stopped without a StallGuard event");
        this->finish_endstop_seek_(false);
      }
      break;
  }
}

static int32_t clamp_to_int32(int64_t value) {
  return static_cast<int32_t>(std::max<int64_t>(std::numeric_limits<int32_t>::min(),
                                                std::min<int64_t>(std::numeric_limits<int32_t>::max(), value)));
}

int32_t TMC2209Stepper::stopping_distance_() const {
  // Brake at the larger of the two ramp rates: controlled, but still prompt
  // when the configured deceleration is gentle.
  const float brake = std::max(this->acceleration_, this->deceleration_);
  const float v = this->current_speed_;
  return std::max<int32_t>(1, static_cast<int32_t>(std::ceil(v * v / (2.0f * brake))));
}

void TMC2209Stepper::begin_braking_() {
  // The ramp decelerates exactly as hard as needed to stop at its target (see
  // Stepper::calculate_speed_), so aiming at the stopping point is enough.
  this->braking_ = true;
  this->set_target_locked_(clamp_to_int32(static_cast<int64_t>(this->current_position) +
                                          static_cast<int64_t>(this->current_direction) * this->stopping_distance_()));
}

void TMC2209Stepper::halt_() {
  this->braking_ = false;
  this->has_pending_target_ = false;
  this->has_pending_seek_ = false;
  this->stop_motion_();
}

void TMC2209Stepper::abort_motion_() {
  if (this->endstop_seek_phase_ != EndstopSeekPhase::IDLE)
    this->end_endstop_seek_();
  this->halt_();
}

bool TMC2209Stepper::sg_stalled_(uint32_t sgthrs) {
  if (this->current_direction == Direction::STANDSTILL) {
    return false;
  }

  int32_t sgresult = 0;
  // A failed SG_RESULT read returns 0 = "fully stalled"; treating a UART glitch
  // as a stall would end StallGuard homing at a wrong position. Only trust a
  // parsed reply (see TMC2209Component::is_stalled).
  if (!this->read_register_checked(SG_RESULT, &sgresult)) {
    return false;
  }
  // Datasheet (SGTHRS): a stall is signaled with SG_RESULT <= SGTHRS*2.
  return sgresult <= static_cast<int32_t>(sgthrs << 1);
}

void TMC2209Stepper::arm_stall_detection_() {
  this->stall_arm_position_ = this->current_position;
  this->consecutive_stalls_ = 0;
  this->last_stall_check_ms_ = millis() - STALL_POLL_INTERVAL_MS;
}

bool TMC2209Stepper::stall_confirmed_(uint32_t sgthrs) {
  const uint32_t now_ms = millis();
  if ((now_ms - this->last_stall_check_ms_) < STALL_POLL_INTERVAL_MS)
    return false;
  this->last_stall_check_ms_ = now_ms;

  // SG_RESULT is not meaningful at standstill or during the first few
  // acceleration steps. Allow enough pulses since arming for StallGuard to
  // become valid, then require several consecutive matching samples.
  const int64_t steps_since_arm = std::abs(static_cast<int64_t>(this->current_position) -
                                           static_cast<int64_t>(this->stall_arm_position_));
  if (steps_since_arm < ENDSTOP_STALL_ARM_STEPS)
    return false;

  if (this->sg_stalled_(sgthrs)) {
    if (this->consecutive_stalls_ < STALL_CONFIRMATIONS)
      this->consecutive_stalls_++;
  } else {
    this->consecutive_stalls_ = 0;
  }
  return this->consecutive_stalls_ >= STALL_CONFIRMATIONS;
}

int32_t TMC2209Stepper::endstop_seek_target_(int64_t distance) {
  return clamp_to_int32(static_cast<int64_t>(this->current_position) +
                        static_cast<int64_t>(this->endstop_seek_direction_) * distance);
}

void TMC2209Stepper::enter_endstop_fast_travel_() {
  // The probe steps count towards the travel length, so the total distance
  // covered before the slow approach is still one full door length.
  const int64_t travelled = std::abs(static_cast<int64_t>(this->current_position) -
                                     static_cast<int64_t>(this->endstop_seek_start_position_));
  this->endstop_seek_phase_ = EndstopSeekPhase::FAST_TRAVEL;
  this->endstop_seek_fast_armed_ = false;
  this->set_max_speed(this->endstop_seek_fast_speed_);
  this->set_target_locked_(this->endstop_seek_target_(this->endstop_seek_travel_length_ - travelled));
}

void TMC2209Stepper::enter_endstop_slow_approach_() {
  this->endstop_seek_phase_ = EndstopSeekPhase::SLOW_APPROACH;
  this->set_max_speed(this->endstop_seek_slow_speed_);
  this->arm_stall_detection_();
  // Keep seeking in the same direction. One billion steps is deliberately
  // finite so the base stepper's signed distance calculation cannot overflow.
  this->set_target_locked_(this->endstop_seek_target_(1000000000LL));
}

void TMC2209Stepper::start_endstop_seek(Direction direction, int32_t travel_length, float fast_speed,
                                        float slow_speed, int32_t endpoint_position) {
  if (direction == Direction::STANDSTILL || travel_length <= 0 || fast_speed <= 0.0f || slow_speed <= 0.0f) {
    ESP_LOGE(TAG, "End-stop seek requires a direction, positive travel length, and positive speeds");
    return;
  }
  if (!this->bus_enabled()) {
    ESP_LOGE(TAG, "End-stop seek requires a working UART connection for StallGuard");
    return;
  }

  // Cancel any prior seek/homing and restore its temporary settings first.
  if (this->endstop_seek_phase_ != EndstopSeekPhase::IDLE)
    this->end_endstop_seek_();
  if (this->is_homing_) {
    this->set_max_speed(this->pre_homing_max_speed_);
    this->write_register(SGTHRS, this->pre_homing_sgthrs_);
    this->write_register(TCOOLTHRS, this->pre_homing_tcoolthrs_);
    this->is_homing_ = false;
  }
  // Still moving (e.g. Close pressed while the door is opening): brake to
  // standstill first; loop() starts the seek from there.
  if (this->is_moving_()) {
    this->has_pending_target_ = false;
    this->pending_seek_ = {direction, travel_length, fast_speed, slow_speed, endpoint_position};
    this->has_pending_seek_ = true;
    if (!this->braking_)
      this->begin_braking_();
    ESP_LOGI(TAG, "End-stop seek: braking to standstill first");
    return;
  }
  this->halt_();

  this->pre_endstop_seek_max_speed_ = this->max_speed_;
  this->pre_endstop_seek_sgthrs_ = this->read_register(SGTHRS);
  this->pre_endstop_seek_tcoolthrs_ = this->read_register(TCOOLTHRS);
  this->write_register(SGTHRS, this->homing_sgthrs_);
  this->write_register(TCOOLTHRS, this->homing_tcoolthrs_);

  this->endstop_seek_direction_ = direction;
  this->endstop_seek_fast_speed_ = fast_speed;
  this->endstop_seek_slow_speed_ = slow_speed;
  this->endstop_seek_position_ = endpoint_position;
  this->endstop_seek_start_position_ = this->current_position;
  this->endstop_seek_travel_length_ = travel_length;
  this->endstop_seek_phase_ = EndstopSeekPhase::PROBE;
  this->arm_stall_detection_();
  this->set_max_speed(slow_speed);
  this->enable(true);
  this->auto_disabled_ = false;

  // The probe aims far ahead and is ended by step count, so the ramp never
  // decelerates inside it (SG_RESULT is unreliable at low speed). Switching to
  // fast travel then just raises max speed and accelerates seamlessly.
  this->set_target_locked_(this->endstop_seek_target_(1000000000LL));
  ESP_LOGI(TAG, "End-stop seek: probing %d steps at %.0f steps/s, then up to %d steps at %.0f steps/s",
           static_cast<int>(direction) * ENDSTOP_PROBE_STEPS, slow_speed, static_cast<int>(direction) * travel_length,
           fast_speed);
}

// Leaves the seek state and restores the settings it changed, without touching
// the current motion (callers decide whether to halt, brake or re-target).
void TMC2209Stepper::end_endstop_seek_() {
  this->set_max_speed(this->pre_endstop_seek_max_speed_);
  this->write_register(SGTHRS, this->pre_endstop_seek_sgthrs_);
  this->write_register(TCOOLTHRS, this->pre_endstop_seek_tcoolthrs_);
  this->endstop_seek_phase_ = EndstopSeekPhase::IDLE;
  this->endstop_seek_direction_ = Direction::STANDSTILL;
}

void TMC2209Stepper::finish_endstop_seek_(bool stalled) {
  this->halt_();
  this->end_endstop_seek_();

  if (stalled) {
    this->report_position(this->endstop_seek_position_);
    this->set_target_locked_(this->endstop_seek_position_);
    this->position_known_ = true;
    // Release ENN immediately after the mechanical stop is confirmed. Bypass
    // the stepper override because motion is already stopped and the seek state
    // has already been cleared above.
    TMC2209Component::enable(false);
    this->auto_disabled_ = true;
    ESP_LOGI(TAG, "End-stop found; position set to %d and driver disabled", this->endstop_seek_position_);
  }
}

void TMC2209Stepper::set_target(int32_t steps) {
  if (this->is_failed())
    return;
  if (this->control_method_ == ControlMethod::CONTROL_UNSET) {
    ESP_LOGE(TAG, "Control method not set!");
  }

  // A new motion command supersedes a running or queued end-stop seek. Cancel
  // it first, otherwise the seek state machine would re-target the motor on its
  // next phase change and keep the homing speed/StallGuard settings applied.
  if (this->endstop_seek_phase_ != EndstopSeekPhase::IDLE) {
    ESP_LOGI(TAG, "End-stop seek cancelled by a new target");
    this->end_endstop_seek_();
  }
  this->has_pending_seek_ = false;
  this->obstacle_armed_ = false;

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
        this->set_target_locked_(nearest);
        ESP_LOGI(TAG, "StallGuard homing: nearest end-stop %d at %.0f steps/s", nearest, this->home_speed_);
        return;
      }
    }
    this->enable(true);
  }
  this->auto_disabled_ = false;
  // Never reverse or end a move within one step: a target behind the motor, or
  // closer than it can stop, is reached by braking to standstill first.
  if (this->is_moving_()) {
    const int64_t ahead = (static_cast<int64_t>(steps) - static_cast<int64_t>(this->current_position)) *
                          static_cast<int64_t>(this->current_direction);
    if (ahead < this->stopping_distance_()) {
      this->pending_target_ = steps;
      this->has_pending_target_ = true;
      if (!this->braking_)
        this->begin_braking_();
      return;
    }
  }
  this->braking_ = false;
  this->has_pending_target_ = false;
  this->set_target_locked_(steps);
}

void TMC2209Stepper::on_update_speed() {
  // stepper.set_speed during a seek: keep the seek's own speed for now and
  // apply the new value when the seek ends. Otherwise it would be overwritten
  // by the speed the seek saved at its start.
  if (this->endstop_seek_phase_ == EndstopSeekPhase::IDLE)
    return;
  this->pre_endstop_seek_max_speed_ = this->max_speed_;
  this->set_max_speed(this->endstop_seek_phase_ == EndstopSeekPhase::FAST_TRAVEL ? this->endstop_seek_fast_speed_
                                                                                 : this->endstop_seek_slow_speed_);
}

void TMC2209Stepper::stop() {
  if (this->endstop_seek_phase_ != EndstopSeekPhase::IDLE)
    this->end_endstop_seek_();
  this->has_pending_target_ = false;
  this->has_pending_seek_ = false;
  if (this->is_moving_()) {
    if (!this->braking_)
      this->begin_braking_();
  } else {
    this->halt_();
  }
}

void TMC2209Stepper::set_target_locked_(int32_t steps) {
#ifdef TMC2209_USE_STEP_TIMER
  portENTER_CRITICAL(&this->sps_.lock);
#endif
  Stepper::set_target(steps);
#ifdef TMC2209_USE_STEP_TIMER
  portEXIT_CRITICAL(&this->sps_.lock);
#endif
}

void TMC2209Stepper::stop_motion_() {
#ifdef TMC2209_USE_STEP_TIMER
  portENTER_CRITICAL(&this->sps_.lock);
  this->sps_.interval_us = 0;
  this->sps_.timed_out = false;
#endif
  Stepper::stop();
#ifdef TMC2209_USE_STEP_TIMER
  portEXIT_CRITICAL(&this->sps_.lock);
#endif
  if (this->control_method_ == ControlMethod::SERIAL_CONTROL) {
    this->write_field(VACTUAL_FIELD, 0);
    this->vactual_ = 0;
  }
}

void TMC2209Stepper::enable(bool enable) {
  // Cutting the coils leaves nothing to ramp down: stop at once.
  if (!enable) {
    this->abort_motion_();
  }
  TMC2209Component::enable(enable);
}

bool TMC2209Stepper::is_stalled() { return this->sg_stalled_(this->read_register(SGTHRS)); }

}  // namespace tmc2209
}  // namespace esphome
