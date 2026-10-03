#pragma once
#include "esphome/components/tmc2209/tmc2209_api_registers.h"
#include "esphome/components/tmc2209/tmc2209_api.h"
#include "esphome/components/tmc2209/tmc2209_component.h"
#include "esphome/components/tmc2209/events.h"

#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/stepper/stepper.h"

#if defined(USE_ESP32)
#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
// Hardware-paced STEP pulse generation (see StepPulseStore). Requires the
// GPTimer driver, which is only available from ESP-IDF v5 onwards.
#define TMC2209_USE_STEP_TIMER
#include <driver/gptimer.h>
#endif
#endif

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

#ifdef TMC2209_USE_STEP_TIMER
// Emits STEP/DIR edges from a GPTimer alarm ISR so pulse spacing is exact
// (µs-level) no matter how long a main-loop pass takes. The loop only runs the
// acceleration ramp and publishes interval_us; the ISR paces the pulses and is
// the sole owner of position while moving. Stepping stops by itself once
// current_position reaches target_position, so a stalled loop can never
// overshoot the target.
struct StepPulseStore {
  // Ticks are 1 µs. While standing still the alarm keeps firing at this slow
  // idle rate so motion can resume without task-context timer reconfiguration
  // (gptimer_set_alarm_action is only called from the ISR after setup).
  static constexpr uint32_t IDLE_TICK_US = 1000;

  ISRInternalGPIOPin step_pin;
  ISRInternalGPIOPin dir_pin;
  volatile int32_t *current_position{nullptr};
  volatile int32_t *target_position{nullptr};
  volatile uint32_t interval_us{0};  // step period in µs; 0 = standstill
  portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
  // Stop pulses if the main loop stops servicing the motion controller.
  uint64_t serviced_at{0};
  bool timed_out{false};
  bool dedge{false};                 // DEDGE on: one microstep per edge (toggle)
  // ISR-private state below
  bool step_state{false};
  int8_t last_dir{0};
  uint64_t next_step_at{0};  // absolute tick of the next step; 0 = restart fresh

  static bool IRAM_ATTR timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *arg);
};
#endif

class TMC2209Stepper : public TMC2209Component, public Stepper {
 public:
  TMC2209Stepper() = default;
  TMC2209Stepper(uint8_t address) : TMC2209Component(address){};

  void dump_config() override;
  void setup() override;
  // IRAM_ATTR/HOT only on the definition: repeating them here gives the two
  // declarations different IRAM section names (-Wattributes warning).
  void loop() override;
  void on_shutdown() override;
  // Ramps down to standstill (cancelling a seek) instead of halting within one
  // step; see begin_braking_().
  void stop() override;
  void enable(bool enable) override;
  // void enable(bool enable, bool recover_toff = true) override;
  // Cancels a running seek. A target behind the motor, or closer than it can
  // stop, is reached by braking to standstill first instead of reversing or
  // halting at speed.
  void set_target(int32_t steps) override;
  void on_update_speed() override;
  bool is_stalled() override;

  // Seek a mechanical end-stop without relying on the remembered step count.
  // Start with a short slow probe (so a door already at the stop is detected
  // after a gentle push instead of a full-speed run), then travel up to a full
  // door length at fast_speed, then continue slowly until StallGuard fires.
  // A confirmed StallGuard stall in any phase stops motion and declares the
  // end-stop immediately.
  void start_endstop_seek(Direction direction, int32_t travel_length, float fast_speed, float slow_speed,
                          int32_t endpoint_position);

  void set_control_method(ControlMethod method) { this->control_method_ = method; }

  void set_auto_disable_ms(uint32_t ms) { this->auto_disable_ms_ = ms; }
  void add_home_position(int32_t pos) {
    if (this->home_position_count_ < MAX_HOME_POSITIONS) {
      this->home_positions_[this->home_position_count_++] = pos;
      this->homing_enabled_ = true;
    }
  }
  void set_home_speed(float speed) { this->home_speed_ = speed; }
  // StallGuard threshold (SGTHRS) used by end-stop seeks, homing and obstacle
  // detection.
  void set_homing_sgthrs(uint8_t v) { this->homing_sgthrs_ = v; }
  void set_homing_tcoolthrs(uint32_t v) { this->homing_tcoolthrs_ = v; }

