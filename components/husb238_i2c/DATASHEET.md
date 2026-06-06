# HUSB238-002D USB Power Delivery Controller - Technical Reference

## Device Overview

The **HUSB238-002D** is an I2C-controlled USB Power Delivery (USB PD) Configuration Controller manufactured by Hynetek. It enables dynamic voltage and current configuration for USB PD-compatible power supplies.

**Manufacturer:** Hynetek (海芯微)  
**Part Number:** HUSB238-002DD  
**Package:** DFN-10L (3mm × 3mm)  
**Temperature Range:** -40°C to +85°C  
**Supply Voltage:** 3V to 25V  
**Quiescent Current:** ~3.1mA  

## Features

- **I2C Communication Interface** - Standard I2C protocol at 400kHz
- **Multiple PDO Support** - Up to 5 USB Power Delivery Objects (profiles)
- **Voltage Selection** - Support for 5V, 9V, 15V, 20V and custom profiles
- **Current Limiting** - Configurable maximum current per voltage level
- **Output Control** - Enable/disable USB PD output
- **Real-time Monitoring** - Read actual output voltage and current
- **Status Indicators** - Cable plug detection, negotiation status
- **Auto-negotiation** - Automatic USB PD handshake with power adapter
- **Hot-swap Safe** - Safe insertion/removal of USB cables

## I2C Interface

### Communication Protocol

- **I2C Address:** 0x08 (7-bit) / 0x10 (8-bit read) / 0x11 (8-bit write)
- **Speed:** 100kHz to 400kHz
- **Register Size:** 8-bit (1 byte per register)

### Timing Requirements

```
SDA/SCL Setup Time:  100ns minimum
Hold Time:           100ns minimum
Frequency:           100-400kHz
Clock Stretch:       Supported
```

## Register Map

### Control and Status Registers (0x00-0x0A)

| Address | Name | Type | Description |
|---------|------|------|-------------|
| 0x00 | CHIP_ID | RO | Chip identification (typically 0xF2 or similar) |
| 0x01 | FIRMWARE | RO | Firmware version |
| 0x02 | CONFIG | R/W | Device configuration flags |
| 0x03 | STATUS | RO | Current status and events |
| 0x04 | POWER | RO | Power information (input voltage) |
| 0x05 | VOLTAGE | RO | Output voltage (0.1V per LSB) |
| 0x06 | CURRENT | RO | Output current (10mA per LSB) |
| 0x07 | AVAILABLE_PDOS | RO | Number of available PDOs (bit field) |
| 0x08 | SELECTED_PDO | R/W | Currently selected PDO index (1-5) |
| 0x09 | OUTPUT_ENABLE | R/W | Output enable control (1=enabled, 0=disabled) |
| 0x0A | CABLE_PLUG | RO | Cable plug detection status |

### PDO Configuration Registers (0x20-0x28)

| Address | Name | Type | Description |
|---------|------|------|-------------|
| 0x20-0x21 | PDO1 | R/W | PDO1 voltage and current |
| 0x22-0x23 | PDO2 | R/W | PDO2 voltage and current |
| 0x24-0x25 | PDO3 | R/W | PDO3 voltage and current |
| 0x26-0x27 | PDO4 | R/W | PDO4 voltage and current |
| 0x28-0x29 | PDO5 | R/W | PDO5 voltage and current (optional) |

### Register Format Details

#### CHIP_ID (0x00)
```
Bit 7-0: Chip ID
Value: 0xF2 (typical)
```

#### STATUS (0x03)
```
Bit 0: PDO Change Event (1 = changed)
Bit 1: Cable Plug-in Event (1 = plugged in)
Bit 2: Negotiation Done (1 = complete)
Bit 3: Hard Reset Received (1 = received)
Bit 4-7: Reserved
```

#### VOLTAGE (0x05)
```
Calculation: Register_Value × 0.1V
Example: 0x32 = 50 × 0.1 = 5.0V
Range: 0V to 25.5V
```

#### CURRENT (0x06)
```
Calculation: Register_Value × 10mA
Example: 0x64 = 100 × 10 = 1000mA
Range: 0mA to 2550mA (2.55A)
```

#### AVAILABLE_PDOS (0x07)
```
Bit 0: PDO1 available
Bit 1: PDO2 available
Bit 2: PDO3 available
Bit 3: PDO4 available
Bit 4: PDO5 available
```

#### SELECTED_PDO (0x08)
```
Valid Values: 0x01 to 0x05
0x00: No PDO selected / negotiation in progress
```

#### PDO Configuration
Each PDO is configured with 2 bytes: [Voltage][Current]

Voltage Byte:
```
Calculation: (Voltage_Desired / 0.1)
Example: 9V → 0x5A (90 decimal)
```

Current Byte:
```
Calculation: (Current_Desired_mA / 10)
Example: 2A (2000mA) → 0xC8 (200 decimal)
```

## USB Power Delivery Profiles

### Standard Profiles

| PDO | Voltage | Max Current | Power |
|-----|---------|-------------|-------|
| 1 | 5V | 3A | 15W |
| 2 | 9V | 2A | 18W |
| 3 | 15V | 2A | 30W |
| 4 | 20V | 1A | 20W |
| 5 | Custom | Custom | Varies |

### Example Configuration Values

**Standard USB PD 3.0:**
```
PDO1: 5V, 3A  (0x32, 0xC8)
PDO2: 9V, 2A  (0x5A, 0x96)
PDO3: 15V, 2A (0x96, 0x96)
PDO4: 20V, 1A (0xC8, 0x64)
```

**High-Power Configuration:**
```
PDO1: 5V, 5A  (0x32, 0x32)
PDO2: 15V, 3A (0x96, 0xB4)
PDO3: 20V, 5A (0xC8, 0x32)
```

