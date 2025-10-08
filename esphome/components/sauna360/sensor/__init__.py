import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_HUMIDITY,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_MINUTE,
    ICON_THERMOMETER,
    ICON_TIMER,
)

from .. import (
    sauna360_ns,
    SAUNA360Component,
    CONF_SAUNA360_ID,
)

SAUNA360Sensor = sauna360_ns.class_("SAUNA360Sensor", sensor.Sensor, cg.Component) 

CONF_CURRENT_TEMPERATURE = "current_temperature"
CONF_SETTING_TEMPERATURE = "setting_temperature"
CONF_REMAINING_TIME = "remaining_time"
CONF_SETTING_BATH_TIME = "setting_bath_time"
CONF_TOTAL_UPTIME = "total_uptime"
CONF_MAX_BATH_TEMPERATURE = "max_bath_temperature"
CONF_OVERHEATING_PCB_LIMIT = "overheating_pcb_limit"
CONF_SETTING_HUMIDITY_STEP = "setting_humidity_step"
CONF_SETTING_HUMIDITY = "setting_humidity"
CONF_WATER_TANK_LEVEL = "water_tank_level"

CONFIG_SCHEMA = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
          cv.GenerateID(): cv.declare_id(SAUNA360Sensor),
          cv.GenerateID(CONF_SAUNA360_ID): cv.use_id(SAUNA360Component),
          cv.Optional(CONF_CURRENT_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_THERMOMETER,
            ),
          cv.Optional(CONF_SETTING_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_THERMOMETER,
            ),
          cv.Optional(CONF_REMAINING_TIME): sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:counter",
            ),
          cv.Optional(CONF_SETTING_BATH_TIME): sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_TIMER,
            ),
          cv.Optional(CONF_TOTAL_UPTIME): sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:counter",
            ),
          cv.Optional(CONF_MAX_BATH_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_THERMOMETER,
            ),
          cv.Optional(CONF_OVERHEATING_PCB_LIMIT): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:thermometer-alert",
            ),
          cv.Optional(CONF_SETTING_HUMIDITY_STEP): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:water-percent",
            ),
          cv.Optional(CONF_SETTING_HUMIDITY): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:water-percent",
            ),
          cv.Optional(CONF_WATER_TANK_LEVEL): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:water",
            ),
        }
    ),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_CURRENT_TEMPERATURE in config:
      sens = await sensor.new_sensor(config[CONF_CURRENT_TEMPERATURE])
      cg.add(var.set_temperature_sensor(sens))
    if CONF_SETTING_TEMPERATURE in config:
      sens = await sensor.new_sensor(config[CONF_SETTING_TEMPERATURE])
      cg.add(var.set_temperature_setting_sensor(sens))
    if CONF_REMAINING_TIME in config:
      sens = await sensor.new_sensor(config[CONF_REMAINING_TIME])
      cg.add(var.set_remaining_time_sensor(sens))
    if CONF_SETTING_BATH_TIME in config:
      sens = await sensor.new_sensor(config[CONF_SETTING_BATH_TIME])
      cg.add(var.set_bath_time_setting_sensor(sens))
    if CONF_TOTAL_UPTIME in config:
      sens = await sensor.new_sensor(config[CONF_TOTAL_UPTIME])
      cg.add(var.set_total_uptime_sensor(sens))
    if CONF_MAX_BATH_TEMPERATURE in config:
      sens = await sensor.new_sensor(config[CONF_MAX_BATH_TEMPERATURE])
      cg.add(var.set_max_bath_temperature_sensor(sens))
    if CONF_OVERHEATING_PCB_LIMIT in config:
      sens = await sensor.new_sensor(config[CONF_OVERHEATING_PCB_LIMIT])
      cg.add(var.set_overheating_pcb_limit_sensor(sens))
    if CONF_SETTING_HUMIDITY_STEP in config:
      sens = await sensor.new_sensor(config[CONF_SETTING_HUMIDITY_STEP])
      cg.add(var.set_setting_humidity_step_sensor(sens))
    if CONF_SETTING_HUMIDITY in config:
      sens = await sensor.new_sensor(config[CONF_SETTING_HUMIDITY])
      cg.add(var.set_setting_humidity_percent_sensor(sens))
    if CONF_WATER_TANK_LEVEL in config:
      sens = await sensor.new_sensor(config[CONF_WATER_TANK_LEVEL])
      cg.add(var.set_water_tank_level_sensor(sens))
    sauna360 = await cg.get_variable(config[CONF_SAUNA360_ID])
    cg.add(sauna360.register_listener(var))