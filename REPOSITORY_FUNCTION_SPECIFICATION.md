# MicroDriver repository function specification

## Document control

| Item | Value |
| --- | --- |
| Purpose | Explain the present repository behavior and collect requested changes. |
| Source issue | ASD-STE100 Simplified Technical English, Issue 9 |
| Conformance label | `structure-checked` |
| Text type | Mixed description and procedure |
| Spelling | American English |
| Human review | Required |
| Last repository review | 2026-08-30 |

This document uses a derived Issue 9 implementation profile. It is not an official ASD publication.

The official Issue 9 dictionary was not available for this review. The project also has no approved domain terminology list.

Thus, this document cannot have the `candidate-ste` label. An authorized reviewer must approve terminology and final conformance.

Identifiers, file names, register names, pin names, values, and units are protected content. This document does not change their spelling.

## How to request changes

1. Read each feature section.
2. Edit the **Requested change** cells.
3. Use `KEEP` when you want the present behavior.
4. Use `REMOVE` when you do not want the feature.
5. Write exact new values when you want different values.
6. Add requirements to the change-request table.
7. Do not delete an identified risk unless you accept the risk.

After your review, ask Codex to implement the edited requirements. Code changes must preserve unchanged values, identifiers, sequences, and safety intent.

## Scope

The review includes repository-owned Python, C++, YAML, Markdown, and manifest files. It excludes generated files below `components/husb238_i2c/.esphome/`.

The review treats PDF datasheets, images, drawings, and the Tellegan netlist as reference artifacts. It does not rewrite these protected artifacts.

The repository contains ESPHome external components. It also contains two device configurations for the MicroDriver hardware.

## Repository map

| Area | Present function | Maturity | Requested change |
| --- | --- | --- | --- |
| `microdriver_betea_1.yaml` | General motor controller with position, jog, and positive homing controls. | Active product configuration | `KEEP` |
| `microdriver_door.yaml` | Door cover controller with positive and negative StallGuard end-stops. | Active product configuration | `KEEP` |
| `stepper` | Common position, speed, acceleration, and stop functions. | Active base component | `KEEP` |
| `tmc2209` and `tmc2209_hub` | UART register control, STEP/DIR motion, diagnostics, current control, and StallGuard. | Most developed driver | `KEEP` |
| `husb238_i2c` | USB Power Delivery negotiation and status sensors. | Active in both products | `KEEP` |
| `tmc2208` and `tmc2208_hub` | UART driver control and stepper motion. | Older implementation | `KEEP` |
| `tmc2300` | UART driver control, stepper motion, diagnostics, and sensors. | Older implementation | `KEEP` |
| `stepper_closed_loop` | Encoder correction for a stepper position. | Implemented, but not used by product YAML | `KEEP` |
| `as5047` | SPI magnetic encoder framework. | Incomplete | `KEEP` |
| `icm20948`, `icm20948_i2c`, `icm20948_spi` | IMU framework and bus variants. | Incomplete and unsafe | `KEEP` |
| `tmc5240`, `tmc5240_spi`, `tmc5240_uart` | TMC5240 register and transport framework. | Experimental | `KEEP` |
| `insight` | ESPHome loop-frequency sensor. | Small diagnostic component | `KEEP` |
| `freematics` | Planned Freematics OBD interface. | Nonfunctional stub | `KEEP` |

## Product behavior

### `microdriver_betea_1.yaml`

This configuration creates an ESP32-S3 device named `microdriver`. It uses the ESP-IDF framework.

The device connects to Wi-Fi. It also supplies a fallback access point, encrypted ESPHome API, OTA updates, and a port `80` web server.

The logger uses `USB_SERIAL_JTAG` at `115200` baud. The I2C bus uses `GPIO17` for SDA and `GPIO21` for SCL.

The TMC2209 UART uses `GPIO7` for transmit and receive. The configuration makes the shared pin open-drain with an internal pull-up.

The motor uses these pins:

| Function | Pin or value | Requested change |
| --- | --- | --- |
| STEP | `GPIO35` | `KEEP` |
| DIR | `GPIO34` | `KEEP` |
| ENN | `GPIO9` | `KEEP` |
| DIAG | `GPIO4` | `KEEP` |
| UART | `GPIO7` | `KEEP` |
| Sense resistance | `110 mOhm` | `KEEP` |
| Driver clock | `12000000` | `KEEP` |

