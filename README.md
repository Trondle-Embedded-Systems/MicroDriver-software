# MicroDriver ESPHome Components

Custom ESPHome external components for the MicroDriver project: stepper motor drivers, encoders, and IMUs.

## Components

| Component | Description | Interface |
|-----------|-------------|-----------|
| `stepper` | Generic stepper base with acceleration/deceleration profiles | — |
| `tmc2209` | ADI Trinamic TMC2209 smart stepper driver | UART |
| `tmc2209_hub` | UART hub for multiple TMC2209 drivers on one bus | UART |
| `tmc2208` | ADI Trinamic TMC2208 smart stepper driver | UART |
| `tmc2208_hub` | UART hub for multiple TMC2208 drivers on one bus | UART |
| `tmc2300` | ADI Trinamic TMC2300 compact stepper driver | UART |
| `tmc5240` | ADI Trinamic TMC5240 high-power stepper driver (base) | SPI/UART |
| `tmc5240_spi` | TMC5240 SPI variant | SPI |
| `tmc5240_uart` | TMC5240 UART variant | UART |
| `as5047` | AMS AS5047 absolute magnetic position encoder | SPI |
| `icm20948` | InvenSense ICM-20948 9-DOF IMU (base) | — |
| `icm20948_i2c` | ICM-20948 I2C variant | I2C |
| `icm20948_spi` | ICM-20948 SPI variant | SPI |
| `stepper_closed_loop` | Optional closed-loop feedback for any stepper (AS5600 or any sensor) | I2C (via sensor) |
| `insight` | ESPHome internal diagnostics (loop frequency, etc.) | — |
| `freematics` | Freematics CAN bus sniffer interface | UART |

---

## Usage

Add the following to your ESPHome YAML configuration:

```yaml
external_components:
  - source: github://Trondle-Embedded-Systems/MicroDriver-software
    components: [tmc2209_hub, tmc2209, stepper]
```

To pin to a specific commit for stability, append the commit SHA:

```yaml
external_components:
  - source: github://Trondle-Embedded-Systems/MicroDriver-software@<commit-sha>
    components: [tmc2209_hub, tmc2209, stepper]
```

For local development, reference the `components/` directory directly:

```yaml
external_components:
  - source:
      type: local
      path: /path/to/MicroDriver-software/components
    components: [tmc2209_hub, tmc2209, stepper]
```

---

## ESP32-S3 Recommended Configuration

These components are optimised for the ESP32-S3. Use the `esp-idf` framework for best performance — it gives full FreeRTOS control, proper IRAM placement, and access to both cores.

```yaml
esp32:
  board: esp32-s3-devkitc-1
  variant: esp32s3
  framework:
    type: esp-idf

external_components:
  - source: github://Trondle-Embedded-Systems/MicroDriver-software
    components: [tmc2209_hub, tmc2209, stepper]
```

### Performance notes for ESP32-S3

- **IRAM placement**: Speed calculation and step-generation functions are marked `IRAM_ATTR HOT`, keeping them in internal RAM to avoid flash-cache latency during high-frequency stepping.
- **Dual-core step generation**: On dual-core ESP32-S3 builds (`CONFIG_FREERTOS_UNICORE=0`), the PULSES_CONTROL mode spawns a dedicated FreeRTOS task pinned to core 0 (PRO_CPU) at maximum priority. This isolates pulse timing from the ESPHome main loop running on core 1, improving step accuracy at high speeds.
- **SERIAL_CONTROL mode**: The TMC2209/2208 SERIAL_CONTROL mode uses the driver's internal step interpolation via `VACTUAL`, bypassing software pulse generation entirely — recommended for the highest step rates.

---

## Component Documentation

Each component has its own `README.md` with wiring diagrams, full configuration reference, and example YAML:

- [TMC2209](components/tmc2209/README.md)
- [TMC2208](components/tmc2208/README.md)
- [TMC2300](components/tmc2300/README.md)
- [AS5047](components/as5047/README.md)
- [ICM-20948](components/icm20948/README.md)
- [Stepper Closed Loop (AS5600)](components/stepper_closed_loop/README.md)
- [Insight](components/insight/README.md)


---

## Acknowledgements

This repo has been modified from https://github.com/slimcdk/esphome-custom-components/tree/master

---

## License

[GNU General Public License v3.0](LICENSE)
