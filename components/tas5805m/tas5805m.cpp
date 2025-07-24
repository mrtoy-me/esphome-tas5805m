#include "tas5805m.h"
#include "tas5805m_minimal.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tas5805m {

static const char *const TAG                 = "tas5805m";
static const char *const WRITE_ERROR         = "Write error";

static const uint8_t TAS5805M_MUTE_CONTROL   = 0x08;  // LR Channel Mute

// maximum delay allowed in "tas5805m_minimal.h" used in configure_registers()
static const uint8_t ESPHOME_MAXIMUM_DELAY   = 5;     // milliseconds

// 'play_file' is initiated by YAML on_boot with priority 220.0f
// 'refresh_settings' is initiated by 'eq_gainband16000hz' with setup priority AFTER_CONNECTION = 100.0f
// delay before writing eq gains to ensure on boot sound has played and tas5805m has detected i2s clock
// 20 loop iterations = ~320ms
static const uint8_t DELAY_LOOPS             = 20;

// ms delay before starting fault updates so i2s is stabilise
static const uint16_t INITIAL_UPDATE_DELAY   = 10000;

void Tas5805mComponent::setup() {
  ESP_LOGCONFIG(TAG, "Running setup");
  if (this->enable_pin_ != nullptr) {
    // Set enable pin as OUTPUT and disable the enable pin
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(false);
    delay(10);
    this->enable_pin_->digital_write(true);
  }

  if (!configure_registers()) {
    this->error_code_ = CONFIGURATION_FAILED;
    this->mark_failed();
  }

  //rescale -103db to 24db digital volume range to register digital volume range 254 to 0
  this->tas5805m_state_.raw_volume_max = (uint8_t)((this->tas5805m_state_.volume_max - 24) * -2);
  this->tas5805m_state_.raw_volume_min = (uint8_t)((this->tas5805m_state_.volume_min - 24) * -2);

  #ifdef USE_BINARY_SENSOR
  // initialise all binary sensors
  if (this->have_fault_binary_sensor_ != nullptr) {
    this->have_fault_binary_sensor_->publish_state(false);
  }

  if (this->right_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->right_channel_over_current_fault_binary_sensor_->publish_state(false);
  }

  if (this->left_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->left_channel_over_current_fault_binary_sensor_->publish_state(false);
  }

  if (this->right_channel_dc_fault_binary_sensor_ != nullptr) {
    this->right_channel_dc_fault_binary_sensor_->publish_state(false);
  }

  if (this->left_channel_dc_fault_binary_sensor_ != nullptr) {
    this->left_channel_dc_fault_binary_sensor_->publish_state(false);
  }
  if (this->pvdd_under_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_under_voltage_fault_binary_sensor_->publish_state(false);
  }

  if (this->pvdd_over_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_over_voltage_fault_binary_sensor_->publish_state(false);
  }

  if (this->clock_fault_binary_sensor_ != nullptr) {
    this->clock_fault_binary_sensor_->publish_state(false);
  }

  if (this->bq_write_failed_fault_binary_sensor_ != nullptr) {
    this->bq_write_failed_fault_binary_sensor_->publish_state(false);
  }

  if (this->otp_crc_check_error_binary_sensor_ != nullptr) {
    this->otp_crc_check_error_binary_sensor_->publish_state(false);
  }

  if (this->over_temperature_shutdown_fault_binary_sensor_ != nullptr) {
    this->over_temperature_shutdown_fault_binary_sensor_->publish_state(false);
  }

  if (this->over_temperature_warning_binary_sensor_ != nullptr) {
    this->over_temperature_warning_binary_sensor_->publish_state(false);
  }
  #endif
}