The boot automation sets `8` microsteps and enables interpolation. It sets `0.5 A` run current and `0.2 A` hold current.

The boot automation selects `freewheeling` at standstill. This mode removes coil holding torque at standstill.

The default motion limits are `900 steps/s`, `1500 steps/s^2`, and `500 steps/s^2`. The three values control speed, acceleration, and deceleration.

The `home_to_plus` script moves toward a positive mechanical stop. It uses a StallGuard threshold of `50` at `400 steps/s`.

The script sets `TCOOLTHRS` to `500000`. A DIAG event stops motion, sets position `0`, disarms StallGuard, and restores `900 steps/s`.

The device supplies these controls:

| Control | Present behavior | Requested change |
| --- | --- | --- |
| `Stop Motor` | Stop motion and cancel homing. | `KEEP` |
| `Home +` | Run `home_to_plus`. | `KEEP` |
| `Move 1000 Steps Forward` | Add `1000` to the target position. | `KEEP` |
| `Move 1000 Steps Backward` | Subtract `1000` from the target position. | `KEEP` |
| `Reset Position` | Set the reported position to `0`. | `KEEP` |
| `Target Position` | Accept a target from `-100000` through `100000`. | `KEEP` |
| `Max Speed` | Accept `50` through `2000 steps/s`. | `KEEP` |
| `Jog Speed` | Run continuously from `-2000` through `2000 steps/s`. | `KEEP` |
| `Current Position` | Publish changed positions each second. | `KEEP` |
| `status_led` | Expose `GPIO18` as a normal GPIO output. | `KEEP` |

The HUSB238 requests `12 V`. It publishes the negotiated voltage and available current.

### `microdriver_door.yaml`

This configuration creates an ESP32-S3 device named `microdriver_door`. Its network, logger, API, OTA, web, UART, and I2C settings match the first product.

The motor uses the same control pins. It starts disabled and uses the same current, microstep, and sense-resistor values.

The `home` script accepts a signed `direction`. It moves toward the selected end-stop until StallGuard reports a stall.

The script enables the motor before motion. The stall handler stops and disables the motor after the end-stop event.

Positive motion opens the door. Negative motion closes the door.

The `Door` cover entity supplies Open, Close, and Stop commands. It publishes the applicable endpoint after a StallGuard event.

The maximum run time is `30s`. A timeout stops and disables the motor when StallGuard does not detect an end-stop.

The direction-change pause is `250ms`. This pause lets the motion state return to standstill before the motor reverses.

The user can set `Speed` from `50` through `1000 steps/s`. The initial speed is `400 steps/s`.

| Control | Present behavior | Requested change |
| --- | --- | --- |
| `Open Door` | Move toward the positive open end-stop. | `KEEP` |
| `Close Door` | Move toward the negative closed end-stop. | `KEEP` |
| `Stop Door` | Stop motion, disarm StallGuard, and disable the motor. | `KEEP` |
| `Speed` | Set the homing speed for both directions. | `KEEP` |
| `Door` | Supply Open, Close, and Stop cover commands. | `KEEP` |

The HUSB238 requests `12 V`. This configuration does not publish its voltage or current sensors.

See `DOOR_CONTROL_GUIDE.md` for the control sequence, tuning procedure, safety information, and acceptance test.

## Function catalog

This catalog covers repository-owned callable functions. It groups identical setters and generated action methods to keep the catalog usable.

### Common `stepper` functions

