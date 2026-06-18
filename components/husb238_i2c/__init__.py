import logging
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT, UNIT_VOLT

CODEOWNERS = ["@nicolastrondle"]
DEPENDENCIES = ["i2c"]
# This component creates sensors, so it must pull in the core `sensor` component
# itself (otherwise it only builds when the config also has a top-level `sensor:`).
AUTO_LOAD = ["sensor"]

_LOGGER = logging.getLogger(__name__)

husb238_ns = cg.esphome_ns.namespace("husb238_i2c")
HUSB238Component = husb238_ns.class_("HUSB238", cg.PollingComponent, i2c.I2CDevice)

OutputVoltageSensor = husb238_ns.class_(
    "OutputVoltageSensor", cg.PollingComponent, sensor.Sensor, cg.Parented.template(HUSB238Component)
)
OutputCurrentSensor = husb238_ns.class_(
    "OutputCurrentSensor", cg.PollingComponent, sensor.Sensor, cg.Parented.template(HUSB238Component)
)
InputVoltageSensor = husb238_ns.class_(
    "InputVoltageSensor", cg.PollingComponent, sensor.Sensor, cg.Parented.template(HUSB238Component)
)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_ID): cv.declare_id(HUSB238Component),
    cv.Optional("output_voltage"): sensor.sensor_schema(
        OutputVoltageSensor,
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("output_current"): sensor.sensor_schema(
        OutputCurrentSensor,
        unit_of_measurement="mA",
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional("input_voltage"): sensor.sensor_schema(
        InputVoltageSensor,
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    # Voltage to request from the attached USB-PD source. The source must
    # actually advertise this PDO or the request is ignored by the adapter.
    # Omit (or set 0) to leave the chip in auto mode and not request anything.
    cv.Optional("request_voltage"): cv.one_of(0, 5, 9, 12, 15, 18, 20, int=True),
}).extend(cv.polling_component_schema("1s")).extend(i2c.i2c_device_schema(0x08))

# Maps a requested voltage (V) to the HUSB238 SRC_PDO selection code (top 4
# bits of register 0x08). Note the non-contiguous jump at 15V.
_VOLTAGE_TO_SELECTION = {
    0: 0x0,
    5: 0x1,
    9: 0x2,
    12: 0x3,
    15: 0x8,
    18: 0x9,
    20: 0xA,
}


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if "output_voltage" in config:
        sens = await sensor.new_sensor(config["output_voltage"])
        await cg.register_parented(sens, var)
        cg.add(var.set_output_voltage_sensor(sens))

    if "output_current" in config:
        sens = await sensor.new_sensor(config["output_current"])
        await cg.register_parented(sens, var)
        cg.add(var.set_output_current_sensor(sens))

    if "input_voltage" in config:
        sens = await sensor.new_sensor(config["input_voltage"])
        await cg.register_parented(sens, var)
        cg.add(var.set_input_voltage_sensor(sens))

    if "request_voltage" in config:
        selection = _VOLTAGE_TO_SELECTION[config["request_voltage"]]
        cg.add(var.set_request_voltage(selection))