bool Tas5805mComponent::configure_registers() {
  uint16_t i = 0;
  uint16_t counter = 0;
  uint16_t number_configurations = sizeof(tas5805m_registers) / sizeof(tas5805m_registers[0]);

  while (i < number_configurations) {
    switch (tas5805m_registers[i].offset) {
      case TAS5805M_CFG_META_DELAY:
        if (tas5805m_registers[i].value > ESPHOME_MAXIMUM_DELAY) return false;
        delay(tas5805m_registers[i].value);
        break;
      default:
        if (!this->tas5805m_write_byte(tas5805m_registers[i].offset, tas5805m_registers[i].value)) return false;
        counter++;
        break;
    }
    i++;
  }
  this->number_registers_configured_ = counter;

  // enable Tas5805m
  if(!this->set_deep_sleep_off()) return false;

  // only setup once here
  if (!this->set_dac_mode(this->tas5805m_state_.dac_mode)) return false;

  // setup of mixer mode deferred to 'loop' once 'refresh_settings' runs

  // only setup once here
  if (!this->set_analog_gain(this->tas5805m_state_.analog_gain)) return false;

  // start ready to Play
  if (!this->set_state(CTRL_PLAY)) return false;

  #ifdef USE_TAS5805M_EQ
    #ifdef USE_SPEAKER
    if (!this->set_eq(true)) return false;
    #else
    if (!this->set_eq(false)) return false;
    #endif
  #endif

  // initialise to now
  this->last_update_time_ = millis();

  return true;
}

void Tas5805mComponent::loop() {
  // when tas5805m has detected i2s clock, mixer mode and eq gains can be written

  // settings are only refreshed once
  // disable 'loop' once refresh settings are completed
  if (this->refresh_settings_complete_) {
    this->disable_loop(); // requires Esphome 2025.7.0
    return;
  }

  // refresh has not been triggered yet but once triggered it remains true
  if (!this->refresh_settings_triggered_) return;

  // once refresh settings is triggered, wait 'DELAY_LOOPS' before proceeding
  // to ensure on boot sound has played and tas5805m has detected i2s clock
  if (this->loop_counter_ < DELAY_LOOPS) {
    this->loop_counter_ = this->loop_counter_ + 1;
    return;
  }

  if (!this->mixer_mode_configured_) {
    if (!this->set_mixer_mode(this->tas5805m_state_.mixer_mode)) {
      //show warning but continue as if mixer mode was set ok
      ESP_LOGW(TAG, "Error refreshing Mixer Mode");
    }

    this->mixer_mode_configured_ = true;

    // if eq gains have not been configured in YAML
    // then once mixer mode is set, the refresh of settings is complete
    if (!this->using_eq_gains_) {
      this->refresh_settings_complete_ =  true;
      return;
    }

    // start re-writing eq gains next 'loop'
    return;
  }

  #ifdef USE_TAS5805M_EQ
  // re-write gains for all eq bands when triggered by boolean 'refresh_settings_triggered_'
  // write gains one band per 'loop' so component does not take too long in 'loop'

  // refresh_band_ is initially set to 0 in tas5805m.h
  // when finished reset variables just in case
  if (this->refresh_band_ == TAS5805M_EQ_BANDS) {
    this->refresh_settings_complete_ = true;
    this->refresh_band_ = 0;
    this->loop_counter_ = 0;
    return;
  }

  // re-write gains of current band and increment to next band ready for when loop next runs
  if (!this->set_eq_gain(this->refresh_band_, this->tas5805m_state_.eq_gain[this->refresh_band_])) {
    //show warning but continue as if eq gain was set ok
    ESP_LOGW(TAG, "Error refreshing EQ Gain");
  }
  this->refresh_band_ = this->refresh_band_ + 1;
  #endif

  return;
}