  // Stop ordinary moves (set_target) when StallGuard confirms the motor is
  // blocked at cruise speed, instead of forcing against the obstacle forever.
  void set_obstacle_detection(bool enable) { this->obstacle_detection_ = enable; }

  // True once an end-stop seek has located the door (or the position was set
  // by hand). Cleared at boot, after an obstacle stop and after a driver reset,
  // because steps may have been lost.
  bool is_position_known() const { return this->position_known_; }
  void set_position_known(bool known) { this->position_known_ = known; }

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
  volatile uint32_t last_step_{0};
  // DEDGE (one microstep per edge) can only be programmed over UART. Without a
  // hub it stays at the driver default (step on rising edge), so the loop must
  // emit a full pulse per microstep instead of toggling a single edge.
  bool dedge_active_{false};
#ifdef TMC2209_USE_STEP_TIMER
  StepPulseStore sps_;
  gptimer_handle_t step_timer_{nullptr};
  // When timer creation fails (or on frameworks without GPTimer) stepping falls
  // back to the legacy loop-paced path.
  bool step_timer_ok_{false};
#endif
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

  enum class EndstopSeekPhase : uint8_t { IDLE, PROBE, FAST_TRAVEL, SLOW_APPROACH };
  EndstopSeekPhase endstop_seek_phase_{EndstopSeekPhase::IDLE};
  Direction endstop_seek_direction_{Direction::STANDSTILL};
  float endstop_seek_fast_speed_{0.0f};
  float endstop_seek_slow_speed_{0.0f};
  int32_t endstop_seek_position_{0};
  int32_t endstop_seek_start_position_{0};
  int32_t endstop_seek_travel_length_{0};
  bool endstop_seek_fast_armed_{false};
  float pre_endstop_seek_max_speed_{0.0f};
  uint32_t pre_endstop_seek_sgthrs_{0};
  uint32_t pre_endstop_seek_tcoolthrs_{0};
  static constexpr int32_t ENDSTOP_STALL_ARM_STEPS = 32;
  // Slow pulses at the start of every seek. Must comfortably exceed
  // ENDSTOP_STALL_ARM_STEPS plus the confirmation samples at the slow speed.
  static constexpr int32_t ENDSTOP_PROBE_STEPS = 96;
  // Fast travel and ordinary moves only watch StallGuard once the ramp is at
  // cruise speed: accelerating the door's inertia reads as load and would
  // trigger falsely.
  static constexpr float CRUISE_ARM_SPEED_RATIO = 0.95f;

  // Shared StallGuard confirmation state (seek phases and obstacle detection
  // never run at the same time).
  int32_t stall_arm_position_{0};
  uint32_t last_stall_check_ms_{0};
  uint8_t consecutive_stalls_{0};
  static constexpr uint32_t STALL_POLL_INTERVAL_MS = 10;
  static constexpr uint8_t STALL_CONFIRMATIONS = 3;

  // Obstacle detection for ordinary moves
  bool obstacle_detection_{false};
  bool obstacle_armed_{false};
  float obstacle_min_speed_{0.0f};
  bool position_known_{false};

  // Controlled stop: brake to standstill, then run what was queued meanwhile.
  struct PendingSeek {
    Direction direction;
    int32_t travel_length;
    float fast_speed;
    float slow_speed;
    int32_t endpoint_position;
  };
  bool braking_{false};
  bool has_pending_target_{false};
  int32_t pending_target_{0};
  bool has_pending_seek_{false};
  PendingSeek pending_seek_{};

  // Speed/direction are only refreshed at the start of loop(), so they're stale
  // in the pass where the last step lands; a reached target means standstill.
  bool is_moving_() {
    return this->current_speed_ > 0.0f && this->current_direction != Direction::STANDSTILL && !this->has_reached_target();
  }
  int32_t stopping_distance_() const;
  void begin_braking_();
  void halt_();
  void abort_motion_();
  bool sg_stalled_(uint32_t sgthrs);
  void arm_stall_detection_();
  bool stall_confirmed_(uint32_t sgthrs);
  int32_t endstop_seek_target_(int64_t distance);
  void enter_endstop_fast_travel_();
  void enter_endstop_slow_approach_();
  void end_endstop_seek_();
  void finish_endstop_seek_(bool stalled);
  void stop_motion_();
  void set_target_locked_(int32_t steps);
};

}  // namespace tmc2209
}  // namespace esphome
