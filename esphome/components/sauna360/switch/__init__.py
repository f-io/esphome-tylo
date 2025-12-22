import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_SWITCH,
    ICON_LIGHTBULB,
    ENTITY_CATEGORY_CONFIG,
)
from .. import (
    sauna360_ns,
    SAUNA360Component,
    CONF_SAUNA360_ID,
)

SAUNA360LightRelaySwitch = sauna360_ns.class_(
    "SAUNA360LightRelaySwitch", switch.Switch, cg.Component
)
SAUNA360HeaterRelaySwitch = sauna360_ns.class_(
    "SAUNA360HeaterRelaySwitch", switch.Switch, cg.Component
)


CONF_LIGHT_RELAY = "light_relay"
CONF_HEATER_RELAY = "heater_relay"


CONFIG_SCHEMA = {
    cv.GenerateID(CONF_SAUNA360_ID): cv.use_id(SAUNA360Component),
    cv.Optional(CONF_LIGHT_RELAY): switch.switch_schema(
        SAUNA360LightRelaySwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon=ICON_LIGHTBULB,
    ),
    cv.Optional(CONF_HEATER_RELAY): switch.switch_schema(
        SAUNA360HeaterRelaySwitch,
        device_class=DEVICE_CLASS_SWITCH,
        icon="mdi:power",
    ),
}


async def to_code(config):
    sauna360_component = await cg.get_variable(config[CONF_SAUNA360_ID])
    if light_relay := config.get(CONF_LIGHT_RELAY):
        s = await switch.new_switch(light_relay)
        await cg.register_parented(s, config[CONF_SAUNA360_ID])
        cg.add(sauna360_component.set_light_relay_switch(s))
    if heater_relay := config.get(CONF_HEATER_RELAY):
        s = await switch.new_switch(heater_relay)
        await cg.register_parented(s, config[CONF_SAUNA360_ID])
        cg.add(sauna360_component.set_heater_relay_switch(s))
