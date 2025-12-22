import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    UNIT_MINUTE,
    UNIT_EMPTY,
    UNIT_CELSIUS,
    ICON_TIMER,
    ICON_WATER_PERCENT,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_TEMPERATURE,
)

from .. import sauna360_ns, SAUNA360Component, CONF_SAUNA360_ID

SAUNA360BathTimeNumber = sauna360_ns.class_("SAUNA360BathTimeNumber", number.Number)
SAUNA360BathTemperatureNumber = sauna360_ns.class_(
    "SAUNA360BathTemperatureNumber", number.Number
)
SAUNA360MaxBathTemperatureNumber = sauna360_ns.class_(
    "SAUNA360MaxBathTemperatureNumber", number.Number
)
SAUNA360HumidityStepNumber = sauna360_ns.class_(
    "SAUNA360HumidityStepNumber", number.Number
)
SAUNA360HumidityPercentNumber = sauna360_ns.class_(
    "SAUNA360HumidityPercentNumber", number.Number
)

CONF_BATH_TIME = "bath_time"
CONF_BATH_TIME_DEFAULT = "bath_time_default"

CONF_BATH_TEMPERATURE = "bath_temperature"
CONF_BATH_TEMPERATURE_DEFAULT = "bath_temperature_default"

CONF_MAX_BATH_TEMPERATURE = "max_bath_temperature"
CONF_MAX_BATH_TEMPERATURE_DEFAULT = "max_bath_temperature_default"

CONF_HUMIDITY_STEP = "humidity_step"
CONF_HUMIDITY_STEP_DEFAULT = "humidity_step_default"

CONF_HUMIDITY_PERCENT = "humidity_percent"
CONF_HUMIDITY_PERCENT_DEFAULT = "humidity_percent_default"

CONF_MIN_VALUE = "min_value"
CONF_MAX_VALUE = "max_value"


def _temp_number_extend_schema() -> cv.Schema:
    return cv.Schema(
        {
            cv.Optional(CONF_MIN_VALUE, default=40.0): cv.float_range(min=40, max=110),
            cv.Optional(CONF_MAX_VALUE, default=110.0): cv.float_range(min=40, max=110),
        }
    )


def _validate_min_max_common(cfg: dict) -> tuple[float, float]:
    min_v = float(cfg.get(CONF_MIN_VALUE, 40.0))
    max_v = float(cfg.get(CONF_MAX_VALUE, 110.0))
    if min_v > max_v:
        raise cv.Invalid(
            f"{CONF_MIN_VALUE} must be <= {CONF_MAX_VALUE} (got {min_v} > {max_v})"
        )
    return min_v, max_v


def _validate_bath_temperature(cfg: dict) -> dict:
    min_v, max_v = _validate_min_max_common(cfg)
    if CONF_BATH_TEMPERATURE_DEFAULT in cfg:
        dv = float(cfg[CONF_BATH_TEMPERATURE_DEFAULT])
        if dv < min_v or dv > max_v:
            raise cv.Invalid(
                f"{CONF_BATH_TEMPERATURE_DEFAULT} must be within [{min_v}, {max_v}] (got {dv})"
            )
    return cfg


def _validate_max_bath_temperature(cfg: dict) -> dict:
    min_v, max_v = _validate_min_max_common(cfg)
    if CONF_MAX_BATH_TEMPERATURE_DEFAULT in cfg:
        dv = float(cfg[CONF_MAX_BATH_TEMPERATURE_DEFAULT])
        if dv < min_v or dv > max_v:
            raise cv.Invalid(
                f"{CONF_MAX_BATH_TEMPERATURE_DEFAULT} must be within [{min_v}, {max_v}] (got {dv})"
            )
    return cfg


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SAUNA360_ID): cv.use_id(SAUNA360Component),
        cv.Optional(CONF_BATH_TIME): number.number_schema(
            SAUNA360BathTimeNumber,
            device_class=DEVICE_CLASS_DURATION,
            unit_of_measurement=UNIT_MINUTE,
            icon=ICON_TIMER,
        ).extend(
            {
                cv.Optional(CONF_BATH_TIME_DEFAULT): cv.float_range(min=1, max=360),
            }
        ),
        cv.Optional(CONF_BATH_TEMPERATURE): cv.All(
            number.number_schema(
                SAUNA360BathTemperatureNumber,
                device_class=DEVICE_CLASS_TEMPERATURE,
                unit_of_measurement=UNIT_CELSIUS,
                icon="mdi:thermometer-lines",
            )
            .extend(
                {
                    cv.Optional(CONF_BATH_TEMPERATURE_DEFAULT): cv.float_,
                }
            )
            .extend(_temp_number_extend_schema()),
            _validate_bath_temperature,
        ),
        cv.Optional(CONF_MAX_BATH_TEMPERATURE): cv.All(
            number.number_schema(
                SAUNA360MaxBathTemperatureNumber,
                device_class=DEVICE_CLASS_TEMPERATURE,
                unit_of_measurement=UNIT_CELSIUS,
                icon="mdi:thermometer-lines",
            )
            .extend(
                {
                    cv.Optional(CONF_MAX_BATH_TEMPERATURE_DEFAULT): cv.float_,
                }
            )
            .extend(_temp_number_extend_schema()),
            _validate_max_bath_temperature,
        ),
        cv.Optional(CONF_HUMIDITY_STEP): number.number_schema(
            SAUNA360HumidityStepNumber,
            unit_of_measurement=UNIT_EMPTY,
            icon=ICON_WATER_PERCENT,
        ).extend(
            {
                cv.Optional(CONF_HUMIDITY_STEP_DEFAULT): cv.float_range(min=0, max=10),
            }
        ),
        cv.Optional(CONF_HUMIDITY_PERCENT): number.number_schema(
            SAUNA360HumidityPercentNumber,
            unit_of_measurement="%",
            icon=ICON_WATER_PERCENT,
        ).extend(
            {
                cv.Optional(CONF_HUMIDITY_PERCENT_DEFAULT): cv.float_range(
                    min=0, max=100
                ),
            }
        ),
    }
)


