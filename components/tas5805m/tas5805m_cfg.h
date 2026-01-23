#pragma once

namespace esphome::tas5805m {
    enum ControlState : uint8_t {
        CTRL_DEEP_SLEEP = 0x00, // Deep Sleep
        CTRL_SLEEP = 0x01, // Sleep
        CTRL_HI_Z = 0x02, // Hi-Z
        CTRL_PLAY = 0x03, // Play
    };

    enum DacMode : uint8_t {
        BTL = 0, // Bridge tied load
        PBTL = 1, // Parallel load
    };

    enum MixerMode : uint8_t {
        STEREO = 0,
        STEREO_INVERSE,
        MONO,
        RIGHT,
        LEFT,
    };

    static const char *const MIXER_MODE_TEXT[] = {"STEREO", "STEREO_INVERSE", "MONO", "RIGHT", "LEFT"};

    struct Tas5805mConfiguration {
        uint8_t offset;
        uint8_t value;
    }__attribute__((packed));

    struct Tas5805mFault {
        uint8_t channel_fault{0}; // individual faults extracted when publishing
        uint8_t global_fault{0}; // individual faults extracted when publishing

        bool clock_fault{false};
        bool temperature_fault{false};
        bool temperature_warning{false};

        bool is_fault_except_clock_fault{false}; // fault conditions combined except clock fault

#ifdef USE_TAS5805M_BINARY_SENSOR
    bool have_fault{false};                    // combined binary sensor - any fault found but does not include clock fault if excluded
#endif
    };

    // Startup sequence codes
    static constexpr uint8_t TAS5805M_CFG_META_DELAY = 254;

    static constexpr float TAS5805M_MIN_ANALOG_GAIN = -15.5;
    static constexpr float TAS5805M_MAX_ANALOG_GAIN = 0.0;

    // set book and page registers
    static constexpr uint8_t TAS5805M_REG_PAGE_SET = 0x00;
    static constexpr uint8_t TAS5805M_REG_BOOK_SET = 0x7F;
    static constexpr uint8_t TAS5805M_REG_BOOK_CONTROL_PORT = 0x00;
    static constexpr uint8_t TAS5805M_REG_PAGE_ZERO = 0x00;

    // tas5805m registers
    static constexpr uint8_t TAS5805M_DEVICE_CTRL_1 = 0x02;
    static constexpr uint8_t TAS5805M_DEVICE_CTRL_2 = 0x03;
    static constexpr uint8_t TAS5805M_FS_MON = 0x37;
    static constexpr uint8_t TAS5805M_BCK_MON = 0x38;
    static constexpr uint8_t TAS5805M_DIG_VOL_CTRL = 0x4C;
    static constexpr uint8_t TAS5805M_ANA_CTRL = 0x53;
    static constexpr uint8_t TAS5805M_AGAIN = 0x54;
    static constexpr uint8_t TAS5805M_DSP_MISC = 0x66;
    static constexpr uint8_t TAS5805M_POWER_STATE = 0x68;

    // TAS5805M_REG_FAULT register values
    static constexpr uint8_t TAS5805M_CHAN_FAULT = 0x70;
    static constexpr uint8_t TAS5805M_GLOBAL_FAULT1 = 0x71;
    static constexpr uint8_t TAS5805M_GLOBAL_FAULT2 = 0x72;
    static constexpr uint8_t TAS5805M_OT_WARNING = 0x73;
    static constexpr uint8_t TAS5805M_FAULT_CLEAR = 0x78;
    static constexpr uint8_t TAS5805M_ANALOG_FAULT_CLEAR = 0x80;

    // EQ registers
    static constexpr uint8_t TAS5805M_CTRL_EQ_ON = 0x00;
    static constexpr uint8_t TAS5805M_CTRL_EQ_OFF = 0x01;

    // Level meter register

    // Mixer registers
    static constexpr uint8_t TAS5805M_REG_BOOK_5 = 0x8C;
    static constexpr uint8_t TAS5805M_REG_BOOK_5_MIXER_PAGE = 0x29;
    static constexpr uint8_t TAS5805M_REG_LEFT_TO_LEFT_GAIN = 0x18;
    static constexpr uint8_t TAS5805M_REG_RIGHT_TO_LEFT_GAIN = 0x1C;
    static constexpr uint8_t TAS5805M_REG_LEFT_TO_RIGHT_GAIN = 0x20;
    static constexpr uint8_t TAS5805M_REG_RIGHT_TO_RIGHT_GAIN = 0x24;
    static constexpr uint8_t TAS5805M_REG_BOOK_5_VOLUME_PAGE = 0x2A;
    static constexpr uint8_t TAS5805M_REG_LEFT_VOLUME = 0x24;
    static constexpr uint8_t TAS5805M_REG_RIGHT_VOLUME = 0x28;
    static constexpr uint32_t TAS5805M_MIXER_VALUE_MUTE = 0x00000000;
    static constexpr uint32_t TAS5805M_MIXER_VALUE_0DB = 0x00008000;
    //static constexpr uint32_t TAS5805M_MIXER_VALUE_MINUS6DB    = 0xE7264000;
    static constexpr uint32_t TAS5805M_MIXER_VALUE_MINUS6DB = 0x00004000;

    // Crossbar registers
    static constexpr uint32_t TAS5805M_CROSSBAR_VALUE_MUTE = 0x00000000;
    static constexpr uint32_t TAS5805M_CROSSBAR_VALUE_0DB = 0x00008000;

    static constexpr uint8_t TAS5805M_REG_OUTPUT_CROSSBAR_BEGINNING = 0x1C;
    // Crossbar flags
    enum CrossbarConfig {
        OUTPUT_CROSSBAR_LEFT_TO_AMP_LEFT = 1 << 0,
        OUTPUT_CROSSBAR_RIGHT_TO_AMP_LEFT = 1 << 1,
        OUTPUT_CROSSBAR_MONO_TO_AMP_LEFT = 1 << 2,
        OUTPUT_CROSSBAR_LEFT_TO_AMP_RIGHT = 1 << 3,
        OUTPUT_CROSSBAR_RIGHT_TO_AMP_RIGHT = 1 << 4,
        OUTPUT_CROSSBAR_MONO_TO_AMP_RIGHT = 1 << 5,
        OUTPUT_CROSSBAR_LEFT_TO_I2S_LEFT = 1 << 6,
        OUTPUT_CROSSBAR_RIGHT_TO_I2S_LEFT = 1 << 7,
        OUTPUT_CROSSBAR_MONO_TO_I2S_LEFT = 1 << 8,
        OUTPUT_CROSSBAR_LEFT_TO_I2S_RIGHT = 1 << 9,
        OUTPUT_CROSSBAR_RIGHT_TO_I2S_RIGHT = 1 << 10,
        OUTPUT_CROSSBAR_MONO_TO_I2S_RIGHT = 1 << 11,
    };
    static constexpr uint8_t CROSSBAR_CONFIG_COUNT = 12;

    enum MonoMixerMode: uint8_t {
        MONO_MIXER_MODE_LEFT = 0,
        MONO_MIXER_MODE_RIGHT,
        MONO_MIXER_MODE_STEREO,
        MONO_MIXER_MODE_EQ_LEFT,
        MONO_MIXER_MODE_EQ_RIGHT,
      };
    static constexpr auto MONO_MIXER_MODE_TEXT[] = {"LEFT", "RIGHT", "STEREO", "EQ_LEFT", "EQ_RIGHT"};

} // namespace esphome::tas5805m
