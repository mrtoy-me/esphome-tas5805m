#pragma once

namespace esphome::tas5805m {

  enum ControlState : uint8_t {
    CTRL_DEEP_SLEEP = 0x00, // Deep Sleep
    CTRL_SLEEP      = 0x01, // Sleep
    CTRL_HI_Z       = 0x02, // Hi-Z
    CTRL_PLAY       = 0x03, // Play
   };

  enum DacMode : uint8_t {
    BTL  = 0, // Bridge tied load
    PBTL = 1, // Parallel load
  };

  enum MixerMode : uint8_t {
    STEREO = 0,
    STEREO_INVERSE,
    MONO,
    RIGHT,
    LEFT,
  };

  static const char* const MIXER_MODE_TEXT[] = {"STEREO", "STEREO_INVERSE", "MONO", "RIGHT", "LEFT"};

  struct Tas5805mConfiguration {
    uint8_t offset;
    uint8_t value;
  }__attribute__((packed));

  struct Tas5805mFault {
    uint8_t channel_fault{0};                  // individual faults extracted when publishing
    uint8_t global_fault{0};                   // individual faults extracted when publishing

    bool have_fault_except_clock_fault{false}; // internal use

    bool have_fault{false};                    // any fault found but does not include clock fault if it is excluded

    bool clock_fault{false};
    bool temperature_fault{false};
    bool temperature_warning{false};
  };

// Startup sequence codes
static const uint8_t TAS5805M_CFG_META_DELAY           = 254;

static const float TAS5805M_MIN_ANALOG_GAIN            = -15.5;
static const float TAS5805M_MAX_ANALOG_GAIN            = 0.0;

// set book and page registers
static const uint8_t TAS5805M_REG_PAGE_SET             = 0x00;
static const uint8_t TAS5805M_REG_BOOK_SET             = 0x7F;
static const uint8_t TAS5805M_REG_BOOK_CONTROL_PORT    = 0x00;
static const uint8_t TAS5805M_REG_PAGE_ZERO            = 0x00;

// tas5805m registers
static const uint8_t TAS5805M_DEVICE_CTRL_1            = 0x02;
static const uint8_t TAS5805M_DEVICE_CTRL_2            = 0x03;
static const uint8_t TAS5805M_FS_MON                   = 0x37;
static const uint8_t TAS5805M_BCK_MON                  = 0x38;
static const uint8_t TAS5805M_DIG_VOL_CTRL             = 0x4C;
static const uint8_t TAS5805M_ANA_CTRL                 = 0x53;
static const uint8_t TAS5805M_AGAIN                    = 0x54;
static const uint8_t TAS5805M_DSP_MISC                 = 0x66;
static const uint8_t TAS5805M_POWER_STATE              = 0x68;

// TAS5805M_REG_FAULT register values
static const uint8_t TAS5805M_CHAN_FAULT               = 0x70;
static const uint8_t TAS5805M_GLOBAL_FAULT1            = 0x71;
static const uint8_t TAS5805M_GLOBAL_FAULT2            = 0x72;
static const uint8_t TAS5805M_OT_WARNING               = 0x73;
static const uint8_t TAS5805M_FAULT_CLEAR              = 0x78;
static const uint8_t TAS5805M_ANALOG_FAULT_CLEAR       = 0x80;

// EQ registers
static const uint8_t  TAS5805M_CTRL_EQ_ON              = 0x00;
static const uint8_t  TAS5805M_CTRL_EQ_OFF             = 0x01;

// Level meter register

// Mixer registers
static const uint8_t TAS5805M_REG_BOOK_5               = 0x8C;
static const uint8_t TAS5805M_REG_BOOK_5_MIXER_PAGE    = 0x29;
static const uint8_t TAS5805M_REG_LEFT_TO_LEFT_GAIN    = 0x18;
static const uint8_t TAS5805M_REG_RIGHT_TO_LEFT_GAIN   = 0x1C;
static const uint8_t TAS5805M_REG_LEFT_TO_RIGHT_GAIN   = 0x20;
static const uint8_t TAS5805M_REG_RIGHT_TO_RIGHT_GAIN  = 0x24;
static const uint8_t TAS5805M_REG_BOOK_5_VOLUME_PAGE   = 0x2A;
static const uint8_t TAS5805M_REG_LEFT_VOLUME          = 0x24;
static const uint8_t TAS5805M_REG_RIGHT_VOLUME         = 0x28;
static const uint32_t TAS5805M_MIXER_VALUE_MUTE        = 0x00000000;
static const uint32_t TAS5805M_MIXER_VALUE_0DB         = 0x00008000;
//static const uint32_t TAS5805M_MIXER_VALUE_MINUS6DB    = 0xE7264000;
static const uint32_t TAS5805M_MIXER_VALUE_MINUS6DB    = 0x00004000;

}  // namespace esphome::tas5805m
