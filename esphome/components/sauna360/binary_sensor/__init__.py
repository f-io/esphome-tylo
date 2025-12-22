import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_LIGHT,
    DEVICE_CLASS_SAFETY,
    ICON_LIGHTBULB,
)
from .. import (
    sauna360_ns,
    SAUNA360Component,
    CONF_SAUNA360_ID,
)

SAUNA360BinarySensor = sauna360_ns.class_(
    "SAUNA360BinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_HEATER_STATUS = "heater_status"
CONF_LIGHT_STATUS = "light_status"
CONF_READY_STATUS = "ready_status"

CONFIG_SCHEMA = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(SAUNA360BinarySensor),
            cv.GenerateID(CONF_SAUNA360_ID): cv.use_id(SAUNA360Component),
            cv.Optional(CONF_HEATER_STATUS): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_HEAT, icon="mdi:radiator-disabled"
            ),
            cv.Optional(CONF_LIGHT_STATUS): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_LIGHT,
                icon=ICON_LIGHTBULB,
            ),
            cv.Optional(CONF_READY_STATUS): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_SAFETY,
                icon="mdi:security",
            ),
        }
    ),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_HEATER_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_HEATER_STATUS])
        cg.add(var.set_heater_binary_sensor(sens))
    if CONF_LIGHT_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_LIGHT_STATUS])
        cg.add(var.set_light_binary_sensor(sens))
    if CONF_READY_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_READY_STATUS])
        cg.add(var.set_ready_binary_sensor(sens))
    sauna360 = await cg.get_variable(config[CONF_SAUNA360_ID])
    cg.add(sauna360.register_listener(var))