void Tas5805mComponent::update() {
  // wait INITIAL_UPDATE_DELAY before proceeding with updates
  // allows i2s clock to stabilise so initial clock faults are avoided
  if (!this->update_delay_finished_) {
    uint32_t current_time = millis();
    this->update_delay_finished_ = ((current_time - this->last_update_time_) > INITIAL_UPDATE_DELAY);

    if(!this->update_delay_finished_) {
      this->last_update_time_ = current_time;
      return;
    }

    // INITIAL_UPDATE_DELAY is finished, clear faults registers and zero fault counter
    // return and start checking faults from next update interval
    if (!this->initialise_fault_registers()) {
        ESP_LOGW(TAG, "Error initialising faults");
    }
    return;
  }

  // last update had faults, so now clear fault registers
  if (this->have_fault_) {
    if (!this->clear_fault_registers()) {
       ESP_LOGW(TAG, "Error clearing faults");
    }
  }

  if (!this->read_fault_registers()) {
    ESP_LOGW(TAG, "Error reading faults");
    this->have_fault_ = false;
    return;
  }

  this->have_fault_ = (this->tas5805m_state_.last_channel_fault || this->tas5805m_state_.last_global_fault ||
                       this->tas5805m_state_.last_over_temperature_fault || this->tas5805m_state_.last_over_temperature_warning );

  if (this->have_fault_) {
    ESP_LOGD(TAG, "Faults found: faults clear next update interval");
  }

  #ifdef USE_BINARY_SENSOR
  // update all binary sensors

  if (this->have_fault_binary_sensor_ != nullptr) {
    this->have_fault_binary_sensor_->publish_state(this->have_fault_);
  }

  if (this->right_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->right_channel_over_current_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_channel_fault & (1 << 0));
  }

  if (this->left_channel_over_current_fault_binary_sensor_ != nullptr) {
    this->left_channel_over_current_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_channel_fault & (1 << 1));
  }

  if (this->right_channel_dc_fault_binary_sensor_ != nullptr) {
    this->right_channel_dc_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_channel_fault & (1 << 2));
  }

  if (this->left_channel_dc_fault_binary_sensor_ != nullptr) {
    this->left_channel_dc_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_channel_fault & (1 << 3));
  }
  if (this->pvdd_under_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_under_voltage_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_global_fault & (1 << 0));
  }
  if (this->pvdd_over_voltage_fault_binary_sensor_ != nullptr) {
    this->pvdd_over_voltage_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_global_fault & (1 << 1));
  }

  if (this->clock_fault_binary_sensor_ != nullptr) {
    this->clock_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_global_fault & (1 << 2));
  }

  if (this->bq_write_failed_fault_binary_sensor_ != nullptr) {
    this->bq_write_failed_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_global_fault & (1 << 6));
  }

  if (this->otp_crc_check_error_binary_sensor_ != nullptr) {
    this->otp_crc_check_error_binary_sensor_->publish_state(this->tas5805m_state_.last_global_fault & (1 << 7));
  }

  if (this->over_temperature_shutdown_fault_binary_sensor_ != nullptr) {
    this->over_temperature_shutdown_fault_binary_sensor_->publish_state(this->tas5805m_state_.last_over_temperature_fault != 0);
  }

  if (this->over_temperature_warning_binary_sensor_ != nullptr) {
    this->over_temperature_warning_binary_sensor_->publish_state(this->tas5805m_state_.last_over_temperature_warning != 0);
  }
  #endif
}

void Tas5805mComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas5805m Audio Dac:");

  switch (this->error_code_) {
    case CONFIGURATION_FAILED:
      ESP_LOGE(TAG, "  Write configuration error= %i",this->i2c_error_);
      break;
    case WRITE_REGISTER_FAILED:
      ESP_LOGE(TAG, "  Write register error= %i",this->i2c_error_);
      break;
    case NONE:
      LOG_PIN("  Enable Pin: ", this->enable_pin_);
      LOG_I2C_DEVICE(this);
      ESP_LOGCONFIG(TAG,
              "  Registers configured: %i\n"
              "  DAC mode: %s\n"
              "  Mixer mode: %s\n"
              "  Analog Gain: %3.1fdB\n"
              "  Maximum Volume: %idB\n"
              "  Minimum Volume: %idB\n",
              this->number_registers_configured_, this->tas5805m_state_.dac_mode ? "PBTL" : "BTL",
              MIXER_MODE_TEXT[this->tas5805m_state_.mixer_mode], this->tas5805m_state_.analog_gain,
              this->tas5805m_state_.volume_max, this->tas5805m_state_.volume_min);
      LOG_UPDATE_INTERVAL(this);
      break;
  }

  #ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Any Faults:", this->have_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Right Channel Over Current:", this->right_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Left Channel Over Current:", this->left_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Right Channel DC Fault:", this->right_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Left Channel DC Fault:", this->left_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor PVDD Under Voltage:", this->pvdd_under_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor PVDD Over Voltage:", this->pvdd_over_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Clock Fault:", this->clock_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor BQ Write Failed:", this->bq_write_failed_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor OTP CRC Check Error:", this->otp_crc_check_error_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Over Temperature Shutdown:", this->over_temperature_shutdown_fault_binary_sensor_);
  LOG_BINARY_SENSOR("", "Tas5805m Binary Sensor Over Temperature Warning:", this->over_temperature_warning_binary_sensor_);
  #endif
}

