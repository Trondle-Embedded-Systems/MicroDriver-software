import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_SENSOR

CODEOWNERS = ["@slimcdk"]

AUTO_LOAD = ["sensor"]
MULTI_CONF = True

CONF_STEPPER_ID = "stepper_id"
CONF_POSITION_SENSOR = "position_sensor"
CONF_STEPS_PER_REVOLUTION = "steps_per_revolution"
CONF_ENCODER_COUNTS = "encoder_counts_per_revolution"
CONF_CORRECTION_THRESHOLD = "correction_threshold"
CONF_MAX_CORRECTION = "max_correction"
CONF_INITIAL_SYNC = "initial_sync"
CONF_SETTLE_WINDOW = "settle_window"
CONF_AUTO_DISABLE = "auto_disable"
CONF_ERROR_SENSOR = "error_sensor"

stepper_closed_loop_ns = cg.esphome_ns.namespace("stepper_closed_loop")
StepperClosedLoop = stepper_closed_loop_ns.class_("StepperClosedLoop", cg.PollingComponent)

# Reference Stepper base class by its C++ namespace — works with any stepper platform
stepper_ns = cg.esphome_ns.namespace("stepper")
Stepper = stepper_ns.class_("Stepper")

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(StepperClosedLoop),
        cv.Required(CONF_STEPPER_ID): cv.use_id(Stepper),
        cv.Required(CONF_POSITION_SENSOR): cv.use_id(sensor.Sensor),
        cv.Required(CONF_STEPS_PER_REVOLUTION): cv.positive_int,
        cv.Optional(CONF_ENCODER_COUNTS, default=4096): cv.positive_int,
        cv.Optional(CONF_CORRECTION_THRESHOLD, default=10): cv.positive_int,
        cv.Optional(CONF_MAX_CORRECTION, default=500): cv.positive_int,
        cv.Optional(CONF_INITIAL_SYNC, default=True): cv.boolean,
        cv.Optional(CONF_SETTLE_WINDOW, default="500ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_AUTO_DISABLE, default=False): cv.boolean,
        cv.Optional(CONF_ERROR_SENSOR): sensor.sensor_schema(
            unit_of_measurement="steps",
            accuracy_decimals=0,
            icon="mdi:swap-vertical",
        ),
    }
).extend(cv.polling_component_schema("50ms"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    stepper_var = await cg.get_variable(config[CONF_STEPPER_ID])
    cg.add(var.set_stepper(stepper_var))

    pos_sensor = await cg.get_variable(config[CONF_POSITION_SENSOR])
    cg.add(var.set_position_sensor(pos_sensor))

    cg.add(var.set_steps_per_revolution(config[CONF_STEPS_PER_REVOLUTION]))
    cg.add(var.set_encoder_counts(config[CONF_ENCODER_COUNTS]))
    cg.add(var.set_correction_threshold(config[CONF_CORRECTION_THRESHOLD]))
    cg.add(var.set_max_correction(config[CONF_MAX_CORRECTION]))
    cg.add(var.set_initial_sync(config[CONF_INITIAL_SYNC]))
    cg.add(var.set_settle_window(config[CONF_SETTLE_WINDOW].total_milliseconds))
    cg.add(var.set_auto_disable(config[CONF_AUTO_DISABLE]))

    if CONF_ERROR_SENSOR in config:
        err_sens = await sensor.new_sensor(config[CONF_ERROR_SENSOR])
        cg.add(var.set_error_sensor(err_sens))