async def to_code(config):
    sauna360_component = await cg.get_variable(config[CONF_SAUNA360_ID])

    if bath_time := config.get(CONF_BATH_TIME):
        n = await number.new_number(bath_time, min_value=1, max_value=360, step=1)
        await cg.register_parented(n, sauna360_component)
        cg.add(sauna360_component.set_bath_time_number(n))
        if CONF_BATH_TIME_DEFAULT in bath_time:
            cg.add(
                sauna360_component.set_bath_time_default_value(
                    bath_time[CONF_BATH_TIME_DEFAULT]
                )
            )

    if bath_temperature := config.get(CONF_BATH_TEMPERATURE):
        min_v = float(bath_temperature.get(CONF_MIN_VALUE, 40.0))
        max_v = float(bath_temperature.get(CONF_MAX_VALUE, 110.0))
        n = await number.new_number(
            bath_temperature, min_value=min_v, max_value=max_v, step=1
        )
        await cg.register_parented(n, sauna360_component)
        cg.add(sauna360_component.set_bath_temperature_number(n))
        if CONF_BATH_TEMPERATURE_DEFAULT in bath_temperature:
            cg.add(
                sauna360_component.set_bath_temperature_default_value(
                    bath_temperature[CONF_BATH_TEMPERATURE_DEFAULT]
                )
            )

    if max_bath_temperature := config.get(CONF_MAX_BATH_TEMPERATURE):
        min_v = float(max_bath_temperature.get(CONF_MIN_VALUE, 40.0))
        max_v = float(max_bath_temperature.get(CONF_MAX_VALUE, 110.0))
        n = await number.new_number(
            max_bath_temperature, min_value=min_v, max_value=max_v, step=1
        )
        await cg.register_parented(n, sauna360_component)
        cg.add(sauna360_component.set_max_bath_temperature_number(n))
        if CONF_MAX_BATH_TEMPERATURE_DEFAULT in max_bath_temperature:
            cg.add(
                sauna360_component.set_max_bath_temperature_default_value(
                    max_bath_temperature[CONF_MAX_BATH_TEMPERATURE_DEFAULT]
                )
            )

    if humidity_step := config.get(CONF_HUMIDITY_STEP):
        n = await number.new_number(humidity_step, min_value=0, max_value=10, step=1)
        await cg.register_parented(n, sauna360_component)
        cg.add(sauna360_component.set_humidity_step_number(n))
        if CONF_HUMIDITY_STEP_DEFAULT in humidity_step:
            cg.add(
                sauna360_component.set_humidity_step_default_value(
                    humidity_step[CONF_HUMIDITY_STEP_DEFAULT]
                )
            )

    if humidity_percent := config.get(CONF_HUMIDITY_PERCENT):
        n = await number.new_number(
            humidity_percent, min_value=0, max_value=100, step=1
        )
        await cg.register_parented(n, sauna360_component)
        cg.add(sauna360_component.set_humidity_percent_number(n))
        if CONF_HUMIDITY_PERCENT_DEFAULT in humidity_percent:
            cg.add(
                sauna360_component.set_humidity_percent_default_value(
                    humidity_percent[CONF_HUMIDITY_PERCENT_DEFAULT]
                )
            )