bool Tas5805mComponent::set_volume(float volume) {
  float new_volume = clamp(volume, 0.0f, 1.0f);
  uint8_t raw_volume = remap<uint8_t, float>(new_volume, 0.0f, 1.0f,
                                                         this->tas5805m_state_.raw_volume_min,
                                                         this->tas5805m_state_.raw_volume_max);
  if (!this->set_digital_volume(raw_volume)) return false;

  int8_t dB = -(raw_volume / 2) + 24;
  ESP_LOGD(TAG, "Volume: %idB", dB);
  return true;
}

float Tas5805mComponent::volume() {
  uint8_t raw_volume;
  get_digital_volume(&raw_volume);

  return remap<float, uint8_t>(raw_volume, this->tas5805m_state_.raw_volume_min,
                                           this->tas5805m_state_.raw_volume_max,
                                           0.0f, 1.0f);
}

bool Tas5805mComponent::set_mute_off() {
  if (!this->is_muted_) return true;
  if (!this->tas5805m_write_byte(TAS5805M_DEVICE_CTRL_2, this->tas5805m_state_.control_state)) return false;
  this->is_muted_ = false;
  ESP_LOGD(TAG, "Mute Off");
  return true;
}

// set bit 3 MUTE in TAS5805M_DEVICE_CTRL_2 and retain current Control State
// ensures get_state = get_power_state
bool Tas5805mComponent::set_mute_on() {
  if (this->is_muted_) return true;
  if (!this->tas5805m_write_byte(TAS5805M_DEVICE_CTRL_2, this->tas5805m_state_.control_state + TAS5805M_MUTE_CONTROL)) return false;
  this->is_muted_ = true;
  ESP_LOGD(TAG, "Mute On");
  return true;
}

bool Tas5805mComponent::set_deep_sleep_on() {
  if (this->tas5805m_state_.control_state == CTRL_DEEP_SLEEP) return true; // already in deep sleep

  // preserve mute state
  uint8_t new_value = (this->is_muted_) ? (CTRL_DEEP_SLEEP + TAS5805M_MUTE_CONTROL) : CTRL_DEEP_SLEEP;
  if (!this->tas5805m_write_byte(TAS5805M_DEVICE_CTRL_2, new_value)) return false;

  this->tas5805m_state_.control_state = CTRL_DEEP_SLEEP;                   // set Control State to deep sleep
  ESP_LOGD(TAG, "Deep Sleep On");
  if (this->is_muted_) ESP_LOGD(TAG, "Mute On preserved");
  return true;
}

bool Tas5805mComponent::set_deep_sleep_off() {
  if (this->tas5805m_state_.control_state != CTRL_DEEP_SLEEP) return true; // already not in deep sleep
  // preserve mute state
  uint8_t new_value = (this->is_muted_) ? (CTRL_PLAY + TAS5805M_MUTE_CONTROL) : CTRL_PLAY;
  if (!this->tas5805m_write_byte(TAS5805M_DEVICE_CTRL_2, new_value)) return false;

  this->tas5805m_state_.control_state = CTRL_PLAY;                        // set Control State to play
  ESP_LOGD(TAG, "Deep Sleep Off");
  if (this->is_muted_) ESP_LOGD(TAG, "Mute On preserved");
  return true;
}

void Tas5805mComponent::enable_dac(bool enable) {
  enable ? this->set_deep_sleep_off() : this->set_deep_sleep_on();
}

bool Tas5805mComponent::get_state(ControlState* state) {
  *state = this->tas5805m_state_.control_state;
  return true;
}

bool Tas5805mComponent::set_state(ControlState state) {
  if (this->tas5805m_state_.control_state == state) return true;
  if (!this->tas5805m_write_byte(TAS5805M_DEVICE_CTRL_2, state)) return false;
  this->tas5805m_state_.control_state = state;
  ESP_LOGD(TAG, "State: 0x%02X", state);
  return true;
}

