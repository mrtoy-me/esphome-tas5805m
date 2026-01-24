import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG

from ..audio_dac import CONF_TAS5805M_ID, Tas5805mComponent, tas5805m_ns

DEPENDENCIES = ["tas5805"]

EqModeSelect = tas5805m_ns.class_("EqMode", select.Select, cg.Component)

CONF_EQ_MODE = "eq_mode"

CONFIG_SCHEMA = {
    cv.GenerateID(CONF_TAS5805M_ID): cv.use_id(Tas5805mComponent),
    cv.Optional(CONF_EQ_MODE): select.select_schema(
        EqModeSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
    ),
}


async def to_code(config):
    tas5805m_component = await cg.get_variable(config[CONF_TAS5805M_ID])
    if eq_mode_config := config.get(CONF_EQ_MODE):
        s = await select.new_select(
            eq_mode_config,
            options=["Off", "EQ 15 Band", "EQ 15 Band BIAMP", "EQ BIAMP Presets"],
        )
        await cg.register_component(s, eq_mode_config)
        await cg.register_parented(s, tas5805m_component)
        cg.add(tas5805m_component.set_eq_mode_select(s))


