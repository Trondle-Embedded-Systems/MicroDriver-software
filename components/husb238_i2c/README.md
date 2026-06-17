# HUSB238 USB Power Delivery Configuration Controller

A complete ESPHome component for the Hynetek HUSB238-002D USB Power Delivery (USB PD) configuration controller with full I2C support.

## Overview

The HUSB238-002D is an I2C-based USB Power Delivery configuration controller that enables:

- **Dynamic voltage selection** - Select from multiple Power Delivery Object (PDO) profiles
- **Current limiting** - Configure maximum current for each voltage level
- **Real-time monitoring** - Read current output voltage, current, and input voltage
- **Output control** - Enable/disable the USB PD output
- **Status tracking** - Monitor cable plug detection and negotiation status
- **Full YAML configuration** - Simple, declarative setup in your ESPHome config

## Hardware Connection

The HUSB238-002D communicates via I2C with a typical default address of `0x08`. Connect as follows:

```
HUSB238 Pin    ESP32 Pin
─────────────────────────
SDA            GPIO21 (or your configured SDA pin)
SCL            GPIO22 (or your configured SCL pin)
GND            GND
VCC            3.3V
```

Ensure pull-up resistors (4.7kΩ) are present on both SDA and SCL lines.

## Component Features

### Sensors
- **Output Voltage** - Current output voltage in volts
- **Output Current** - Current output current in milliamps
- **Input Voltage** - Input voltage level

### Switches
- **Enable Output** - Toggle the USB PD output on/off

### Number Inputs
- **Selected PDO** - Read/set which PDO profile is active
- **Select Voltage** - Set output voltage by value (5V, 9V, 15V, 20V, etc.)
- **Max Current** - Set maximum output current

### Configuration
- **PDO Voltage/Current** - Configure up to 5 USB PD profiles with custom voltage and current values

## YAML Configuration

### Basic Configuration

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 100kHz

external_components:
  - source: github://Trondle-Embedded-Systems/MicroDriver-software
    components: [husb238_i2c]

husb238_i2c:
  - id: usb_pd
    address: 0x08
    update_interval: 1s
    
    # Sensors for monitoring
    output_voltage:
      name: "USB PD Output Voltage"
      unit_of_measurement: V
      accuracy_decimals: 2
    
    output_current:
      name: "USB PD Output Current"
      unit_of_measurement: mA
      accuracy_decimals: 0
    
    input_voltage:
      name: "USB PD Input Voltage"
      unit_of_measurement: V
      accuracy_decimals: 2
    
    # Switch for output control
    enable_output:
      name: "USB PD Output Enable"
    
    # Number input for voltage selection
    select_voltage:
      name: "USB PD Select Voltage"
      min_value: 5
      max_value: 20
      step: 1
      unit_of_measurement: V
```

### Advanced Configuration with All Features

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 100kHz

external_components:
  - source: github://Trondle-Embedded-Systems/MicroDriver-software
    components: [husb238_i2c]

husb238_i2c:
  - id: usb_pd_main
    address: 0x08
    update_interval: 1s
    
    # Configure PDO profiles (customize voltage and current for each PDO)
    pdo1_voltage: 5      # 5V
    pdo1_current: 3      # 3A max
    
    pdo2_voltage: 9      # 9V
    pdo2_current: 2      # 2A max
    
    pdo3_voltage: 15     # 15V
    pdo3_current: 2      # 2A max
    
    pdo4_voltage: 20     # 20V
    pdo4_current: 1      # 1A max
    
    pdo5_voltage: 0      # Unused (set to 0)
    pdo5_current: 0
    
    # Output voltage sensor
    output_voltage:
      id: usb_output_voltage
      name: "USB PD Output Voltage"
      unit_of_measurement: V
      accuracy_decimals: 2
      filters:
        - throttle: 1s
    
    # Output current sensor
    output_current:
      id: usb_output_current
      name: "USB PD Output Current"
      unit_of_measurement: mA
      accuracy_decimals: 0
      filters:
        - throttle: 1s
    
    # Input voltage sensor
    input_voltage:
      id: usb_input_voltage
      name: "USB PD Input Voltage"
      unit_of_measurement: V
      accuracy_decimals: 2
      filters:
        - throttle: 1s
    
    # Output enable/disable switch
    enable_output:
      id: usb_output_enable
      name: "USB PD Output Enable"
      icon: mdi:power
    
    # Select specific voltage level
    select_voltage:
      id: usb_select_voltage
      name: "USB PD Select Voltage"
      min_value: 5
      max_value: 20
      step: 1
      unit_of_measurement: V
      icon: mdi:flash
    
    # Set maximum current limit
    max_current:
      id: usb_max_current
      name: "USB PD Max Current"
      min_value: 0
      max_value: 10
      step: 0.5
      unit_of_measurement: A
      icon: mdi:lightning-bolt
    
    # Monitor selected PDO
    selected_pdo:
      id: usb_selected_pdo
      name: "USB PD Selected PDO"
      min_value: 1
      max_value: 5
      step: 1
      icon: mdi:power-plug

# Automation Example: Monitor and log voltage changes
automation:
  - trigger:
      platform: state
      entity_id: sensor.usb_pd_output_voltage
    action:
      - logger.log:
          format: "USB PD voltage changed to %.2f V"
          args: ["id(usb_output_voltage).state"]

# Home Assistant Example: Template Sensors
template:
  - sensor:
      - name: "USB PD Power"
        unit_of_measurement: "W"
        device_class: power
        state: "{{ (states('sensor.usb_pd_output_voltage') | float(0)) * (states('sensor.usb_pd_output_current') | float(0)) / 1000 }}"
```

