import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]

sauna360_ns = cg.esphome_ns.namespace("sauna360")
SAUNA360Component = sauna360_ns.class_(
    "SAUNA360Component", cg.Component, uart.UARTDevice
)
Mode = sauna360_ns.enum("SAUNA360Component::Mode")

CONF_SAUNA360_ID = "sauna360_id"

CONF_MODEL = "model"
MODEL_OPTIONS = {
    "pure": cg.RawExpression("esphome::sauna360::SAUNA360Component::Mode::PURE"),
    "combi": cg.RawExpression("esphome::sauna360::SAUNA360Component::Mode::COMBI"),
    "elite": cg.RawExpression("esphome::sauna360::SAUNA360Component::Mode::ELITE"),
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SAUNA360Component),
            cv.Optional(CONF_MODEL, default="pure"): cv.enum(MODEL_OPTIONS, lower=True),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "sauna360_uart",
    require_tx=True,
    require_rx=True,
    baud_rate=19200,
    parity="EVEN",
    data_bits=8,
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_mode(config[CONF_MODEL]))