bool Tas5805mComponent::get_digital_volume(uint8_t* raw_volume) {
  uint8_t current = 254; // lowest raw volume
  if(!this->tas5805m_read_byte(TAS5805M_DIG_VOL_CTRL, &current)) return false;
  *raw_volume = current;
  return true;
}

// controls both left and right channel digital volume
// digital volume is 24 dB to -103 dB in -0.5 dB step
// 00000000: +24.0 dB
// 00000001: +23.5 dB
// 00101111: +0.5 dB
// 00110000: 0.0 dB
// 00110001: -0.5 dB
// 11111110: -103 dB
// 11111111: Mute
bool Tas5805mComponent::set_digital_volume(uint8_t raw_volume) {
  if (!this->tas5805m_write_byte(TAS5805M_DIG_VOL_CTRL, raw_volume)) return false;
  return true;
}

// Analog Gain Control , with 0.5dB one step
// lower 5 bits controls the analog gain.
// 00000: 0 dB (29.5V peak voltage)
// 00001: -0.5db
// 11111: -15.5 dB
// set analog gain in dB
bool Tas5805mComponent::set_analog_gain(float gain_db) {
  if ((gain_db < TAS5805M_MIN_ANALOG_GAIN) || (gain_db > TAS5805M_MAX_ANALOG_GAIN)) return false;

  uint8_t new_again = static_cast<uint8_t>(-gain_db * 2.0);

  uint8_t current_again;
  if (!this->tas5805m_read_byte(TAS5805M_AGAIN, &current_again)) return false;

  // keep top 3 reserved bits combine with bottom 5 analog gain bits
  new_again = (current_again & 0xE0) | new_again;
  if (!this->tas5805m_write_byte(TAS5805M_AGAIN, new_again)) return false;

  ESP_LOGD(TAG, "Analog Gain: %fdB", gain_db);
  return true;
}

bool Tas5805mComponent::get_analog_gain(uint8_t* raw_gain) {
  uint8_t current;
  if (!this->tas5805m_read_byte(TAS5805M_AGAIN, &current)) return false;
  // remove top 3 reserved bits
  *raw_gain = current & 0x1F;
  return true;
}

bool Tas5805mComponent::get_dac_mode(DacMode* mode) {
    uint8_t current_value;
    if (!this->tas5805m_read_byte(TAS5805M_DEVICE_CTRL_1, &current_value)) return false;
    if (current_value & (1 << 2)) {
        *mode = PBTL;
    } else {
        *mode = BTL;
    }
    this->tas5805m_state_.dac_mode = *mode;
    return true;
}

// only runs once from 'setup'
bool Tas5805mComponent::set_dac_mode(DacMode mode) {
  uint8_t current_value;
  if (!this->tas5805m_read_byte(TAS5805M_DEVICE_CTRL_1, &current_value)) return false;

  // Update bit 2 based on the mode
  if (mode == PBTL) {
      current_value |= (1 << 2);  // Set bit 2 to 1 (PBTL mode)
  } else {
      current_value &= ~(1 << 2); // Clear bit 2 to 0 (BTL mode)
  }
  if (!this->tas5805m_write_byte(TAS5805M_DEVICE_CTRL_1, current_value)) return false;

  // 'tas5805m_state_' global already has dac mode from YAML config
  // save anyway so 'set_dac_mode' could be used more generally
  this->tas5805m_state_.dac_mode = mode;
  ESP_LOGD(TAG, "DAC mode: %s", this->tas5805m_state_.dac_mode ? "PBTL" : "BTL");
  return true;
}

bool Tas5805mComponent::get_mixer_mode(MixerMode *mode) {
  *mode = this->tas5805m_state_.mixer_mode;
  return true;
}