| Function | Present function |
| --- | --- |
| `validate_acceleration` | Remove an accepted unit suffix, parse the value, and require a positive value. |
| `validate_speed` | Remove an accepted unit suffix, parse the value, and require a positive value. |
| `setup_stepper_core_` | Send acceleration, deceleration, and maximum speed to the C++ object. |
| `register_stepper` | Register the C++ stepper object with ESPHome code generation. |
| `stepper_set_target_to_code` | Build the `stepper.set_target` automation action. |
| `stepper_report_position_to_code` | Build the `stepper.report_position` automation action. |
| `stepper_set_speed_to_code` | Build the `stepper.set_speed` automation action. |
| `stepper_set_acceleration_to_code` | Build the `stepper.set_acceleration` automation action. |
| `stepper_set_deceleration_to_code` | Build the `stepper.set_deceleration` automation action. |
| `stepper_stop_to_code` | Build the `stepper.stop` automation action. |
| `to_code` | Add the C++ `stepper` namespace to generated code. |
| `Stepper::set_target` | Store a new absolute target position. |
| `Stepper::report_position` | Replace the current reported position. |
| `Stepper::set_acceleration` | Store the acceleration limit. |
| `Stepper::set_deceleration` | Store the deceleration limit. |
| `Stepper::set_max_speed` | Store the speed limit. |
| `Stepper::on_update_speed` | Supply an empty extension point after a speed change. |
| `Stepper::has_reached_target` | Compare the current position with the target position. |
| `Stepper::stop` | Make the target equal the current position and set standstill. |
| `Stepper::enable` | Supply an empty base extension point for driver enable control. |
| `Stepper::calculate_speed_` | Apply the acceleration or deceleration ramp and clamp speed. |
| `Stepper::should_step_` | Decide whether one step is due and update the software position. |
| Action constructors | Store the parent stepper for each ESPHome action. |
| Action `play` methods | Apply target, position, speed, acceleration, deceleration, or stop commands. |

### TMC hub functions

These functions exist for `tmc2208_hub` and `tmc2209_hub` unless the table identifies one driver.

| Function | Present function |
| --- | --- |
| Python `to_code` | Create the hub and connect it to an ESPHome UART device. |
| Python `register_tmc*_hub_device` | Attach one driver to its selected hub. |
| Python `final_validate` | Check UART requirements and report duplicate driver addresses. |
| `TMC*Hub::get_setup_priority` | Start the hub at ESPHome bus priority. |
| `TMC*Hub::setup` | Log hub startup. |
| `TMC*Hub::dump_config` | Log all registered driver identifiers and addresses. |
| `TMC*Hub::add_device_to_hub_` | Add one identifier and address to the hub inventory. |

### TMC register API functions

The shared API applies to TMC2208, TMC2209, and TMC2300. The implementation maintains a shadow cache for writable registers.

| Function | Present function |
| --- | --- |
| `set_address` and `get_address` | Set or get the UART node address. TMC2300 sets its address through construction. |
| `write_register` | Validate access, update the cache, and send one register value. |
| `read_register` | Read hardware when possible or return the cached value for write-only registers. |
| `write_field` | Read the register image, replace one field, and write the result. |
| `read_field` | Read a register and extract one field. |
| `extract_field` | Apply the field mask, shift, and signed conversion. |
| `update_field` | Replace a field in a supplied register value. |
| `cache_` | Read or write one entry in the shadow cache. |
| `set_dirty_bit_` and `get_dirty_bit_` | Record whether the host changed a cached register. |
| `crc8_` | Calculate the Trinamic UART CRC. |
| `read_write_register_` | Run one combined TMC2300 UART exchange. |
| `init_cache_` | Declare a TMC2300 cache initializer. No separate definition is visible. |

TMC2209 adds these communication functions:

| Function | Present function |
| --- | --- |
| `set_bus_enabled` and `bus_enabled` | Enable or disable all register-bus operations. |
| `flush_rx_` | Remove stale UART receive bytes. |
| `send_datagram_` | Send a datagram and discard the shared-wire echo. |
| `wait_available_` | Wait for a bounded number of receive bytes. |
| `read_register_once_` | Make one checked register-read attempt. |
| `read_register_checked` | Return a success flag and a register value. |
| `replay_dirty_registers` | Restore all host-written registers after a driver reset. |

### TMC component functions

The table applies to `TMC2208Component`, `TMC2209Component`, and `TMC2300Component` where the named function exists.

