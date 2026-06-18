# Stepper Closed Loop

Optional closed-loop position feedback for any ESPHome stepper motor. On each update cycle the component reads an encoder sensor, tracks multi-turn position, and re-syncs the stepper's internal position counter when it deviates from the encoder by more than a configurable threshold.

Works with any stepper platform in this library (TMC2209, TMC2208, TMC2300, TMC5240) and any ESPHome sensor as the position input. The recommended encoder is the **AS5600** magnetic absolute encoder, which is a built-in ESPHome component.

---

## How it works

1.  On every `update_interval` the component reads the encoder position sensor.
2.  It tracks wrap-arounds (multi-turn) by detecting jumps larger than half a revolution.
3.  It converts the encoder reading to steps using `steps_per_revolution / encoder_counts_per_revolution`.
4.  If `|encoder_steps − motor.current_position| > correction_threshold`, it sets `motor.current_position = encoder_steps`.
5.  The stepper's existing acceleration profile then drives the motor from the corrected position back to its original `target_position`.

This handles:

*   **Missed steps** — any skipped steps are corrected on the next update
*   **Stall recovery** — once obstruction is cleared the motor resumes from its actual position
*   **Startup alignment** — optionally syncs `current_position` to encoder on boot (`initial_sync: true`)

> **Idle behavior**: wrap-around tracking always runs (so the encoder position stays accurate even while parked). Correction continues for `settle_window` after the motor reaches its target (useful to catch the last few missed steps during deceleration), then stops to avoid the motor chasing encoder noise/drift while parked. Set `settle_window: 0s` to stop correcting immediately on arrival.

---

## Wiring

The AS5600 is an I²C device (address `0x36`). Mount the magnet concentrically on the motor shaft.

```
AS5600 VCC  → 3.3 V
AS5600 GND  → GND
AS5600 SDA  → ESP32-S3 SDA pin
AS5600 SCL  → ESP32-S3 SCL pin
AS5600 DIR  → GND (clockwise) or 3.3 V (counter-clockwise)
```

---

## Setup

Import the component:

```
external_components:
  - source: github://Trondle-Embedded-Systems/MicroDriver-software
    components: [tmc2209_hub, tmc2209, stepper, stepper_closed_loop]
```

---

## Configuration

### Minimal example — TMC2209 with AS5600

```
i2c:
  sda: GPIO8
  scl: GPIO9

as5600:
  id: encoder_hub
  direction: clockwise

sensor:
  - platform: as5600
    position:
      id: encoder_position
      name: "Motor encoder"
      update_interval: 20ms  # keep fast for accurate wrap detection

uart:
  tx_pin: GPIO17
  rx_pin: GPIO18
  baud_rate: 500000

tmc2209_hub:
  id: driver_hub
  uart_id: ...

stepper:
  - platform: tmc2209
    id: motor
    tmc2209_hub_id: driver_hub
    address: 0x00
    max_speed: 3200 steps/s
    acceleration: 10000 steps/s^2
    serial_control:
      index_pin: GPIO5

stepper_closed_loop:
  stepper_id: motor
  position_sensor: encoder_position
  steps_per_revolution: 3200        # 200 full steps × 16 microsteps
  correction_threshold: 10          # steps error before correcting
  initial_sync: true                # align position to encoder on boot
  update_interval: 50ms
```

### With auto-disable (motor de-energises when idle)

```
stepper_closed_loop:
  stepper_id: motor
  position_sensor: encoder_position
  steps_per_revolution: 3200
  settle_window: 500ms   # keep correcting for 500 ms after arrival
  auto_disable: true     # then cut power; re-enable syncs from AS5600
```

### With error monitoring sensor

```
stepper_closed_loop:
  stepper_id: motor
  position_sensor: encoder_position
  steps_per_revolution: 3200
  error_sensor:
    name: "Position error"
    id: motor_position_error
```

---

## Configuration reference

| Key | Required | Default | Description |
| --- | --- | --- | --- |
| `stepper_id` | **yes** | — | ID of any stepper component |
| `position_sensor` | **yes** | — | ID of the encoder position sensor (AS5600 `position` sub-sensor recommended) |
| `steps_per_revolution` | **yes** | — | Motor steps per encoder revolution (full\_steps × microstepping) |
| `encoder_counts_per_revolution` | no | `4096` | Encoder counts per revolution (AS5600 = 4096) |
| `correction_threshold` | no | `10` | Minimum step error to trigger re-sync (deadband) |
| `max_correction` | no | `500` | Maximum plausible error in steps; larger jumps are discarded as noise |
| `initial_sync` | no | `true` | Sync `current_position` and `target_position` to encoder on first reading |
| `settle_window` | no | `500ms` | How long after reaching the target to keep correcting. Accepts ESPHome time values (`500ms`, `1s`, `0s` to disable). Set to `0s` to stop correcting the moment the target is reached. |
| `auto_disable` | no | `false` | Disable the motor driver after `settle_window` expires (coils de-energised). On the next `stepper.set_target` the encoder re-syncs position before the motor moves. |
| `update_interval` | no | `50ms` | How often to check the encoder |
| `error_sensor` | no | — | Optional sensor that publishes the current step error (positive = motor behind) |

### `steps_per_revolution` examples

| Motor | Microstepping | Value |
| --- | --- | --- |
| NEMA17 (200 full steps) | 1/16 | `3200` |
| NEMA17 (200 full steps) | 1/8 | `1600` |
| NEMA17 (200 full steps) | 1/4 | `800` |

> **Note**: if there is a gearbox between the motor and the encoder, multiply `steps_per_revolution` by the gear ratio.

---

## Multi-turn tracking

The AS5600 reports a single-turn position (0–4095 counts per revolution). The component tracks wrap-arounds automatically: when two successive readings differ by more than half a revolution, a turn boundary is assumed. The encoder's `update_interval` must be short enough that the motor cannot travel more than half a revolution between two readings — at 3200 steps/rev and 3200 steps/s maximum speed:

```
max_interval = (steps_per_rev / 2) / max_speed_steps_s
             = (3200 / 2) / 3200 = 0.5 s
```

Keep `update_interval` well below this value (50 ms is safe for most applications).

---

## Troubleshooting

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| Motor oscillates around target | `correction_threshold` too small | Increase `correction_threshold` |
| Corrections not applied | Encoder update rate slower than `update_interval` | Decrease encoder `update_interval` |
| `noise?` warnings in log | Encoder magnet mis-aligned or weak | Check `magnitude` sensor — optimal ~2100 |
| Turn count wrong after fast moves | `update_interval` too large | Decrease `update_interval` |