// only runs once from 'loop'
// 'mixer_mode_configured_' used by 'loop' ensures it only runs once
bool Tas5805mComponent::set_mixer_mode(MixerMode mode) {
  uint32_t mixer_l_to_l, mixer_r_to_r, mixer_l_to_r, mixer_r_to_l;

  switch (mode) {
    case STEREO:
      mixer_l_to_l = TAS5805M_MIXER_VALUE_0DB;
      mixer_r_to_r = TAS5805M_MIXER_VALUE_0DB;
      mixer_l_to_r = TAS5805M_MIXER_VALUE_MUTE;
      mixer_r_to_l = TAS5805M_MIXER_VALUE_MUTE;
      break;

    case STEREO_INVERSE:
      mixer_l_to_l = TAS5805M_MIXER_VALUE_MUTE;
      mixer_r_to_r = TAS5805M_MIXER_VALUE_MUTE;
      mixer_l_to_r = TAS5805M_MIXER_VALUE_0DB;
      mixer_r_to_l = TAS5805M_MIXER_VALUE_0DB;
      break;

    case MONO:
      mixer_l_to_l = TAS5805M_MIXER_VALUE_MINUS6DB;
      mixer_r_to_r = TAS5805M_MIXER_VALUE_MINUS6DB;
      mixer_l_to_r = TAS5805M_MIXER_VALUE_MINUS6DB;
      mixer_r_to_l = TAS5805M_MIXER_VALUE_MINUS6DB;
      break;

    case LEFT:
      mixer_l_to_l = TAS5805M_MIXER_VALUE_0DB;
      mixer_r_to_r = TAS5805M_MIXER_VALUE_MUTE;
      mixer_l_to_r = TAS5805M_MIXER_VALUE_0DB;
      mixer_r_to_l = TAS5805M_MIXER_VALUE_MUTE;
      break;

    case RIGHT:
      mixer_l_to_l = TAS5805M_MIXER_VALUE_MUTE;
      mixer_r_to_r = TAS5805M_MIXER_VALUE_0DB;
      mixer_l_to_r = TAS5805M_MIXER_VALUE_MUTE;
      mixer_r_to_l = TAS5805M_MIXER_VALUE_0DB;
      break;

    default:
      ESP_LOGD(TAG, "Invalid Mixer Mode");
      return false;
  }

  if(!this->set_book_and_page(TAS5805M_REG_BOOK_5, TAS5805M_REG_BOOK_5_MIXER_PAGE)) {
    ESP_LOGE(TAG, "%s book-page @start set Mixer Mode", WRITE_ERROR);
    return false;
  }

  if (!this->tas5805m_write_bytes(TAS5805M_REG_LEFT_TO_LEFT_GAIN, reinterpret_cast<uint8_t *>(&mixer_l_to_l), 4)) {
    ESP_LOGE(TAG, "%s Left-Left Mixer Gain", WRITE_ERROR);
    return false;
  }

  if (!this->tas5805m_write_bytes(TAS5805M_REG_RIGHT_TO_RIGHT_GAIN, reinterpret_cast<uint8_t *>(&mixer_r_to_r), 4)) {
    ESP_LOGE(TAG, "%s Right-Right Mixer Gain", WRITE_ERROR);
    return false;
  }

  if (!this->tas5805m_write_bytes(TAS5805M_REG_LEFT_TO_RIGHT_GAIN, reinterpret_cast<uint8_t *>(&mixer_l_to_r), 4)) {
    ESP_LOGE(TAG, "%s Left-Right Mixer Gain", WRITE_ERROR);
    return false;
  }

  if (!this->tas5805m_write_bytes(TAS5805M_REG_RIGHT_TO_LEFT_GAIN, reinterpret_cast<uint8_t *>(&mixer_r_to_l), 4)) {
    ESP_LOGE(TAG, "%s Right-Left Mixer Gain", WRITE_ERROR);
    return false;
  }

  if (!this->set_book_and_page(TAS5805M_REG_BOOK_CONTROL_PORT, TAS5805M_REG_PAGE_ZERO)) {
    ESP_LOGE(TAG, "%s book and page @complete set Mixer Mode", WRITE_ERROR);
    return false;
  }

  // 'tas5805m_state_' global already has mixer mode from YAML config
  // save anyway so 'set_mixer_mode' could be used more generally
  this->tas5805m_state_.mixer_mode = mode;
  ESP_LOGD(TAG, "Mixer mode: %s", MIXER_MODE_TEXT[this->tas5805m_state_.mixer_mode]);
  return true;
}