| Function group | Present function |
| --- | --- |
| `setup` | Set pins, test communication, set required registers, and install diagnostic callbacks. |
| `loop` | Process DIAG state when health or stall checks are enabled. |
| `pin_isr` | Record a DIAG interrupt for later processing. |
| Pin setters | Store ENN, DIAG, INDEX, STEP, and DIR pin objects. |
| `set_clk_freq` | Store the driver clock and calculate the VACTUAL conversion factor. |
| `set_rsense` | Store the motor-current sense resistance. |
| Current-mode setters | Store analog-scale, VSENSE, and OTTRIM selections. |
| Diagnostic setters | Enable health checks, stall checks, register dumps, or TOFF recovery. |
| Callback adders | Add callbacks for stall events or driver-status events. |
| `enable` | Control ENN or TOFF and record the enabled state. |
| `set_microsteps` | Convert a supported microstep count to the MRES register value. |
| `get_microsteps` | Convert the MRES register value to a microstep count. |
| `is_stalled` | Compare StallGuard threshold and result values. TMC2209 rejects failed reads. |
| `get_motor_load` | Convert StallGuard values to a load ratio. TMC2208 does not supply this function. |
| `read_vsense` | Return the selected sense voltage. TMC2300 declares this function without a visible definition. |
| `current_scale_to_rms_current_mA` | Convert a current-scale code to RMS current. |
| `rms_current_to_current_scale_mA_no_clamp` | Convert RMS current to an unclamped current-scale code. |
| `rms_current_to_current_scale_mA` | Convert RMS current and clamp the result to the driver range. |
| Run-current functions | Read or write run current in milliamperes or amperes. |
| Hold-current functions | Read or write hold current in milliamperes or amperes. |
| `set_tpowerdown_ms` and `get_tpowerdown_ms` | Convert between milliseconds and the TPOWERDOWN register value. |
| `unpack_ottrim_values` | Convert OTTRIM into warning and shutdown temperatures. |
| `vactual_to_speed` and `speed_to_vactual` | Convert between driver VACTUAL units and steps per second. |
| `read_speed` and `write_speed` | Read or write velocity through VACTUAL. |

The TMC2209 `setup` function supports operation without UART. Standalone operation uses STEP, DIR, ENN, hardware microstep pins, and hardware current settings.

The TMC2209 setup disables its register bus after a failed probe. This behavior prevents repeated UART timeouts during the remaining boot sequence.

The TMC2209 reset callback replays changed registers. This behavior restores digital current and motion settings after a driver power reset.

### TMC stepper functions

| Function | Present function |
| --- | --- |
| Python `validate_control_method_` | Select serial control or pulse control from the configured pins. |
| TMC2209 Python `validate_standalone_` | Require valid STEP, DIR, and ENN pins when no UART hub exists. |
| Python `to_code` | Create the driver, register the stepper, and select its control method. |
| `dump_config` | Log motion limits, pins, driver settings, currents, and optional registers. |
| `setup` | Initialize the driver and configure serial or pulse motion. |
| `loop` | Update the speed ramp, direction, motion output, diagnostics, and optional homing. |
| `on_shutdown` | Stop TMC2208 or TMC2209 motion during shutdown. |
| `set_target` | Enable the motor and set a new absolute target. |
| `stop` | Stop the base stepper and set VACTUAL to `0` for serial control. |
| `enable` | Stop before disable, then call the driver enable function. |
| `is_stalled` | Ignore standstill and compare StallGuard values. |
| `IndexPulseStore::pulse_isr` | Update serial-control position from INDEX edges. |
| TMC2208 `step_task_` | Generate pulse-control edges on ESP32 core `0`. |
| TMC2209 `StepPulseStore::timer_isr` | Generate drift-resistant STEP pulses with an ESP-IDF GPTimer. |

TMC2209 supplies optional auto-disable and re-homing setters. These are `set_auto_disable_ms`, `add_home_position`, `set_home_speed`, `set_homing_sgthrs`, and `set_homing_tcoolthrs`.

### TMC automation actions

Each Python `*_to_code` function validates YAML and creates its matching C++ action. Each C++ `play` function writes the selected settings.

| Action | Drivers | Present function |
| --- | --- | --- |
| `enable` and `disable` | TMC2208, TMC2209, TMC2300 | Change driver activation and optional TOFF recovery. |
| `configure` | TMC2208, TMC2209, TMC2300 | Set direction, microsteps, interpolation, thresholds, and chopper mode. |
| `currents` | TMC2208, TMC2209, TMC2300 | Set run current, hold current, delays, and standstill mode. |
| `stallguard` | TMC2209, TMC2300 | Set the StallGuard threshold. |
| `coolconf` | TMC2209, TMC2300 | Set CoolStep control fields. |
| `chopconf` | TMC2208, TMC2209, TMC2300 | Set chopper timing fields. |
| `pwmconf` | TMC2208, TMC2209, TMC2300 | Set StealthChop PWM fields. |
| `sync` | TMC2208, TMC2209 | Copy selected registers from one driver to other drivers. |