## Operation Modes

### Mode 1: Auto-negotiation
1. Connect USB PD power adapter
2. Device automatically negotiates with adapter
3. Reads available PDOs from adapter
4. Stores capabilities in AVAILABLE_PDOS register

### Mode 2: Manual PDO Selection
1. Write desired PDO index to SELECTED_PDO register
2. Device requests voltage change from adapter
3. Adapter switches to requested voltage
4. Output voltage updates in VOLTAGE register

### Mode 3: Output Control
1. Enable/disable USB PD output via OUTPUT_ENABLE register
2. When disabled, device stops requesting power
3. When enabled, device resumes power delivery

## I2C Transaction Examples

### Read Output Voltage
```
START → [0x10] → [0x05] → [ACK] → [Data] → STOP
Address:  0x08     Reg    ACK    Value
Result: Voltage = Data × 0.1V
```

### Write Selected PDO
```
START → [0x11] → [0x08] → [0x02] → [ACK] → STOP
Address:  0x08     Reg    Value(PDO2)
```

### Read Multiple Registers
```
START → [0x10] → [0x03] → [ACK] → [Status] → [NACK] → STOP
Address:  0x08     Start    ACK     Data        NACK
```

## Typical Application Schematic

```
             ┌────────────────────────────────┐
    USB PD   │   HUSB238-002D                 │
    Supply   │                                │
      ↓      │  SDA ────────────── SDA        │
     5-20V   │  SCL ────────────── SCL        │
      │      │  GND ────────────── GND        │
      ├─────→│  VCC (3.3V)        │ 3.3V      │
      │      │                    │           │
      └─────→│  VBUS         Output Connector│
             │  GND               │ (USB Type-C)
             │                    │           │
             └────────────────────────────────┘
                        ▲
                        │
                    [Pull-ups]
                    4.7kΩ on
                    SDA/SCL
```

## Pin Configuration

### HUSB238 DFN-10L Pinout

```
         ┌─────────┐
    VCC  │1      10│ GND
    SDA  │2       9│ (NC)
    SCL  │3       8│ (NC)
   (NC)  │4       7│ (NC)
   (NC)  │5       6│ (NC)
         └─────────┘
```

Note: NC = Not Connected (reserved for future use)

## Electrical Characteristics

### Absolute Maximum Ratings
| Parameter | Min | Typ | Max | Unit |
|-----------|-----|-----|-----|------|
| Supply Voltage (VCC) | -0.3 | — | 6.0 | V |
| I/O Voltage | -0.3 | — | VCC+0.3 | V |
| Junction Temperature | — | — | 150 | °C |
| Storage Temperature | -55 | — | 150 | °C |

### Operating Characteristics
| Parameter | Min | Typ | Max | Unit | Condition |
|-----------|-----|-----|-----|------|-----------|
| VCC Operating | 3.0 | 3.3 | 5.0 | V | Normal operation |
| IDD Quiescent | — | 3.1 | — | mA | No USB activity |
| I²C Speed | 100 | — | 400 | kHz | SCL frequency |
| SDA/SCL Pullup | 4.7 | — | 10 | kΩ | Recommended |

## Software Development Notes

### Power Delivery Negotiation
1. After cable insertion, HUSB238 automatically negotiates with the adapter
2. Negotiation completes when STATUS register bit 2 = 1
3. Available PDOs appear in AVAILABLE_PDOS register
4. Default selection is usually PDO1 (5V)

### Voltage Selection Strategy
```
1. Read AVAILABLE_PDOS to see which voltages are supported
2. Select desired voltage by writing to SELECTED_PDO
3. Wait 100-500ms for voltage to stabilize
4. Read VOLTAGE register to confirm
5. Enable output via OUTPUT_ENABLE if needed
```

### Error Handling
- If SELECTED_PDO write fails: adapter may not support requested voltage
- If voltage doesn't change: check cable quality and adapter PD support
- If negotiation fails: verify adapter is certified USB PD

## Thermal Considerations

- Device is suitable for -40°C to +85°C operation
- Maximum thermal dissipation: ~100mW at typical operation
- No heatsink required for standard usage
- Maintain adequate PCB copper around VCC/GND for thermal stability

## Layout Recommendations

1. **Decoupling Capacitor**
   - 100nF ceramic capacitor close to VCC pin
   - Additional 10µF bulk capacitor for noise filtering

2. **I2C Pull-ups**
   - 4.7kΩ resistors on SDA and SCL (mandatory)
   - Connect to VCC (3.3V)

3. **Trace Routing**
   - Keep I2C traces short (<10cm)
   - Route I2C lines away from high-speed signals
   - Use ground plane for return paths

4. **Ground**
   - Multiple ground vias near HUSB238
   - Connect all ground pins together
   - Good ground return path to ESP32

## Power Budget

Typical power consumption:
- Idle (negotiating): ~5mW
- Active (USB PD output): ~100-500mW (depending on load)
- Output is powered from USB adapter, not ESP32

## Compliance

- USB Power Delivery Specification Revision 3.0 compliant
- I²C Interface: Meets standard I²C protocol
- EMI: Class B (consumer electronics)

## Related Documentation

- HUSB238 Official Datasheet (Hynetek)
- USB Power Delivery Specification 3.0 (USB-IF)
- I²C Specification v4.1 (NXP)

## Revision History

| Version | Date | Notes |
|---------|------|-------|
| 1.0 | 2026-06 | Initial documentation for MicroDriver integration |

## Support

For questions or issues regarding the HUSB238 integration:
1. Check the [component README](README.md)
2. Review the [example configuration](example.yaml)
3. Verify I2C communication with an I2C scanner tool
4. Consult the official Hynetek HUSB238 datasheet
