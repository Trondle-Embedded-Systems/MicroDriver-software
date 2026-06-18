from esphome.const import CONF_ID, CONF_SPEED, CONF_THRESHOLD
from esphome.core import EsphomeError
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import stepper
from esphome.components.tmc2209_hub import CONF_TMC2209_HUB_ID
from .. import (
    CONF_TMC2209_ID,
    CONF_ENN_PIN,
    CONF_INDEX_PIN,
    CONF_DIR_PIN,
    CONF_STEP_PIN,
    tmc2209_ns,
    TMC2209Component,
    TMC2209_BASE_CONFIG_SCHEMA,
    register_tmc2209_base,
    validate_tmc2209_base,
)

CONF_AUTO_DISABLE = "auto_disable"
CONF_SETTLE = "settle"
CONF_STALLGUARD_HOMING = "stallguard_homing"
CONF_POSITIONS = "positions"
CONF_TCOOLTHRS = "tcoolthrs"

CODEOWNERS = ["@slimcdk"]

AUTO_LOAD = ["tmc2209_hub", "tmc2209"]

TMC2209Stepper = tmc2209_ns.class_("TMC2209Stepper", TMC2209Component, stepper.Stepper)
ControlMethod = tmc2209_ns.enum("ControlMethod")


DEVICE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TMC2209_ID): cv.use_id(TMC2209Stepper),
    }
)


def validate_control_method_(config):
    has_index_pin = CONF_INDEX_PIN in config
    has_stepdir_pins = CONF_STEP_PIN in config and CONF_DIR_PIN in config

    if not has_index_pin and not has_stepdir_pins:
        raise cv.Invalid(
            f"Either {CONF_INDEX_PIN} and/or {CONF_STEP_PIN} and {CONF_DIR_PIN} must be configured"
        )
    return config


def validate_standalone_(config):
    # When no UART hub is attached the driver can only be operated standalone via
    # STEP/DIR pulses, and ENN is the only way to enable/disable it (no UART TOFF).
    if CONF_TMC2209_HUB_ID in config:
        return config

    if CONF_INDEX_PIN in config and not (
        CONF_STEP_PIN in config and CONF_DIR_PIN in config
    ):
        raise cv.Invalid(
            f"Without a {CONF_TMC2209_HUB_ID} (UART), {CONF_INDEX_PIN} serial control is "
            f"unavailable; configure {CONF_STEP_PIN} and {CONF_DIR_PIN} instead."
        )
    if not (CONF_STEP_PIN in config and CONF_DIR_PIN in config):
        raise cv.Invalid(
            f"Without a {CONF_TMC2209_HUB_ID} (UART), both {CONF_STEP_PIN} and "
            f"{CONF_DIR_PIN} are required to drive the motor."
        )
    if CONF_ENN_PIN not in config:
        raise cv.Invalid(
            f"Without a {CONF_TMC2209_HUB_ID} (UART), {CONF_ENN_PIN} is required to "
            f"enable/disable the driver."
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_ID): cv.declare_id(TMC2209Stepper),
            cv.Optional(CONF_AUTO_DISABLE): cv.Schema(
                {
                    cv.Optional(CONF_SETTLE, default="200ms"): cv.positive_time_period_milliseconds,
                    cv.Optional(CONF_STALLGUARD_HOMING): cv.Schema(
                        {
                            cv.Required(CONF_POSITIONS): cv.All(
                                cv.ensure_list(cv.int_), cv.Length(min=1, max=4)
                            ),
                            cv.Required(CONF_SPEED): stepper.validate_speed,
                            cv.Optional(CONF_THRESHOLD, default=50): cv.int_range(0, 255),
                            cv.Optional(CONF_TCOOLTHRS, default=300000): cv.int_range(0, 1048575),
                        }
                    ),
                }
            ),
        }
    ).extend(TMC2209_BASE_CONFIG_SCHEMA, stepper.STEPPER_SCHEMA),
    cv.has_none_or_all_keys(CONF_STEP_PIN, CONF_DIR_PIN),
    validate_control_method_,
    validate_standalone_,
    validate_tmc2209_base,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await register_tmc2209_base(var, config)
    await stepper.register_stepper(var, config)

    has_index_pin = CONF_INDEX_PIN in config
    has_stepdir_pins = CONF_STEP_PIN in config and CONF_DIR_PIN in config

    if has_index_pin:
        cg.add(var.set_control_method(ControlMethod.SERIAL_CONTROL))
    elif has_stepdir_pins:
        cg.add(var.set_control_method(ControlMethod.PULSES_CONTROL))
    else:
        raise EsphomeError("Could not determine control method!")

    if CONF_AUTO_DISABLE in config:
        ad_conf = config[CONF_AUTO_DISABLE]
        cg.add(var.set_auto_disable_ms(ad_conf[CONF_SETTLE].total_milliseconds))

        if CONF_STALLGUARD_HOMING in ad_conf:
            home_conf = ad_conf[CONF_STALLGUARD_HOMING]
            for pos in home_conf[CONF_POSITIONS]:
                cg.add(var.add_home_position(pos))
            cg.add(var.set_home_speed(home_conf[CONF_SPEED]))
            cg.add(var.set_homing_sgthrs(home_conf[CONF_THRESHOLD]))
            cg.add(var.set_homing_tcoolthrs(home_conf[CONF_TCOOLTHRS]))
