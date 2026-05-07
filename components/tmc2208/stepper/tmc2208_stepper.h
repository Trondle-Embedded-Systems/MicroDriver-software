#pragma once
#include "esphome/components/tmc2208/tmc2208_api_registers.h"
#include "esphome/components/tmc2208/tmc2208_api.h"
#include "esphome/components/tmc2208/tmc2208_component.h"
#include "esphome/components/tmc2208/events.h"

#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/stepper/stepper.h"

namespace esphome {
namespace tmc2208 {

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

class TMC2208Stepper : public TMC2208Component, public Stepper {
 public:
  TMC2208Stepper() = default;
  TMC2208Stepper(uint8_t address) : TMC2208Component(address){};

  void dump_config() override;
  void setup() override;
  void IRAM_ATTR HOT loop() override;
  void on_shutdown() override;
  void stop() override;
  void enable(bool enable) override;
  // void enable(bool enable, bool recover_toff = true) override;
  void set_target(int32_t steps) override;

  void set_control_method(ControlMethod method) { this->control_method_ = method; }

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
  /* */

#ifdef USE_ESP32
  TaskHandle_t step_task_handle_{nullptr};
  volatile bool step_task_running_{false};
  static void IRAM_ATTR HOT step_task_(void *arg);
#endif
};

}  // namespace tmc2208
}  // namespace esphome
