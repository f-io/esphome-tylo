import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, number, switch
from esphome.const import CONF_ID

from .. import sauna360_ns, SAUNA360Component, CONF_SAUNA360_ID

Sauna360Climate = sauna360_ns.class_("Sauna360Climate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(Sauna360Climate).extend(
    {
        cv.GenerateID(): cv.declare_id(Sauna360Climate),
        cv.GenerateID(CONF_SAUNA360_ID): cv.use_id(SAUNA360Component),
        cv.Optional("bath_temperature_number_id"): cv.use_id(number.Number),
        cv.Optional("heater_relay_id"): cv.use_id(switch.Switch),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SAUNA360_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    cg.add(var.set_controller(parent))

    if "bath_temperature_number_id" in config:
        bath_temp = await cg.get_variable(config["bath_temperature_number_id"])
        cg.add(var.set_bath_temperature_number(bath_temp))

    if "heater_relay_id" in config:
        heater_relay = await cg.get_variable(config["heater_relay_id"])
        cg.add(var.set_heater_relay(heater_relay))
