import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
)

CODEOWNERS = ["@nicolastrondle"]
DEPENDENCIES = ["i2c"]

_LOGGER = logging.getLogger(__name__)

# Configuration constants
CONF_OUTPUT_VOLTAGE = "output_voltage"
CONF_OUTPUT_CURRENT = "output_current"
CONF_INPUT_VOLTAGE = "input_voltage"
CONF_PDO1_VOLTAGE = "pdo1_voltage"
CONF_PDO1_CURRENT = "pdo1_current"
CONF_PDO2_VOLTAGE = "pdo2_voltage"
CONF_PDO2_CURRENT = "pdo2_current"
CONF_PDO3_VOLTAGE = "pdo3_voltage"
CONF_PDO3_CURRENT = "pdo3_current"
CONF_PDO4_VOLTAGE = "pdo4_voltage"
CONF_PDO4_CURRENT = "pdo4_current"
CONF_PDO5_VOLTAGE = "pdo5_voltage"
CONF_PDO5_CURRENT = "pdo5_current"

husb238_ns = cg.esphome_ns.namespace("husb238_i2c")
HUSB238Component = husb238_ns.class_("HUSB238", cg.PollingComponent, i2c.I2CDevice)

# Sensor class definitions
OutputVoltageSensor = husb238_ns.class_(
    "OutputVoltageSensor", cg.PollingComponent, sensor.Sensor, cg.Parented.template(HUSB238Component)
)
OutputCurrentSensor = husb238_ns.class_(
    "OutputCurrentSensor", cg.PollingComponent, sensor.Sensor, cg.Parented.template(HUSB238Component)
)
InputVoltageSensor = husb238_ns.class_(
    "InputVoltageSensor", cg.PollingComponent, sensor.Sensor, cg.Parented.template(HUSB238Component)
)

# Configuration schema
CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(CONF_ID): cv.declare_id(HUSB238Component),
        cv.Optional(CONF_OUTPUT_VOLTAGE): sensor.sensor_schema(
            OutputVoltageSensor,
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=2,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_OUTPUT_CURRENT): sensor.sensor_schema(
            OutputCurrentSensor,
            unit_of_measurement="mA",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_INPUT_VOLTAGE): sensor.sensor_schema(
            InputVoltageSensor,
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=2,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_PDO1_VOLTAGE, default=5): cv.int_range(5, 48),
        cv.Optional(CONF_PDO2_VOLTAGE, default=9): cv.int_range(5, 48),
        cv.Optional(CONF_PDO3_VOLTAGE, default=15): cv.int_range(5, 48),
        cv.Optional(CONF_PDO4_VOLTAGE, default=20): cv.int_range(5, 48),
        cv.Optional(CONF_PDO5_VOLTAGE, default=0): cv.int_range(0, 48),
        cv.Optional(CONF_PDO1_CURRENT, default=3): cv.int_range(0, 10),
        cv.Optional(CONF_PDO2_CURRENT, default=2): cv.int_range(0, 10),
        cv.Optional(CONF_PDO3_CURRENT, default=2): cv.int_range(0, 10),
        cv.Optional(CONF_PDO4_CURRENT, default=1): cv.int_range(0, 10),
        cv.Optional(CONF_PDO5_CURRENT, default=0): cv.int_range(0, 10),
    })
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x08))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_OUTPUT_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_OUTPUT_VOLTAGE], var)
        cg.add(var.set_output_voltage_sensor(sens))

    if CONF_OUTPUT_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_OUTPUT_CURRENT], var)
        cg.add(var.set_output_current_sensor(sens))

    if CONF_INPUT_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_INPUT_VOLTAGE], var)
        cg.add(var.set_input_voltage_sensor(sens))

    for pdo_idx in range(1, 6):
        voltage_key = f"pdo{pdo_idx}_voltage"
        current_key = f"pdo{pdo_idx}_current"
        if voltage_key in config:
            cg.add(var.set_pdo_voltage(pdo_idx - 1, config[voltage_key]))
        if current_key in config:
            cg.add(var.set_pdo_current(pdo_idx - 1, config[current_key]))