bool Tas5805mComponent::set_eq_on() {
  #ifdef USE_TAS5805M_EQ
  if (this->tas5805m_state_.eq_enabled) return true;
  if (!this->tas5805m_write_byte(TAS5805M_DSP_MISC, TAS5805M_CTRL_EQ_ON)) return false;
  this->tas5805m_state_.eq_enabled = true;
  ESP_LOGD(TAG, "EQ control On");
  #endif
  return true;
}

bool Tas5805mComponent::set_eq_off() {
  #ifdef USE_TAS5805M_EQ
  if (!this->tas5805m_state_.eq_enabled) return true;
  if (!this->tas5805m_write_byte(TAS5805M_DSP_MISC, TAS5805M_CTRL_EQ_OFF)) return false;
  this->tas5805m_state_.eq_enabled = false;
  ESP_LOGD(TAG, "EQ control Off");
  #endif
  return true;
}

bool Tas5805mComponent::set_eq(bool enable) {
  #ifdef USE_TAS5805M_EQ
  enable ? set_eq_on() : set_eq_off();
  #endif
  return true;
}

#ifdef USE_TAS5805M_EQ
bool Tas5805mComponent::get_eq(bool* enabled) {
  uint8_t current_value;
  if (!this->tas5805m_read_byte(TAS5805M_DSP_MISC, &current_value)) return false;
  *enabled = !(current_value & 0x01);
  this->tas5805m_state_.eq_enabled = *enabled;
  return true;
}
#endif

#ifdef USE_TAS5805M_EQ
bool Tas5805mComponent::set_eq_gain(uint8_t band, int8_t gain) {
  if (band < 0 || band >= TAS5805M_EQ_BANDS) {
    ESP_LOGE(TAG, "Invalid EQ Band: %d", band);
    return false;
  }
  if (gain < TAS5805M_EQ_MIN_DB || gain > TAS5805M_EQ_MAX_DB) {
    ESP_LOGE(TAG, "Invalid EQ Gain: %d  EQ Band: %d unchanged", gain, band);
    return false;
  }

  if (!this->refresh_settings_triggered_) {
    this->tas5805m_state_.eq_gain[band] = gain;
    ESP_LOGD(TAG, "EQ Band: %d set new Gain: %ddB later", band, gain);
    return true;
  }

  // run when 'refresh_settings_triggered_' is true
  uint8_t current_page = 0;
  bool ok = true;
  ESP_LOGD(TAG, "EQ Band %d (%dHz) set Gain %ddB", band, tas5805m_eq_bands[band], gain);

  int x = gain + TAS5805M_EQ_MAX_DB;
  uint16_t y = band * TAS5805M_EQ_KOEF_PER_BAND * TAS5805M_EQ_REG_PER_KOEF;

  for (int16_t i = 0; i < TAS5805M_EQ_KOEF_PER_BAND * TAS5805M_EQ_REG_PER_KOEF; i++) {
      const RegisterSequenceEq* reg_value = &tas5805m_eq_registers[x][y + i];
      if (reg_value == NULL) {
          ESP_LOGW(TAG, "NULL pointer @row[%d]", y + i);
          continue;
      }

      if (reg_value->page != current_page) {
          current_page = reg_value->page;
          if(!this->set_book_and_page(TAS5805M_REG_BOOK_EQ, reg_value->page)) {
            ESP_LOGE(TAG, "%s EQ Gain Band %d (%d Hz)", WRITE_ERROR, band, tas5805m_eq_bands[band]);
            return false;
          }
      }

      ESP_LOGV(TAG, "Write EQ Gain @Band %d at 0x%02X with 0x%02X", i, reg_value->offset, reg_value->value);
      ok = tas5805m_write_byte(reg_value->offset, reg_value->value);
      if (!ok) {
        ESP_LOGE(TAG, "%s EQ Gain @register 0x%02X", WRITE_ERROR, reg_value->offset);
      }
  }

  this->tas5805m_state_.eq_gain[band] = gain;
  return this->set_book_and_page(TAS5805M_REG_BOOK_CONTROL_PORT, TAS5805M_REG_PAGE_ZERO);
}
#endif

bool Tas5805mComponent::use_eq_gain_refresh() {
  return (this->auto_refresh_ == BY_GAIN);
}

bool Tas5805mComponent::use_eq_switch_refresh() {
  return (this->auto_refresh_ == BY_SWITCH);
}

