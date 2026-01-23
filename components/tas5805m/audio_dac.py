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
CONF_MONO_MIXER_MODE = "mono_mixer_mode"
CONF_CROSSOVER_FREQUENCY = "crossover_frequency"
CONF_CROSSBAR = "crossbar"
CONF_REFRESH_EQ = "refresh_eq"
CONF_VOLUME_MIN = "volume_min"
CONF_VOLUME_MAX = "volume_max"
CONF_TAS5805M_ID = "tas5805m_id"

tas5805m_ns = cg.esphome_ns.namespace("tas5805m")
Tas5805mComponent = tas5805m_ns.class_("Tas5805mComponent", AudioDac, cg.PollingComponent, i2c.I2CDevice)

AutoRefreshMode = tas5805m_ns.enum("AutoRefreshMode")
AUTO_REFRESH_MODES = {
    "BY_GAIN": AutoRefreshMode.BY_GAIN,
    "BY_SWITCH": AutoRefreshMode.BY_SWITCH,
}

DacMode = tas5805m_ns.enum("DacMode")
DAC_MODES = {
    "BTL": DacMode.BTL,
    "PBTL": DacMode.PBTL,
}

ExcludeIgnoreMode = tas5805m_ns.enum("ExcludeIgnoreModes")
EXCLUDE_IGNORE_MODES = {
    "NONE": ExcludeIgnoreMode.NONE,
    "CLOCK_FAULT": ExcludeIgnoreMode.CLOCK_FAULT,
}
MixerMode = tas5805m_ns.enum("MixerMode")
MIXER_MODES = {
    "STEREO": MixerMode.STEREO,
    "STEREO_INVERSE": MixerMode.STEREO_INVERSE,
    "MONO": MixerMode.MONO,
    "RIGHT": MixerMode.RIGHT,
    "LEFT": MixerMode.LEFT,
}

MonoMixerMode = tas5805m_ns.enum("MonoMixerMode")
MONO_MIXER_MODES = {
    "LEFT": MonoMixerMode.MONO_MIXER_MODE_LEFT,
    "RIGHT": MonoMixerMode.MONO_MIXER_MODE_RIGHT,
    "STEREO": MonoMixerMode.MONO_MIXER_MODE_STEREO,
    "EQ_LEFT": MonoMixerMode.MONO_MIXER_MODE_EQ_LEFT,
    "EQ_RIGHT": MonoMixerMode.MONO_MIXER_MODE_EQ_RIGHT,
}

ANALOG_GAINS = [-15.5, -15, -14.5, -14, -13.5, -13, -12.5, -12, -11.5, -11, -10.5, -10, -9.5, -9, -8.5, -8,
                -7.5, -7, -6.5, -6, -5.5, -5, -4.5, -4, -3.5, -3, -2.5, -2, -1.5, -1, -0.5, 0]


def validate_config(config):
    if config[CONF_DAC_MODE] == "PBTL" and (
            config[CONF_MIXER_MODE] == "STEREO" or config[CONF_MIXER_MODE] == "STEREO_INVERSE"):
        raise cv.Invalid("dac_mode: PBTL must have mixer_mode: MONO or RIGHT or LEFT")
    if (config[CONF_VOLUME_MAX] - config[CONF_VOLUME_MIN]) < 9:
        raise cv.Invalid("volume_max must at least 9db greater than volume_min")
    return config


CROSSBAR_LEFT_TO_AMP_LEFT = "l_to_amp_l",
CROSSBAR_RIGHT_TO_AMP_LEFT = "r_to_amp_l"
CROSSBAR_MONO_TO_AMP_LEFT = "mono_to_amp_l"
CROSSBAR_LEFT_TO_AMP_RIGHT = "l_to_amp_r"
CROSSBAR_RIGHT_TO_AMP_RIGHT = "r_to_amp_r"
CROSSBAR_MONO_TO_AMP_RIGHT = "mono_to_amp_r"
CROSSBAR_LEFT_TO_I2S_LEFT = "left_to_i2s_l"
CROSSBAR_RIGHT_TO_I2S_LEFT = "right_to_i2s_l"
CROSSBAR_MONO_TO_I2S_LEFT = "mono_to_i2s_l"
CROSSBAR_LEFT_TO_I2S_RIGHT = "l_to_i2s_r"
CROSSBAR_RIGHT_TO_I2S_RIGHT = "r_to_i2s_r"
CROSSBAR_MONO_TO_I2S_RIGHT = "mono_to_i2s_r"

CrossBar = tas5805m_ns.enum("CrossbarConfig")
CROSSBAR_CONFIGS = {
    CROSSBAR_LEFT_TO_AMP_LEFT: 1 << 0,
    CROSSBAR_RIGHT_TO_AMP_LEFT: 1 << 1,
    CROSSBAR_MONO_TO_AMP_LEFT: 1 << 2,
    CROSSBAR_LEFT_TO_AMP_RIGHT: 1 << 3,
    CROSSBAR_RIGHT_TO_AMP_RIGHT: 1 << 4,
    CROSSBAR_MONO_TO_AMP_RIGHT: 1 << 5,
    CROSSBAR_LEFT_TO_I2S_LEFT: 1 << 6,
    CROSSBAR_RIGHT_TO_I2S_LEFT: 1 << 7,
    CROSSBAR_MONO_TO_I2S_LEFT: 1 << 8,
    CROSSBAR_LEFT_TO_I2S_RIGHT: 1 << 9,
    CROSSBAR_RIGHT_TO_I2S_RIGHT: 1 << 10,
    CROSSBAR_MONO_TO_I2S_RIGHT: 1 << 11
}


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
            cv.Optional(CONF_MONO_MIXER_MODE, default="STEREO"): cv.enum(
                MONO_MIXER_MODES, upper=True
            ),
            cv.Optional(CONF_CROSSOVER_FREQUENCY, default="-1Hz"): cv.All(
                cv.frequency, cv.int_range(0, 25000)
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
            cv.Optional(CONF_CROSSBAR): cv.Schema({cv.Optional(name, default="false"): cv.boolean for name in CROSSBAR_CONFIGS})
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x2D))
    .add_extra(validate_config),
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
    cg.add(var.config_mono_mixer_mode(config[CONF_MONO_MIXER_MODE]))
    cg.add(var.config_crossover_frequency(config[CONF_CROSSOVER_FREQUENCY]))
    for k, v in config[CONF_CROSSBAR]:
        enum_val = CROSSBAR_CONFIGS[k]
        cg.add(var.config_crossbar_flag(enum_val, v))
    cg.add(var.config_refresh_eq(config[CONF_REFRESH_EQ]))
    cg.add(var.config_volume_max(config[CONF_VOLUME_MAX]))
    cg.add(var.config_volume_min(config[CONF_VOLUME_MIN]))
