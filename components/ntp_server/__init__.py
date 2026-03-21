import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT
from esphome.components import time as time_ns

DEPENDENCIES = ["network"]

ntp_server_ns = cg.esphome_ns.namespace("ntp_server")
NTPServerComponent = ntp_server_ns.class_("NTPServerComponent", cg.Component)

CONF_TIME_ID = "time_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NTPServerComponent),
        cv.Required(CONF_TIME_ID): cv.use_id(time_ns.RealTimeClock),
        cv.Optional(CONF_PORT, default=123): cv.port,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config[CONF_PORT])
    await cg.register_component(var, config)

    time_ = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time_id(time_))
