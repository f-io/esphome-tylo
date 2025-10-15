import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import sauna360_ns, SAUNA360Component, CONF_SAUNA360_ID

SAUNA360TextSensor = sauna360_ns.class_("SAUNA360TextSensor", text_sensor.TextSensor, cg.Component)

CONF_HEATER_STATE = "heater_state"

CONFIG_SCHEMA = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(SAUNA360TextSensor),
            cv.GenerateID(CONF_SAUNA360_ID): cv.use_id(SAUNA360Component),
            cv.Optional(CONF_HEATER_STATE): text_sensor.text_sensor_schema(
                icon="mdi:heat-wave",
            ),
        }
    ),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_HEATER_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_HEATER_STATE])
        cg.add(var.set_heater_state_text_sensor(sens))
    sauna360 = await cg.get_variable(config[CONF_SAUNA360_ID])
    cg.add(sauna360.register_listener(var))