`OnDriverStatusTrigger` starts an automation for a driver-status code. `OnStallTrigger` starts an automation after a stall callback.

`EventHandler::set_on_rise_callback` stores a rising-edge callback. `set_on_fall_callback` stores a falling-edge callback.

`EventHandler::set_callbacks` stores both callbacks. `EventHandler::check` runs a callback only after a state transition.

### TMC status events

The TMC event lists include DIAG, reset, driver error, charge-pump undervoltage, temperature, open-load, low-side-short, and ground-short states.

Most fault groups include phase `A`, phase `B`, combined, active, and cleared events. TMC2300 hardware supports a smaller temperature subset.

### TMC sensor functions

Each sensor `to_code` function creates a polling sensor and attaches it to a driver. Each `dump_config` function logs the sensor.

| Sensor `update` function | Drivers | Published value |
| --- | --- | --- |
| `StallGuardResultSensor::update` | TMC2209, TMC2300 | Raw `SG_RESULT` value. |
| `MotorLoadSensor::update` | TMC2209, TMC2300 | Calculated motor load as a percentage. |
| `ActualCurrentSensor::update` | TMC2208, TMC2209, TMC2300 | Calculated RMS current in milliamperes. |
| `PWMScaleSumSensor::update` | TMC2208, TMC2209, TMC2300 | `PWM_SCALE_SUM_FIELD`. |
| `PWMScaleAutoSensor::update` | TMC2208, TMC2209, TMC2300 | `PWM_SCALE_AUTO_FIELD`. |
| `PWMOFSAutoSensor::update` | TMC2208, TMC2209, TMC2300 | `PWM_OFS_AUTO_FIELD`. |
| `PWMGradAutoSensor::update` | TMC2208, TMC2209, TMC2300 | `PWM_GRAD_AUTO_FIELD`. |

### HUSB238 functions

| Function | Present function |
| --- | --- |
| Python `to_code` | Create the controller, attach I2C, create sensors, and store the voltage request. |
| Child-sensor `update` methods | Do nothing because the parent controller publishes sensor states. |
| `HUSB238::setup` | Read `PD_STATUS0` and fail setup when the device does not respond. |
| `HUSB238::update` | Publish status and make a bounded voltage request after source attachment. |
| `HUSB238::dump_config` | Log I2C, sensor, interval, and requested-voltage settings. |
| Sensor setters | Store output-voltage, output-current, and input-voltage sensor pointers. |
| `set_request_voltage` | Store an HUSB238 PDO selection code. |
| `negotiated_to_selection` | Convert a negotiated-voltage code to a PDO selection code. |
| `best_available_selection` | Find the highest advertised voltage that does not exceed the request. |
| `request_voltage` | Write `SRC_PDO`, then write `GO_COMMAND`. |
| `update_status` | Decode and publish the negotiated voltage and available current. |
| `decode_voltage` | Convert the status nibble to `5`, `9`, `12`, `15`, `18`, or `20 V`. |
| `decode_current` | Convert the status nibble to `0.5` through `5.0 A`. |
| `read_register` and `write_register` | Convert ESPHome I2C results to Boolean success values. |

The `input_voltage` sensor repeats the negotiated VBUS value. The HUSB238 does not measure a separate upstream input voltage.

### Closed-loop stepper functions

| Function | Present function |
| --- | --- |
| Python `to_code` | Create the controller and connect its stepper, encoder, settings, and optional error sensor. |
| `setup` | Store the initial target. |
| `dump_config` | Log all correction and auto-disable settings. |
| Configuration setters | Store linked objects and all correction settings. |
| `steps_per_count_` | Calculate motor steps for one encoder count. |
| `encoder_to_steps_` | Convert the multi-turn encoder position to motor steps. |
| `update` | Track encoder wrap, correct position, publish error, and control optional disable behavior. |

The controller changes `current_position` when the encoder error exceeds the threshold. It rejects errors above `max_correction` as suspect readings.

### AS5047 functions

