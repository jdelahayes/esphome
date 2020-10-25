import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

AUTO_LOAD = ["climate_ir"]

mitsubishi_km05g_ns = cg.esphome_ns.namespace("mitsubishi_km05g")
mitsubishi_km05gClimate = mitsubishi_km05g_ns.class_(
    "mitsubishi_km05gClimate", climate_ir.ClimateIR
)

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(mitsubishi_km05gClimate),
    }
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield climate_ir.register_climate_ir(var, config)
