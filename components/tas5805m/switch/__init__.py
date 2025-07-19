import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_SWITCH

CONF_ENABLE_DAC = "enable_dac"
CONF_ENABLE_EQ = "enable_eq"

from ..audio_dac import CONF_TAS5805M_ID, Tas5805mComponent, tas5805m_ns

EnableDacSwitch = tas5805m_ns.class_("EnableDacSwitch", switch.Switch, cg.Component)
EnableEqSwitch = tas5805m_ns.class_("EnableEqSwitch", switch.Switch , cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TAS5805M_ID): cv.use_id(Tas5805mComponent),

        cv.Optional(CONF_ENABLE_DAC): switch.switch_schema(
            EnableDacSwitch,
            device_class=DEVICE_CLASS_SWITCH,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Optional(CONF_ENABLE_EQ): switch.switch_schema(
            EnableEqSwitch,
            device_class=DEVICE_CLASS_SWITCH,
        )
        .extend(cv.COMPONENT_SCHEMA),
    }
)

async def to_code(config):
  tas5805m_component = await cg.get_variable(config[CONF_TAS5805M_ID])
  if enable_dac_config := config.get(CONF_ENABLE_DAC):
    s = await switch.new_switch(enable_dac_config)
    await cg.register_component(s, enable_dac_config)
    await cg.register_parented(s, tas5805m_component)

  if enable_eq_config := config.get(CONF_ENABLE_EQ):
    s = await switch.new_switch(enable_eq_config)
    await cg.register_component(s, enable_eq_config)
    await cg.register_parented(s, tas5805m_component)