| Function | Present function |
| --- | --- |
| Python component `to_code` | Create the SPI device. |
| Python sensor `to_code` | Create an angle sensor and attach it to the encoder. |
| `dump_config` | Log the SPI chip-select pin. |
| `setup` | Start SPI, clear error state, and test the error register. |
| `clear_error_` | Replace the stored error with code `0`. |
| `calc_parity_` | Calculate one parity bit. |
| `is_parity_ok_` | Compare received and calculated parity. |
| `read_write_raw` | Transfer one 16-bit SPI frame and validate error and parity bits. |
| `read_position` | Attempt a compensated-angle read and optional diagnostic checks. |
| `handle_error_` | Store the first error code and its message. |
| Stub functions | Return `0` without their named hardware operation. |
| `AngleSensor::dump_config` | Log the angle sensor. |
| `AngleSensor::update` | Calls `publish_state` without a value. This code is incomplete. |

The stub functions are `read_register`, `write_register`, `set_factory_settings`, `error_pending`, `error_ack`, and `set_field_in_register`.

The remaining stub functions are `set_zero_position`, `set_abi_resolution`, and `burn_otp`. The present setup depends on the stubbed `read_register` function.

### ICM20948 functions

| Function | Present function |
| --- | --- |
| `register_icm20948_device` | Create configured acceleration, gyro, and temperature sensor objects. |
| I2C `to_code` | Create the IMU and register its default `0x68` I2C address. |
| SPI `to_code` | Create an untested SPI IMU and log an error during code generation. |
| Sensor setters | Store seven optional sensor pointers. |
| `read_icid` | Read the `WHOAMI` register. The requested length is unsafe for its buffer. |
| `read_b0` | Read `PWR_MGMT_1`. The requested length is unsafe for its buffer. |
| `update` | Read and log raw temperature bytes. It does not publish configured sensors. |
| I2C `read` and `write` | Forward register transfers to ESPHome I2C. |
| I2C `dump_config` | Log sensor and I2C information. |
| SPI `setup` | Start the SPI device. |
| SPI `dump_config` | Log SPI settings and the chip identifier. |
| SPI `on_shutdown` | Stop the SPI device. |

The SPI class does not implement the base `read` and `write` functions. Thus, it remains abstract and is not a complete bus implementation.

### TMC5240 functions

| Function | Present function |
| --- | --- |
| `register_tmc5240` | Register the component and attach optional ENN and DIAG pins. |
| `register_tmc5240_stepper` | Register the base component and common stepper settings. |
| SPI and UART `to_code` | Create a transport-specific stepper and register its bus. |
| `setup` | Configure pins and report an ADC error. |
| `loop` | Log supply voltage on every loop pass. |
| `dump_config` | Log pins, version, ADC status, voltage, and a test VACTUAL write. |
| Pin setters | Store ENN, DIAG0, and DIAG1 pin objects. |
| `read_field` | Read one register and extract one field. |
| `write_field` | Attempt a read-modify-write operation. The implementation does not use its `value` parameter. |
| `extract_field` | Extract and sign-extend one field. |
| ADC readers | Convert supply, analog-input, and temperature fields to engineering values. |
| `get_vactual` | Read and sign-extend the 23-bit VACTUAL value. |
| SPI `read_register` and `write_register` | Transfer five-byte register frames over SPI. |
| UART `read_register` and `write_register` | Attempt five-byte register transfers over UART. |

The TMC5240 stepper class has no motion overrides. The SPI `setup` function does not call the TMC5240 base setup.

The UART read function calls a one-byte API with a five-byte array. The remaining bytes do not receive a complete response.

### Insight functions

| Function | Present function |
| --- | --- |
| Python `to_code` | Create and register the diagnostic sensor. |
| `dump_config` | Log the loop-frequency sensor. |
| `setup` | Request a high-frequency ESPHome loop. |
| `loop` | Measure microseconds between consecutive loop calls. |
| `update` | Publish `1000000 / diff` as loop frequency. |

### Freematics functions

| Function | Present function |
| --- | --- |
| Python `to_code` | Create the UART sensor and add the external Freematics library. |
| `dump_config` | Log the sensor. |
| `setup` | Set `obd_detected` to false and always mark the component failed. |
| `update` | Do nothing. |

All OBD communication code is commented out. The component does not detect a device or publish CAN or OBD data.

### Exact names for grouped functions

The preceding tables explain these functions by behavior group. This index gives each exact source name in those groups.

