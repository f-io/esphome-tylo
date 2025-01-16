import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    UNIT_MINUTE,
    UNIT_SECOND,
    UNIT_EMPTY,
    UNIT_CELSIUS,
    ICON_TIMER,
    ICON_WATER_PERCENT,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_TEMPERATURE,

)
from .. import (
    sauna360_ns,
    SAUNA360Component,
    CONF_SAUNA360_ID,
)

SAUNA360BathTimeNumber = sauna360_ns.class_("SAUNA360BathTimeNumber", number.Number)
SAUNA360BathTemperatureNumber = sauna360_ns.class_("SAUNA360BathTemperatureNumber", number.Number)
SAUNA360MaxBathTemperatureNumber = sauna360_ns.class_("SAUNA360MaxBathTemperatureNumber", number.Number)
SAUNA360OverheatingPCBLimitNumber = sauna360_ns.class_("SAUNA360OverheatingPCBLimitNumber", number.Number)


CONF_BATH_TIME = "bath_time"
CONF_BATH_TIME_DEFAULT = "bath_time_default"
CONF_BATH_TEMPERATURE = "bath_temperature"
CONF_BATH_TEMPERATURE_DEFAULT = "bath_temperature_default"
CONF_MAX_BATH_TEMPERATURE = "max_bath_temperature"
CONF_MAX_BATH_TEMPERATURE_DEFAULT = "max_bath_temperature_default"
CONF_OVERHEATING_PCB_LIMIT = "overheating_pcb_limit"
CONF_OVERHEATING_PCB_LIMIT_DEFAULT = "overheating_pcb_limit_default"

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
        cv.Optional(CONF_BATH_TEMPERATURE): number.number_schema(
            SAUNA360BathTemperatureNumber,
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:thermometer-lines",
        ).extend(
            {
                cv.Optional(CONF_BATH_TEMPERATURE_DEFAULT): cv.float_range(min=40, max=110),
            }
        ),
        cv.Optional(CONF_MAX_BATH_TEMPERATURE): number.number_schema(
            SAUNA360MaxBathTemperatureNumber,
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:thermometer-lines",
        ).extend(
            {
                cv.Optional(CONF_MAX_BATH_TEMPERATURE_DEFAULT): cv.float_range(min=40, max=110),
            }
        ),
        cv.Optional(CONF_OVERHEATING_PCB_LIMIT): number.number_schema(
            SAUNA360OverheatingPCBLimitNumber,
            device_class=DEVICE_CLASS_TEMPERATURE,
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:thermometer-alert",
        ).extend(
            {
                cv.Optional(CONF_OVERHEATING_PCB_LIMIT_DEFAULT): cv.float_range(min=70, max=90),
            }
        ),
    }
)

async def to_code(config):
    sauna360_component = await cg.get_variable(config[CONF_SAUNA360_ID])
    if bath_time := config.get(CONF_BATH_TIME):
      n = await number.new_number(
        bath_time, min_value=1, max_value=360, step=1,
      )
      await cg.register_parented(n, sauna360_component)
      cg.add(sauna360_component.set_bath_time_number(n))
      if CONF_BATH_TIME_DEFAULT in bath_time:
        cg.add(sauna360_component.set_bath_time_default_value(bath_time[CONF_BATH_TIME_DEFAULT])
      )   
    if bath_temperature := config.get(CONF_BATH_TEMPERATURE):
      n = await number.new_number(
        bath_temperature, min_value=40, max_value=110, step=1,
      )
      await cg.register_parented(n, sauna360_component)
      cg.add(sauna360_component.set_bath_temperature_number(n))
      if CONF_BATH_TEMPERATURE_DEFAULT in bath_temperature:
        cg.add(sauna360_component.set_bath_temperature_default_value(bath_temperature[CONF_BATH_TEMPERATURE_DEFAULT])
      )        
    if max_bath_temperature := config.get(CONF_MAX_BATH_TEMPERATURE):
      n = await number.new_number(
        max_bath_temperature, min_value=40, max_value=110, step=1,
      )
      await cg.register_parented(n, sauna360_component)
      cg.add(sauna360_component.set_max_bath_temperature_number(n))
      if CONF_MAX_BATH_TEMPERATURE_DEFAULT in max_bath_temperature:
        cg.add(sauna360_component.set_max_bath_temperature_default_value(max_bath_temperature[CONF_MAX_BATH_TEMPERATURE_DEFAULT])
      )       
    if overheating_pcb_limit := config.get(CONF_OVERHEATING_PCB_LIMIT):
      n = await number.new_number(
        overheating_pcb_limit, min_value=70, max_value=90, step=1,
      )
      await cg.register_parented(n, sauna360_component)
      cg.add(sauna360_component.set_overheating_pcb_limit_number(n))
      if CONF_OVERHEATING_PCB_LIMIT_DEFAULT in overheating_pcb_limit:
        cg.add(sauna360_component.set_overheating_pcb_limit_default_value(overheating_pcb_limit[CONF_OVERHEATING_PCB_LIMIT_DEFAULT])
      )