### Minimal Configuration

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22

external_components:
  - source: github://Trondle-Embedded-Systems/MicroDriver-software
    components: [husb238_i2c]

husb238_i2c:
  - id: usb_pd
    output_voltage:
      name: "USB Output Voltage"
    output_current:
      name: "USB Output Current"
    enable_output:
      name: "USB Output Enable"
```

## Configuration Variables

### husb238_i2c Component

- **address** (*Optional*, int): I2C address of the device. Default is `0x08`.
- **update_interval** (*Optional*, time): Update interval for reading sensor values. Default is `1s`.
- **pdo{1-5}_voltage** (*Optional*, int): Voltage for PDO {1-5} in volts (5-48V). Default PDOs: 5V, 9V, 15V, 20V, unused.
- **pdo{1-5}_current** (*Optional*, int): Maximum current for PDO {1-5} in amperes (0-10A).

### Sensor Options

All sensors support standard ESPHome sensor configuration:

- **name** (*Required*, string): Friendly name for the sensor.
- **id** (*Optional*, string): Identifier for the sensor.
- **unit_of_measurement** (*Optional*, string): Unit of measurement.
- **accuracy_decimals** (*Optional*, int): Decimal places for rounding.
- **filters** (*Optional*, list): ESPHome filters (throttle, delta, etc.).

### Switch Options

- **name** (*Required*, string): Friendly name for the switch.
- **id** (*Optional*, string): Identifier for the switch.
- **icon** (*Optional*, string): MDI icon name.

### Number Options

- **name** (*Required*, string): Friendly name for the number input.
- **id** (*Optional*, string): Identifier for the number input.
- **min_value** (*Optional*, float): Minimum value.
- **max_value** (*Optional*, float): Maximum value.
- **step** (*Optional*, float): Step increment.
- **unit_of_measurement** (*Optional*, string): Unit of measurement.
- **icon** (*Optional*, string): MDI icon name.

## Usage Examples

### Select a Specific Voltage

```yaml
number:
  - platform: template
    name: "Set USB PD Voltage"
    min_value: 5
    max_value: 20
    step: 1
    optimistic: true
    on_value:
      then:
        - number.set_value:
            id: usb_select_voltage
            value: !lambda 'return x;'
```

### Conditional Output Enable Based on Voltage

```yaml
automation:
  - trigger:
      platform: state
      entity_id: number.usb_pd_select_voltage
    condition:
      - condition: numeric_state
        entity_id: sensor.usb_pd_output_voltage
        below: 10
    action:
      - switch.turn_on:
          id: usb_output_enable
```

### Monitor Power Consumption

```yaml
template:
  - sensor:
      - name: "USB PD Power Consumption"
        unit_of_measurement: "W"
        device_class: power
        state: >
          {% set voltage = states('sensor.usb_pd_output_voltage') | float(0) %}
          {% set current = states('sensor.usb_pd_output_current') | float(0) %}
          {{ (voltage * current / 1000) | round(2) }}
```

## Register Map Reference

| Register | Address | Description |
|----------|---------|-------------|
| CHIP_ID | 0x00 | Chip identification |
| FIRMWARE_VERSION | 0x01 | Firmware version |
| CONFIGURATION | 0x02 | Device configuration |
| STATUS | 0x03 | Current status flags |
| POWER | 0x04 | Power information |
| VOLTAGE | 0x05 | Current output voltage |
| CURRENT | 0x06 | Current output current |
| AVAILABLE_PDOS | 0x07 | Number of available PDOs |
| SELECTED_PDO | 0x08 | Currently selected PDO |
| OUTPUT_ENABLE | 0x09 | Output enable control |
| CABLE_PLUG | 0x0A | Cable plug detection status |
| PDO_BASE | 0x20+ | PDO configuration (interval: 0x02) |

## Troubleshooting

### Device Not Detected

1. Verify I2C address with an I2C scanner
2. Check pull-up resistors on SDA/SCL lines
3. Confirm power supply (3.3V) to the device
4. Check for loose connections

### Voltage Not Updating

1. Verify the USB PD power adapter supports the requested voltage
2. Check cable quality and connection
3. Ensure device has proper ground connection
4. Monitor logs for I2C communication errors

### High Update Interval

Increase `update_interval` to reduce I2C traffic if experiencing performance issues:

```yaml
husb238_i2c:
  - id: usb_pd
    update_interval: 5s  # Update every 5 seconds instead of 1s
```

## Notes

- The HUSB238 automatically negotiates with the USB PD power adapter
- PDO selection happens after successful negotiation
- Output control via the switch may not work if cable is unplugged
- Maximum update frequency depends on I2C speed and system load
- Some power adapters may not support all voltage levels

## License

[GNU General Public License v3.0](../../LICENSE)