| Group | Exact function names |
| --- | --- |
| TMC base registration | `register_tmc2208_base`, `register_tmc2209_base`, `register_tmc2300_base` |
| TMC base validation | `validate_tmc2208_base`, `validate_tmc2209_base`, `validate_tmc2300_base` |
| Hub attachment | `register_tmc2208_hub_device`, `register_tmc2209_hub_device` |
| TMC2208 action code generation | `tmc2208_enable_to_code`, `tmc2208_disable_to_code`, `tmc2208_configure_to_code`, `tmc2208_currents_to_code`, `tmc2208_chopconf_to_code`, `tmc2208_pwmconf_to_code`, `tmc2208_sync_to_code` |
| TMC2209 action code generation | `tmc2209_enable_to_code`, `tmc2209_disable_to_code`, `tmc2209_configure_to_code`, `tmc2209_currents_to_code`, `tmc2209_stallguard_to_code`, `tmc2209_coolconf_to_code`, `tmc2209_chopconf_to_code`, `tmc2209_pwmconf_to_code`, `tmc2209_sync_to_code` |
| TMC2300 action code generation | `tmc2300_enable_to_code`, `tmc2300_disable_to_code`, `tmc2300_configure_to_code`, `tmc2300_currents_to_code`, `tmc2300_stallguard_to_code`, `tmc2300_coolconf_to_code`, `tmc2300_chopconf_to_code`, `tmc2300_pwmconf_to_code` |
| Stepper action classes | `SetTargetAction`, `ReportPositionAction`, `SetSpeedAction`, `SetAccelerationAction`, `SetDecelerationAction`, `StopAction` |
| TMC configuration setters | `set_enn_pin`, `set_diag_pin`, `set_diag0_pin`, `set_diag1_pin`, `set_index_pin`, `set_step_pin`, `set_dir_pin`, `set_control_method` |
| TMC electrical setters | `set_analog_current_scale`, `set_vsense`, `set_ottrim`, `set_toff_recovery` |
| TMC diagnostic setters | `set_enable_driver_health_check`, `set_enable_stall_detection`, `set_config_dump_include_registers` |
| TMC callback and synchronization setters | `add_on_driver_status_callback`, `set_drivers` |
| Run-current functions | `write_run_current_mA`, `read_run_current_mA`, `write_run_current`, `read_run_current` |
| Hold-current functions | `write_hold_current_mA`, `read_hold_current_mA`, `write_hold_current`, `read_hold_current` |
| Closed-loop object setters | `set_stepper`, `set_position_sensor`, `set_error_sensor` |
| Closed-loop numeric setters | `set_steps_per_revolution`, `set_encoder_counts`, `set_correction_threshold`, `set_max_correction`, `set_settle_window` |
| Closed-loop Boolean setters | `set_initial_sync`, `set_auto_disable` |
| ICM20948 sensor setters | `set_accel_x_sensor`, `set_accel_y_sensor`, `set_accel_z_sensor`, `set_gyro_x_sensor`, `set_gyro_y_sensor`, `set_gyro_z_sensor`, `set_temperature_sensor` |
| HUSB238 sensor setters | `set_output_voltage_sensor`, `set_output_current_sensor`, `set_input_voltage_sensor` |
| TMC5240 ADC functions | `read_supply_voltage_mV`, `read_adc_ain_mV`, `read_temp_C` |

`std::make_tuple` appears in the source. It is a standard-library function, not a repository-owned function.

## Editable configuration reference

| Feature | File | Present value or range | Requested change |
| --- | --- | --- | --- |
| General device name | `microdriver_betea_1.yaml` | `microdriver` | `KEEP` |
| Door device name | `microdriver_door.yaml` | `microdriver_door` | `KEEP` |
| Boot run current | Both product YAML files | `0.5 A` | `KEEP` |
| Boot hold current | Both product YAML files | `0.2 A` | `KEEP` |
| Microsteps | Both product YAML files | `8` | `KEEP` |
| Standstill mode | Both product YAML files | `freewheeling` | `KEEP` |
| USB-PD request | Both product YAML files | `12 V` | `KEEP` |
| StallGuard threshold | Both product YAML files | `50` | `KEEP` |
| StallGuard TCOOLTHRS | Both product YAML files | `500000` | `KEEP` |
| General homing speed | `microdriver_betea_1.yaml` | `400 steps/s` | `KEEP` |
| Door speed range | `microdriver_door.yaml` | `50` through `1000 steps/s` | `KEEP` |
| Door run timeout | `microdriver_door.yaml` | `30s` | `KEEP` |
| Door reverse pause | `microdriver_door.yaml` | `250ms` | `KEEP` |
| General position range | `microdriver_betea_1.yaml` | `-100000` through `100000` | `KEEP` |
| General jog range | `microdriver_betea_1.yaml` | `-2000` through `2000 steps/s` | `KEEP` |
| Wi-Fi and API credentials | ESPHome secrets | Protected secret references | `KEEP` |

