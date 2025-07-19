import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_SOUND_PRESSURE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL,
)

CONF_GAIN_20HZ = "eq_gain_band20Hz"
CONF_GAIN_31P5HZ = "eq_gain_band31.5Hz"
CONF_GAIN_50HZ = "eq_gain_band50Hz"
CONF_GAIN_80HZ = "eq_gain_band80Hz"
CONF_GAIN_125HZ = "eq_gain_band125Hz"
CONF_GAIN_200HZ = "eq_gain_band200Hz"
CONF_GAIN_315HZ = "eq_gain_band315Hz"
CONF_GAIN_500HZ = "eq_gain_band500Hz"
CONF_GAIN_800HZ = "eq_gain_band800Hz"
CONF_GAIN_1250HZ = "eq_gain_band1250Hz"
CONF_GAIN_2000HZ = "eq_gain_band2000Hz"
CONF_GAIN_3150HZ = "eq_gain_band3150Hz"
CONF_GAIN_5000HZ = "eq_gain_band5000Hz"
CONF_GAIN_8000HZ = "eq_gain_band8000Hz"
CONF_GAIN_16000HZ = "eq_gain_band16000Hz"

ICON_VOLUME_SOURCE = "mdi:volume-source"

from ..audio_dac import CONF_TAS5805M_ID, Tas5805mComponent, tas5805m_ns

EqGainBand20hz = tas5805m_ns.class_("EqGainBand20hz", number.Number, cg.Component)
EqGainBand31p5hz = tas5805m_ns.class_("EqGainBand31p5hz", number.Number, cg.Component)
EqGainBand50hz = tas5805m_ns.class_("EqGainBand50hz", number.Number, cg.Component)
EqGainBand80hz = tas5805m_ns.class_("EqGainBand80hz", number.Number, cg.Component)
EqGainBand125hz = tas5805m_ns.class_("EqGainBand125hz", number.Number, cg.Component)
EqGainBand200hz = tas5805m_ns.class_("EqGainBand200hz", number.Number, cg.Component)
EqGainBand315hz = tas5805m_ns.class_("EqGainBand315hz", number.Number, cg.Component)
EqGainBand500hz = tas5805m_ns.class_("EqGainBand500hz", number.Number, cg.Component)
EqGainBand800hz = tas5805m_ns.class_("EqGainBand800hz", number.Number, cg.Component)
EqGainBand1250hz = tas5805m_ns.class_("EqGainBand1250hz", number.Number, cg.Component)
EqGainBand2000hz = tas5805m_ns.class_("EqGainBand2000hz", number.Number, cg.Component)
EqGainBand3150hz = tas5805m_ns.class_("EqGainBand3150hz", number.Number, cg.Component)
EqGainBand5000hz = tas5805m_ns.class_("EqGainBand5000hz", number.Number, cg.Component)
EqGainBand8000hz = tas5805m_ns.class_("EqGainBand8000hz", number.Number, cg.Component)
EqGainBand16000hz = tas5805m_ns.class_("EqGainBand16000hz", number.Number, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_TAS5805M_ID): cv.use_id(Tas5805mComponent),

        cv.Required(CONF_GAIN_20HZ): number.number_schema(
            EqGainBand20hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_31P5HZ): number.number_schema(
            EqGainBand31p5hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_50HZ): number.number_schema(
            EqGainBand50hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_80HZ): number.number_schema(
            EqGainBand80hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_125HZ): number.number_schema(
            EqGainBand125hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_200HZ): number.number_schema(
            EqGainBand200hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_315HZ): number.number_schema(
            EqGainBand315hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_500HZ): number.number_schema(
            EqGainBand500hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_800HZ): number.number_schema(
            EqGainBand800hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_1250HZ): number.number_schema(
            EqGainBand1250hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_2000HZ): number.number_schema(
            EqGainBand2000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_3150HZ): number.number_schema(
            EqGainBand3150hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_5000HZ): number.number_schema(
            EqGainBand5000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_8000HZ): number.number_schema(
            EqGainBand8000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),

        cv.Required(CONF_GAIN_16000HZ): number.number_schema(
            EqGainBand16000hz,
            device_class=DEVICE_CLASS_SOUND_PRESSURE,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_VOLUME_SOURCE,
            unit_of_measurement=UNIT_DECIBEL,
        )
        .extend(cv.COMPONENT_SCHEMA),
    }
)

async def to_code(config):
    cg.add_define("USE_TAS5805M_EQ")
    tas5805m_component = await cg.get_variable(config[CONF_TAS5805M_ID])
    gain_20hz_config = config.get(CONF_GAIN_20HZ)
    n = await number.new_number(
        gain_20hz_config, min_value=-15, max_value=15, step=1
    )
    await cg.register_component(n, gain_20hz_config)
    await cg.register_parented(n, tas5805m_component)

    if gain_31p5hz_config := config.get(CONF_GAIN_31P5HZ):
        n = await number.new_number(
            gain_31p5hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_31p5hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_50hz_config := config.get(CONF_GAIN_50HZ):
        n = await number.new_number(
            gain_50hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_50hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_80hz_config := config.get(CONF_GAIN_80HZ):
        n = await number.new_number(
            gain_80hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_80hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_125hz_config := config.get(CONF_GAIN_125HZ):
        n = await number.new_number(
            gain_125hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_125hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_200hz_config := config.get(CONF_GAIN_200HZ):
        n = await number.new_number(
            gain_200hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_200hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_315hz_config := config.get(CONF_GAIN_315HZ):
        n = await number.new_number(
            gain_315hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_315hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_500hz_config := config.get(CONF_GAIN_500HZ):
        n = await number.new_number(
            gain_500hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_500hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_800hz_config := config.get(CONF_GAIN_800HZ):
        n = await number.new_number(
            gain_800hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_800hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_1250hz_config := config.get(CONF_GAIN_1250HZ):
        n = await number.new_number(
            gain_1250hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_1250hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_2000hz_config := config.get(CONF_GAIN_2000HZ):
        n = await number.new_number(
            gain_2000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_2000hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_3150hz_config := config.get(CONF_GAIN_3150HZ):
        n = await number.new_number(
            gain_3150hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_3150hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_5000hz_config := config.get(CONF_GAIN_5000HZ):
        n = await number.new_number(
            gain_5000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_5000hz_config)
        await cg.register_parented(n, tas5805m_component)


    if gain_8000hz_config := config.get(CONF_GAIN_8000HZ):
        n = await number.new_number(
            gain_8000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_8000hz_config)
        await cg.register_parented(n, tas5805m_component)

    if gain_16000hz_config := config.get(CONF_GAIN_16000HZ):
        n = await number.new_number(
            gain_16000hz_config, min_value=-15, max_value=15, step=1
        )
        await cg.register_component(n, gain_16000hz_config)
        await cg.register_parented(n, tas5805m_component)