void Tas5805mComponent::refresh_settings() {
  // if 'this->refresh_settings_triggered_' is true then refresh of settings
  // has been initiated so no action is required
  if (this->refresh_settings_triggered_) return;

  // triggers 'loop' to configure mixer mode and eq gains
  // allows 'set_eq_gains' to now write eq gains rather than deferring for later setup
  // 'refresh_settings_triggered_' remains true once refresh of settings has completed
  // which allows 'set_eq_gains' continue to write eq gains
  this->refresh_settings_triggered_ = true;

  #ifdef USE_TAS5805M_EQ
  ESP_LOGD(TAG, "Refresh Settings triggered: EQ %s", this->tas5805m_state_.eq_enabled ? "Enabled" : "Disabled");
  #endif
  return;
}

bool Tas5805mComponent::initialise_fault_registers() {
  if (!tas5805m_write_byte(TAS5805M_FAULT_CLEAR, TAS5805M_ANALOG_FAULT_CLEAR)) return false;
  this->tas5805m_state_.times_faults_cleared = 0;
  return true;
}

bool Tas5805mComponent::clear_fault_registers() {
  if (!tas5805m_write_byte(TAS5805M_FAULT_CLEAR, TAS5805M_ANALOG_FAULT_CLEAR)) return false;
  this->tas5805m_state_.times_faults_cleared++;
  return true;
}

bool Tas5805mComponent::read_fault_registers() {
  if (!this->tas5805m_read_byte(TAS5805M_CHAN_FAULT, &this->tas5805m_state_.last_channel_fault)) return false;
  if (!this->tas5805m_read_byte(TAS5805M_GLOBAL_FAULT1, &this->tas5805m_state_.last_global_fault)) return false;
  if (!this->tas5805m_read_byte(TAS5805M_GLOBAL_FAULT2, &this->tas5805m_state_.last_over_temperature_fault)) return false;
  if (!this->tas5805m_read_byte(TAS5805M_OT_WARNING, &this->tas5805m_state_.last_over_temperature_warning)) return false;
  return true;
}

uint32_t Tas5805mComponent::times_faults_cleared() {
  return this->tas5805m_state_.times_faults_cleared;
}

uint8_t Tas5805mComponent::last_channel_fault() {
  return this->tas5805m_state_.last_channel_fault;
}

uint8_t Tas5805mComponent::last_global_fault() {
  return this->tas5805m_state_.last_global_fault;
}

bool Tas5805mComponent::set_book_and_page(uint8_t book, uint8_t page) {
  if (!this->tas5805m_write_byte(TAS5805M_REG_PAGE_SET, TAS5805M_REG_PAGE_ZERO)) {
    ESP_LOGE(TAG, "%s page zero", WRITE_ERROR);
    return false;
  }
  if (!this->tas5805m_write_byte(TAS5805M_REG_BOOK_SET, book)) {
    ESP_LOGE(TAG, "%s book: 0x%02X", WRITE_ERROR, book);
    return false;
  }
  if (!this->tas5805m_write_byte(TAS5805M_REG_PAGE_SET, page)) {
    ESP_LOGE(TAG, "%s page: 0x%02X", WRITE_ERROR, page);
    return false;
  }
  return true;
}

bool Tas5805mComponent::tas5805m_read_byte(uint8_t a_register, uint8_t* data) {
  i2c::ErrorCode error_code;
  error_code = this->write(&a_register, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read error: %i @first write", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  error_code = this->read_register(a_register, data, 1, true);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read error: %i", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

bool Tas5805mComponent::tas5805m_write_byte(uint8_t a_register, uint8_t data) {
    i2c::ErrorCode error_code = this->write_register(a_register, &data, 1, true);
    if (error_code != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "%s: %i", WRITE_ERROR, error_code);
      this->i2c_error_ = (uint8_t)error_code;
      return false;
    }
    return true;
}

bool Tas5805mComponent::tas5805m_write_bytes(uint8_t a_register, uint8_t* data, uint8_t len) {
  i2c::ErrorCode error_code = this->write_register(a_register, data, len, true);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "%s: %i", WRITE_ERROR, error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

}  // namespace tas5805m
}  // namespace esphome