## Identified implementation findings

| Severity | Location | Finding | Requested decision |
| --- | --- | --- | --- |
| Error | `components/as5047` | Most register and calibration functions are stubs. The angle sensor does not publish a value. | `FIX`, `REMOVE`, or `ACCEPT` |
| Error | `components/icm20948/icm20948.cpp` | Read lengths exceed local buffer sizes. This can overwrite memory. | `FIX` |
| Error | `components/icm20948_spi` | The SPI class does not implement required register read and write functions. | `FIX` or `REMOVE` |
| Error | `components/tmc5240/tmc5240.cpp` | `write_field` ignores its input value and can write an incorrect field. | `FIX` |
| Error | `components/tmc5240_uart` | Register reads do not receive a complete five-byte reply. | `FIX` |
| Warning | `components/tmc5240` | The stepper has no motion implementation and contains test logging and writes. | `COMPLETE` or `REMOVE` |
| Warning | `components/freematics` | Setup always fails and update does nothing. | `COMPLETE` or `REMOVE` |
| Warning | `components/icm20948` | Configured acceleration, gyro, and temperature sensors never receive published values. | `COMPLETE` or `REMOVE` |
| Warning | `components/husb238_i2c` | `input_voltage` repeats negotiated VBUS. It is not a separate measurement. | `KEEP`, `RENAME`, or `REMOVE` |
| Review | TMC2208 and TMC2300 | These older implementations do not contain all TMC2209 recovery and checked-read protections. | `UPGRADE` or `KEEP` |
| Review | Repository documentation | Existing component README files use uncontrolled English and contain encoding damage. | `REWRITE` or `KEEP` |
| Review | Generated `.esphome` tree | Build output is present inside a component directory. | `IGNORE`, `REMOVE`, or `GITIGNORE` |

## Change-request table

Add one requirement per row. Use a measurable result when possible.

| ID | Component or file | Requested behavior | Values and limits | Priority | Acceptance test |
| --- | --- | --- | --- | --- | --- |
| CR-001 | Example: `microdriver_door.yaml` | Example: Add a calibrated open position. | Example: `24000 steps` | High | Example: Home, then move to `24000`. |
| CR-002 | `microdriver_door.yaml` and `DOOR_CONTROL_GUIDE.md` | Implemented: Add door cover controls and explain motor operation. Use StallGuard for both mechanical end-stops. Disable the motor after each stall. | Positive is open. Negative is closed. Timeout is `30s`. Reverse pause is `250ms`. | High | Open and Close stop at their end-stops. Stop and timeout remove motor torque. |
| CR-003 |  |  |  |  |  |
| CR-004 |  |  |  |  |  |
| CR-005 |  |  |  |  |  |

## Proposed project terminology

An authorized project reviewer must approve this list before a `candidate-ste` review.

| Concept | Preferred term | Protected alternatives or identifiers |
| --- | --- | --- |
| Motor control integrated circuit | stepper driver | TMC2208, TMC2209, TMC2300, TMC5240 |
| Commanded motor location | target position | `target_position` |
| Estimated motor location | current position | `current_position` |
| Motor subdivision | microstep | MRES |
| Sensorless load detection | StallGuard | SGTHRS, SG_RESULT |
| Driver enable input | ENN pin | `enn_pin` |
| Diagnostic output | DIAG pin | `diag_pin`, DIAG0, DIAG1 |
| Shared serial control line | UART bus | PDN_UART |
| Power contract | USB Power Delivery contract | USB-PD, PDO |
| Encoder correction period | settle window | `settle_window` |

## Verification record

The review preserved all observed identifiers, values, units, task sequences, and safety-related behavior. The review did not modify runtime code or product YAML.

The document received structural checks against the derived Issue 9 profile. Human review is required for terminology, technical accuracy, risk decisions, and final conformance.
