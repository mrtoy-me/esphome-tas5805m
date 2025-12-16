import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.components.audio_dac import AudioDac
from esphome import pins

from esphome.const import (
    CONF_ID,
    CONF_ENABLE_PIN,
)

CODEOWNERS = ["@mrtoy-me"]
DEPENDENCIES = ["i2c"]

CONF_ANALOG_GAIN = "analog_gain"
CONF_DAC_MODE = "dac_mode"
CONF_IGNORE_FAULT = "ignore_fault"
CONF_MIXER_MODE = "mixer_mode"
CONF_REFRESH_EQ = "refresh_eq"
CONF_VOLUME_MIN = "volume_min"
CONF_VOLUME_MAX = "volume_max"
CONF_TAS5805M_ID = "tas5805m_id"

tas5805m_ns = cg.esphome_ns.namespace("tas5805m")
Tas5805mComponent = tas5805m_ns.class_("Tas5805mComponent", AudioDac, cg.PollingComponent, i2c.I2CDevice)

AutoRefreshMode = tas5805m_ns.enum("AutoRefreshMode")
AUTO_REFRESH_MODES = {
     "BY_GAIN"  : AutoRefreshMode.BY_GAIN,
     "BY_SWITCH": AutoRefreshMode.BY_SWITCH,
}

DacMode = tas5805m_ns.enum("DacMode")
DAC_MODES = {
    "BTL" : DacMode.BTL,
    "PBTL": DacMode.PBTL,
}

ExcludeIgnoreMode = tas5805m_ns.enum("ExcludeIgnoreModes")
EXCLUDE_IGNORE_MODES = {
     "NONE"        : ExcludeIgnoreMode.NONE,
     "CLOCK_FAULT" : ExcludeIgnoreMode.CLOCK_FAULT,
}
MixerMode = tas5805m_ns.enum("MixerMode")
MIXER_MODES = {
    "STEREO"         : MixerMode.STEREO,
    "STEREO_INVERSE" : MixerMode.STEREO_INVERSE,
    "MONO"           : MixerMode.MONO,
    "RIGHT"          : MixerMode.RIGHT,
    "LEFT"           : MixerMode.LEFT,
}

ANALOG_GAINS = [-15.5, -15, -14.5, -14, -13.5, -13, -12.5, -12, -11.5, -11, -10.5, -10, -9.5, -9, -8.5, -8,
                 -7.5,  -7,  -6.5,  -6,  -5.5,  -5,  -4.5,  -4,  -3.5,  -3,  -2.5,  -2, -1.5, -1, -0.5,  0]

def validate_config(config):
    if config[CONF_DAC_MODE] == "PBTL" and (config[CONF_MIXER_MODE] == "STEREO" or config[CONF_MIXER_MODE] == "STEREO_INVERSE"):
        raise cv.Invalid("dac_mode: PBTL must have mixer_mode: MONO or RIGHT or LEFT")
    if (config[CONF_VOLUME_MAX] - config[CONF_VOLUME_MIN]) < 9:
        raise cv.Invalid("volume_max must at least 9db greater than volume_min")
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Tas5805mComponent),
            cv.Required(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_ANALOG_GAIN, default="-15.5dB"): cv.All(
                        cv.decibel, cv.one_of(*ANALOG_GAINS)
            ),
            cv.Optional(CONF_DAC_MODE, default="BTL"): cv.enum(
                        DAC_MODES, upper=True
            ),
            cv.Optional(CONF_IGNORE_FAULT, default="CLOCK_FAULT"): cv.enum(
                        EXCLUDE_IGNORE_MODES, upper=True
            ),
            cv.Optional(CONF_MIXER_MODE, default="STEREO"): cv.enum(
                        MIXER_MODES, upper=True
            ),
            cv.Optional(CONF_REFRESH_EQ, default="BY_GAIN"): cv.enum(
                        AUTO_REFRESH_MODES, upper=True
            ),
            cv.Optional(CONF_VOLUME_MAX, default="24dB"): cv.All(
                        cv.decibel, cv.int_range(-103, 24)
            ),
            cv.Optional(CONF_VOLUME_MIN, default="-103dB"): cv.All(
                        cv.decibel, cv.int_range(-103, 24)
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x2D))
    .add_extra(validate_config),
    cv.only_with_esp_idf,
    cv.only_on_esp32,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
    cg.add(var.set_enable_pin(enable))
    cg.add(var.config_analog_gain(config[CONF_ANALOG_GAIN]))
    cg.add(var.config_dac_mode(config[CONF_DAC_MODE]))
    cg.add(var.config_ignore_fault_mode(config[CONF_IGNORE_FAULT]))
    cg.add(var.config_mixer_mode(config[CONF_MIXER_MODE]))
    cg.add(var.config_refresh_eq(config[CONF_REFRESH_EQ]))
    cg.add(var.config_volume_max(config[CONF_VOLUME_MAX]))
    cg.add(var.config_volume_min(config[CONF_VOLUME_MIN]))
