#include "tas5805m.h"
#include "tas5805m_minimal.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tas5805m {

static const char *const TAG               = "tas5805m";
static const char *const ERROR             = "Error ";
static const char *const MIXER_MODE        = "Mixer Mode";
static const char *const EQ_BAND           = "EQ Band ";

static const uint8_t TAS5805M_MUTE_CONTROL = 0x08;  // LR Channel Mute
static const uint8_t REMOVE_CLOCK_FAULT    = 0xFB;  // used to zero clock fault bit of global_fault1 register

// maximum delay allowed in "tas5805m_minimal.h" used in configure_registers()
static const uint8_t ESPHOME_MAXIMUM_DELAY = 5;     // milliseconds

// initial delay in 'loop' before writing eq gains to ensure on boot sound has
// played and tas5805m has detected i2s clock
static const uint8_t DELAY_LOOPS           = 20;    // 20 loop iterations = ~320ms

// initial ms delay before starting fault updates
static const uint16_t INITIAL_UPDATE_DELAY = 4000;

void Tas5805mComponent::setup() {
  ESP_LOGCONFIG(TAG, "Running setup");
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(false);
    delay(10);
    this->enable_pin_->digital_write(true);
  }

  if (!configure_registers_()) {
    this->error_code_ = CONFIGURATION_FAILED;
    this->mark_failed();
  }

  // rescale -103db to 24db digital volume range to register digital volume range 254 to 0
  this->tas5805m_state_.raw_volume_max = (uint8_t)((this->tas5805m_state_.volume_max - 24) * -2);
  this->tas5805m_state_.raw_volume_min = (uint8_t)((this->tas5805m_state_.volume_min - 24) * -2);

  // initialise so first update publishes all binary sensors
  this->tas5805m_state_.faults.channel_fault = 0xFF;
  this->tas5805m_state_.faults.global_fault = 0xFF;
  this->tas5805m_state_.faults.temperature_fault = 0xFF;
  this->tas5805m_state_.faults.temperature_warning = 0xFF;
}

bool Tas5805mComponent::configure_registers_() {
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
        if (!this->tas5805m_write_byte_(tas5805m_registers[i].offset, tas5805m_registers[i].value)) return false;
        counter++;
        break;
    }
    i++;
  }
  this->number_registers_configured_ = counter;

  // enable Tas5805m
  if(!this->set_deep_sleep_off_()) return false;

  // only setup once here
  if (!this->set_dac_mode_(this->tas5805m_state_.dac_mode)) return false;

  // setup of mixer mode deferred to 'loop' once 'refresh_settings' runs

  if (!this->set_analog_gain_(this->tas5805m_state_.analog_gain)) return false;

  if (!this->set_state_(CTRL_PLAY)) return false;

  #ifdef USE_TAS5805M_EQ
    #ifdef USE_SPEAKER
    if (!this->enable_eq(true)) return false;
    #else
    if (!this->enable_eq(false)) return false;
    #endif
  #endif

  // initialise to now
  this->start_time_ = millis();
  return true;
}

void Tas5805mComponent::loop() {
  // 'play_file' is initiated by YAML on_boot with priority 220.0f
  // 'refresh_settings' is set by 'eq_gainband16000hz' or 'enable_eq_switch' (defined by YAML)
  // both have setup priority AFTER_CONNECTION = 100.0f
  // once tas5805m has detected i2s clock then mixer mode and eq gains settings
  // can be written within 'loop'

  // settings are only refreshed once
  // disable 'loop' once all eq gains have been written
  if (this->refresh_settings_complete_) {
    this->disable_loop(); // requires Esphome 2025.7.0
    return;
  }

  // refresh of settings has not been triggered yet
  if (!this->refresh_settings_triggered_) return;

  // once refresh settings is triggered then wait 'DELAY_LOOPS' before proceeding
  // to ensure on boot sound has played and tas5805m has detected i2s clock
  if (this->loop_counter_ < DELAY_LOOPS) {
    this->loop_counter_++;
    return;
  }

  if (!this->mixer_mode_configured_) {
    if (!this->set_mixer_mode_(this->tas5805m_state_.mixer_mode)) {
      // show warning but continue as if mixer mode was set ok
      ESP_LOGW(TAG, "%ssetting mixer mode: %s", ERROR, MIXER_MODE);
    }

    this->mixer_mode_configured_ = true;

    // if eq gains have not been configured in YAML
    // then once mixer mode is set, the refresh of settings is complete
    if (!this->using_eq_gains_) {
      this->refresh_settings_complete_ =  true;
      return;
    }

    // start writing eq gains next 'loop'
    return;
  }

  #ifdef USE_TAS5805M_EQ
  // write gains for all eq bands when triggered by boolean 'refresh_settings_triggered_'
  // write gains one eq band per 'loop' so component does not take too long in 'loop'

  if (this->refresh_band_ == NUMBER_EQ_BANDS) {
    // finished writing all gains
    this->refresh_settings_complete_ = true;
    this->refresh_band_ = 0;
    this->loop_counter_ = 0;
    return;
  }

  // write gains of current band and increment to next band ready for when loop next runs
  if (!this->set_eq_gain(this->refresh_band_, this->tas5805m_state_.eq_gain[this->refresh_band_])) {
    // show warning but continue as if eq gain was set ok
    ESP_LOGW(TAG, "%ssetting EQ Band %d Gain", ERROR, this->refresh_band_);
  }
  this->refresh_band_++;
  #endif

  return;
}

void Tas5805mComponent::update() {
  // initial delay before proceeding with updates
  if (!this->update_delay_finished_) {
    uint32_t current_time = millis();
    this->update_delay_finished_ = ((current_time - this->start_time_) > INITIAL_UPDATE_DELAY);

    if(!this->update_delay_finished_) return;

    // finished delay so clear faults
    if (!tas5805m_write_byte_(TAS5805M_FAULT_CLEAR, TAS5805M_ANALOG_FAULT_CLEAR)) {
      ESP_LOGW(TAG, "%sinitialising faults", ERROR);
    }
    // read and process faults from next update interval
    //return;
  }

  if (!this->read_fault_registers_()) {
    ESP_LOGW(TAG, "%sreading faults", ERROR);
    return;
  }

  bool faults_excluding_clock_fault = (this->tas5805m_state_.faults.channel_fault ||
                                       this->tas5805m_state_.faults.global_fault ||
                                       this->tas5805m_state_.faults.temperature_fault ||
                                       this->tas5805m_state_.faults.temperature_warning );

  this->have_fault_ = (this->tas5805m_state_.faults.clock_fault || faults_excluding_clock_fault);

  // skip sensor update and clear faults if there is no possibility of state change in any faults
  if ((!this->had_fault_last_update_) && (!this->have_fault_)) return;

  #ifdef USE_BINARY_SENSOR
  if (this->have_fault_binary_sensor_ != nullptr) {
    if (this->exclude_fault_ )
      this->have_fault_binary_sensor_->publish_state(faults_excluding_clock_fault);
    else
      this->have_fault_binary_sensor_->publish_state(this->have_fault_);
  }

  if (this->is_new_channel_fault_) {
    if (this->right_channel_over_current_fault_binary_sensor_ != nullptr) {
      this->right_channel_over_current_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.channel_fault & (1 << 0));
    }

    if (this->left_channel_over_current_fault_binary_sensor_ != nullptr) {
      this->left_channel_over_current_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.channel_fault & (1 << 1));
    }

    if (this->right_channel_dc_fault_binary_sensor_ != nullptr) {
      this->right_channel_dc_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.channel_fault & (1 << 2));
    }

    if (this->left_channel_dc_fault_binary_sensor_ != nullptr) {
      this->left_channel_dc_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.channel_fault & (1 << 3));
    }
  }

  if (this->is_new_global_fault_) {
    if (this->pvdd_under_voltage_fault_binary_sensor_ != nullptr) {
      this->pvdd_under_voltage_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.global_fault & (1 << 0));
    }

    if (this->pvdd_over_voltage_fault_binary_sensor_ != nullptr) {
      this->pvdd_over_voltage_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.global_fault & (1 << 1));
    }

    if (this->bq_write_failed_fault_binary_sensor_ != nullptr) {
      this->bq_write_failed_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.global_fault & (1 << 6));
    }

    if (this->otp_crc_check_error_binary_sensor_ != nullptr) {
      this->otp_crc_check_error_binary_sensor_->publish_state(this->tas5805m_state_.faults.global_fault & (1 << 7));
    }
  }

  if (this->clock_fault_binary_sensor_ != nullptr) {
    this->clock_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.clock_fault);
  }

  if (this->over_temperature_shutdown_fault_binary_sensor_ != nullptr) {
    this->over_temperature_shutdown_fault_binary_sensor_->publish_state(this->tas5805m_state_.faults.temperature_fault);
  }

  if (this->over_temperature_warning_binary_sensor_ != nullptr) {
    this->over_temperature_warning_binary_sensor_->publish_state(this->tas5805m_state_.faults.temperature_warning);
  }
  #endif

  // if there is a fault then clear any faults
  if (this->have_fault_) {
    this->had_fault_last_update_ = true;
    // if (!this->clear_fault_registers_()) {
    //   ESP_LOGW(TAG, "%sclearing faults", ERROR);
    // }
  }
  else {
    this->had_fault_last_update_ = false;
  }
}

void Tas5805mComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas5805m Audio Dac:");

  switch (this->error_code_) {
    case CONFIGURATION_FAILED:
      ESP_LOGE(TAG, "  %s setting up Tas5805m: %i", ERROR, this->i2c_error_);
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
  ESP_LOGCONFIG(TAG, "Tas5805m Binary Sensors:");
  LOG_BINARY_SENSOR("  ", "Any Faults", this->have_fault_binary_sensor_);
  if (this->exclude_fault_) {
    ESP_LOGCONFIG(TAG, "    Exclude: CLOCK FAULTS");
  }
  LOG_BINARY_SENSOR("  ", "Right Channel Over Current", this->right_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel Over Current", this->left_channel_over_current_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Right Channel DC Fault", this->right_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Left Channel DC Fault", this->left_channel_dc_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "PVDD Under Voltage", this->pvdd_under_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "PVDD Over Voltage", this->pvdd_over_voltage_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Clock Fault", this->clock_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "BQ Write Failed", this->bq_write_failed_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "OTP CRC Check Error", this->otp_crc_check_error_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Over Temperature Shutdown", this->over_temperature_shutdown_fault_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Over Temperature Warning", this->over_temperature_warning_binary_sensor_);
  #endif
}


// public

// used by 'enable_dac_switch'
void Tas5805mComponent::enable_dac(bool enable) {
  enable ? this->set_deep_sleep_off_() : this->set_deep_sleep_on_();
}

// used by 'enable_eq_switch'
bool Tas5805mComponent::enable_eq(bool enable) {
  #ifdef USE_TAS5805M_EQ
  enable ? set_eq_on_() : set_eq_off_();
  #endif
  return true;
}

// used by eq gain numbers
#ifdef USE_TAS5805M_EQ
bool Tas5805mComponent::set_eq_gain(uint8_t band, int8_t gain) {
  if (band < 0 || band >= NUMBER_EQ_BANDS) {
    ESP_LOGE(TAG, "Invalid %s%d", EQ_BAND, band);
    return false;
  }
  if (gain < TAS5805M_EQ_MIN_DB || gain > TAS5805M_EQ_MAX_DB) {
    ESP_LOGE(TAG, "Invalid %s%d Gain: %ddB", EQ_BAND, band, gain);
    return false;
  }

  // EQ Gains initially set by tas5805 number component setups
  if (!this->refresh_settings_triggered_) {
    this->tas5805m_state_.eq_gain[band] = gain;
    //ESP_LOGD(TAG, "Save %s%d Gain: %ddB", EQ_BAND, band, gain);
    return true;
  }

  // runs when 'refresh_settings_triggered_' is true

  ESP_LOGD(TAG, "Set %s%d Gain: %ddB", EQ_BAND, band, gain);

  uint8_t x = (gain + TAS5805M_EQ_MAX_DB);

  const RegisterSequenceEq* reg_value = &TAS5805M_EQ_REGISTERS[x][band];
  if (reg_value == NULL) {
    ESP_LOGE(TAG, "%sNULL@eq_registers[%d][%d]",ERROR, x, band);
    return false;
  }
  ESP_LOGVV(TAG, "Write %s%d @ page 0x%02X", EQ_BAND, band, reg_value->page);
  if(!this->set_book_and_page_(TAS5805M_REG_BOOK_EQ, reg_value->page)) {
    ESP_LOGE(TAG, "%s%s%d @ page 0x%02X", ERROR, EQ_BAND, band, reg_value->page);
    return false;
  }

  ESP_LOGVV(TAG, "Write %s%d Gain: offset 0x%02X for %d bytes", EQ_BAND, band, reg_value->offset1, reg_value->bytes_in_block1);
  if(!tas5805m_write_bytes_(reg_value->offset1, const_cast<uint8_t *>(reg_value->value), reg_value->bytes_in_block1)) {
    ESP_LOGE(TAG, "%s%s%d Gain: offset 0x%02X for %d bytes", ERROR, EQ_BAND, band, reg_value->offset1, reg_value->bytes_in_block1);
  }

  uint8_t bytes_in_block2 = COEFFICENTS_PER_EQ_BAND - reg_value->bytes_in_block1;
  if (bytes_in_block2 != 0) {
    uint8_t next_page = reg_value->page + 1;
    ESP_LOGVV(TAG, "Write %s%d @ page 0x%02X", EQ_BAND, band, next_page);
    if(!this->set_book_and_page_(TAS5805M_REG_BOOK_EQ, next_page)) {
      ESP_LOGE(TAG, "%s%s%d @ page 0x%02X", ERROR, EQ_BAND, band, next_page);
      return false;
    }
    ESP_LOGVV(TAG, "Write %s%d Gain: offset 0x%02X for %d bytes", EQ_BAND, band, reg_value->offset2, bytes_in_block2);
    if(!tas5805m_write_bytes_(reg_value->offset2, const_cast<uint8_t *>(reg_value->value + reg_value->bytes_in_block1), bytes_in_block2)) {
      ESP_LOGE(TAG, "%s%s%d Gain: offset 0x%02X for %d bytes", ERROR, EQ_BAND, band, reg_value->offset2, bytes_in_block2);
      return false;
    }
  }

  return this->set_book_and_page_(TAS5805M_REG_BOOK_CONTROL_PORT, TAS5805M_REG_PAGE_ZERO);
}
#endif

bool Tas5805mComponent::set_mute_off() {
  if (!this->is_muted_) return true;
  if (!this->tas5805m_write_byte_(TAS5805M_DEVICE_CTRL_2, this->tas5805m_state_.control_state)) return false;
  this->is_muted_ = false;
  ESP_LOGD(TAG, "Mute Off");
  return true;
}

// set bit 3 MUTE in TAS5805M_DEVICE_CTRL_2 and retain current Control State
// ensures get_state = get_power_state
bool Tas5805mComponent::set_mute_on() {
  if (this->is_muted_) return true;
  if (!this->tas5805m_write_byte_(TAS5805M_DEVICE_CTRL_2, this->tas5805m_state_.control_state + TAS5805M_MUTE_CONTROL)) return false;
  this->is_muted_ = true;
  ESP_LOGD(TAG, "Mute On");
  return true;
}

// used by 'enable_eq_switch' and 'eq_gain_band16000hz'
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
  ESP_LOGD(TAG, "Refresh triggered: EQ %s", this->tas5805m_state_.eq_enabled ? "Enabled" : "Disabled");
  #endif
  return;
}

// used by fault sensor
uint32_t Tas5805mComponent::times_faults_cleared() {
  return this->tas5805m_state_.times_faults_cleared;
}

// used by 'eq_gain_band16000hz' to determine if it should 'refresh_settings()'
bool Tas5805mComponent::use_eq_gain_refresh() {
  return (this->auto_refresh_ == BY_GAIN);
}

// used by 'enable_eq_switch' to determine if it should 'refresh_settings()'
bool Tas5805mComponent::use_eq_switch_refresh() {
  return (this->auto_refresh_ == BY_SWITCH);
}

float Tas5805mComponent::volume() {
  uint8_t raw_volume;
  get_digital_volume_(&raw_volume);

  return remap<float, uint8_t>(raw_volume, this->tas5805m_state_.raw_volume_min,
                                           this->tas5805m_state_.raw_volume_max,
                                           0.0f, 1.0f);
}

bool Tas5805mComponent::set_volume(float volume) {
  float new_volume = clamp(volume, 0.0f, 1.0f);
  uint8_t raw_volume = remap<uint8_t, float>(new_volume, 0.0f, 1.0f,
                                                         this->tas5805m_state_.raw_volume_min,
                                                         this->tas5805m_state_.raw_volume_max);
  if (!this->set_digital_volume_(raw_volume)) return false;

  int8_t dB = -(raw_volume / 2) + 24;
  ESP_LOGD(TAG, "Volume: %idB", dB);
  return true;
}


// protected

bool Tas5805mComponent::get_analog_gain_(uint8_t* raw_gain) {
  uint8_t current;
  if (!this->tas5805m_read_byte_(TAS5805M_AGAIN, &current)) return false;
  // remove top 3 reserved bits
  *raw_gain = current & 0x1F;
  return true;
}

// Analog Gain Control , with 0.5dB one step
// lower 5 bits controls the analog gain.
// 00000: 0 dB (29.5V peak voltage)
// 00001: -0.5db
// 11111: -15.5 dB
// set analog gain in dB
bool Tas5805mComponent::set_analog_gain_(float gain_db) {
  if ((gain_db < TAS5805M_MIN_ANALOG_GAIN) || (gain_db > TAS5805M_MAX_ANALOG_GAIN)) return false;

  uint8_t new_again = static_cast<uint8_t>(-gain_db * 2.0);

  uint8_t current_again;
  if (!this->tas5805m_read_byte_(TAS5805M_AGAIN, &current_again)) return false;

  // keep top 3 reserved bits combine with bottom 5 analog gain bits
  new_again = (current_again & 0xE0) | new_again;
  if (!this->tas5805m_write_byte_(TAS5805M_AGAIN, new_again)) return false;

  ESP_LOGD(TAG, "Analog Gain: %fdB", gain_db);
  return true;
}

bool Tas5805mComponent::get_dac_mode_(DacMode* mode) {
    uint8_t current_value;
    if (!this->tas5805m_read_byte_(TAS5805M_DEVICE_CTRL_1, &current_value)) return false;
    if (current_value & (1 << 2)) {
        *mode = PBTL;
    } else {
        *mode = BTL;
    }
    this->tas5805m_state_.dac_mode = *mode;
    return true;
}

// only runs once from 'setup'
bool Tas5805mComponent::set_dac_mode_(DacMode mode) {
  uint8_t current_value;
  if (!this->tas5805m_read_byte_(TAS5805M_DEVICE_CTRL_1, &current_value)) return false;

  // Update bit 2 based on the mode
  if (mode == PBTL) {
      current_value |= (1 << 2);  // Set bit 2 to 1 (PBTL mode)
  } else {
      current_value &= ~(1 << 2); // Clear bit 2 to 0 (BTL mode)
  }
  if (!this->tas5805m_write_byte_(TAS5805M_DEVICE_CTRL_1, current_value)) return false;

  // 'tas5805m_state_' global already has dac mode from YAML config
  // save anyway so 'set_dac_mode' could be used more generally
  this->tas5805m_state_.dac_mode = mode;
  ESP_LOGD(TAG, "DAC mode: %s", this->tas5805m_state_.dac_mode ? "PBTL" : "BTL");
  return true;
}

bool Tas5805mComponent::set_deep_sleep_off_() {
  if (this->tas5805m_state_.control_state != CTRL_DEEP_SLEEP) return true; // already not in deep sleep
  // preserve mute state
  uint8_t new_value = (this->is_muted_) ? (CTRL_PLAY + TAS5805M_MUTE_CONTROL) : CTRL_PLAY;
  if (!this->tas5805m_write_byte_(TAS5805M_DEVICE_CTRL_2, new_value)) return false;

  this->tas5805m_state_.control_state = CTRL_PLAY;                        // set Control State to play
  ESP_LOGD(TAG, "Deep Sleep Off");
  if (this->is_muted_) ESP_LOGD(TAG, "Mute On preserved");
  return true;
}

bool Tas5805mComponent::set_deep_sleep_on_() {
  if (this->tas5805m_state_.control_state == CTRL_DEEP_SLEEP) return true; // already in deep sleep

  // preserve mute state
  uint8_t new_value = (this->is_muted_) ? (CTRL_DEEP_SLEEP + TAS5805M_MUTE_CONTROL) : CTRL_DEEP_SLEEP;
  if (!this->tas5805m_write_byte_(TAS5805M_DEVICE_CTRL_2, new_value)) return false;

  this->tas5805m_state_.control_state = CTRL_DEEP_SLEEP;                   // set Control State to deep sleep
  ESP_LOGD(TAG, "Deep Sleep On");
  if (this->is_muted_) ESP_LOGD(TAG, "Mute On preserved");
  return true;
}

bool Tas5805mComponent::get_digital_volume_(uint8_t* raw_volume) {
  uint8_t current = 254; // lowest raw volume
  if(!this->tas5805m_read_byte_(TAS5805M_DIG_VOL_CTRL, &current)) return false;
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
bool Tas5805mComponent::set_digital_volume_(uint8_t raw_volume) {
  if (!this->tas5805m_write_byte_(TAS5805M_DIG_VOL_CTRL, raw_volume)) return false;
  return true;
}

#ifdef USE_TAS5805M_EQ
bool Tas5805mComponent::get_eq_(bool* enabled) {
  uint8_t current_value;
  if (!this->tas5805m_read_byte_(TAS5805M_DSP_MISC, &current_value)) return false;
  *enabled = !(current_value & 0x01);
  this->tas5805m_state_.eq_enabled = *enabled;
  return true;
}
#endif

bool Tas5805mComponent::set_eq_off_() {
  #ifdef USE_TAS5805M_EQ
  if (!this->tas5805m_state_.eq_enabled) return true;
  if (!this->tas5805m_write_byte_(TAS5805M_DSP_MISC, TAS5805M_CTRL_EQ_OFF)) return false;
  this->tas5805m_state_.eq_enabled = false;
  ESP_LOGD(TAG, "EQ control Off");
  #endif
  return true;
}

bool Tas5805mComponent::set_eq_on_() {
  #ifdef USE_TAS5805M_EQ
  if (this->tas5805m_state_.eq_enabled) return true;
  if (!this->tas5805m_write_byte_(TAS5805M_DSP_MISC, TAS5805M_CTRL_EQ_ON)) return false;
  this->tas5805m_state_.eq_enabled = true;
  ESP_LOGD(TAG, "EQ control On");
  #endif
  return true;
}

bool Tas5805mComponent::get_mixer_mode_(MixerMode *mode) {
  *mode = this->tas5805m_state_.mixer_mode;
  return true;
}

// only runs once from 'loop'
// 'mixer_mode_configured_' used by 'loop' ensures it only runs once
bool Tas5805mComponent::set_mixer_mode_(MixerMode mode) {
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
      ESP_LOGD(TAG, "Invalid %s", MIXER_MODE);
      return false;
  }

  if(!this->set_book_and_page_(TAS5805M_REG_BOOK_5, TAS5805M_REG_BOOK_5_MIXER_PAGE)) {
    ESP_LOGE(TAG, "%s begin Set %s", ERROR, MIXER_MODE);
    return false;
  }

  if (!this->tas5805m_write_bytes_(TAS5805M_REG_LEFT_TO_LEFT_GAIN, reinterpret_cast<uint8_t *>(&mixer_l_to_l), 4)) {
    ESP_LOGE(TAG, "%s Mixer L-L Gain", ERROR);
    return false;
  }

  if (!this->tas5805m_write_bytes_(TAS5805M_REG_RIGHT_TO_RIGHT_GAIN, reinterpret_cast<uint8_t *>(&mixer_r_to_r), 4)) {
    ESP_LOGE(TAG, "%s Mixer R-R Gain", ERROR);
    return false;
  }

  if (!this->tas5805m_write_bytes_(TAS5805M_REG_LEFT_TO_RIGHT_GAIN, reinterpret_cast<uint8_t *>(&mixer_l_to_r), 4)) {
    ESP_LOGE(TAG, "%s Mixer L-R Gain", ERROR);
    return false;
  }

  if (!this->tas5805m_write_bytes_(TAS5805M_REG_RIGHT_TO_LEFT_GAIN, reinterpret_cast<uint8_t *>(&mixer_r_to_l), 4)) {
    ESP_LOGE(TAG, "%s Mixer R-L Gain", ERROR);
    return false;
  }

  if (!this->set_book_and_page_(TAS5805M_REG_BOOK_CONTROL_PORT, TAS5805M_REG_PAGE_ZERO)) {
    ESP_LOGE(TAG, "%s end Set %s", ERROR, MIXER_MODE);
    return false;
  }

  // 'tas5805m_state_' global already has mixer mode from YAML config
  // save anyway so 'set_mixer_mode' could be used more generally
  this->tas5805m_state_.mixer_mode = mode;
  ESP_LOGD(TAG, "%s: %s", MIXER_MODE, MIXER_MODE_TEXT[this->tas5805m_state_.mixer_mode]);
  return true;
}

bool Tas5805mComponent::get_state_(ControlState* state) {
  *state = this->tas5805m_state_.control_state;
  return true;
}

bool Tas5805mComponent::set_state_(ControlState state) {
  if (this->tas5805m_state_.control_state == state) return true;
  if (!this->tas5805m_write_byte_(TAS5805M_DEVICE_CTRL_2, state)) return false;
  this->tas5805m_state_.control_state = state;
  return true;
}

bool Tas5805mComponent::clear_fault_registers_() {
  if (!tas5805m_write_byte_(TAS5805M_FAULT_CLEAR, TAS5805M_ANALOG_FAULT_CLEAR)) return false;
  this->tas5805m_state_.times_faults_cleared++;
  ESP_LOGD(TAG, "Fault registers cleared");
  return true;
}

bool Tas5805mComponent::read_fault_registers_() {
  uint8_t current_faults[4];

  // read all faults registers
  if (!this->tas5805m_read_bytes_(TAS5805M_CHAN_FAULT, current_faults, 4)) return false;

  // check if any change CHAN_FAULT register as it contains 4 fault conditions(binary sensors)
  this->is_new_channel_fault_ = (current_faults[0] != this->tas5805m_state_.faults.channel_fault);
  if (this->is_new_channel_fault_) {
    this->tas5805m_state_.faults.channel_fault = current_faults[0];
  }

  // separate clock fault from GLOBAL_FAULT1 register since clock faults can occur often
  // check if any change in GLOBAL_FAULT1 register as it contains 4 fault conditions(binary sensors) excluding clock fault
  uint8_t current_global_fault = current_faults[1] & REMOVE_CLOCK_FAULT;
  //ESP_LOGD(TAG, "global fault: 0x%02X", current_global_fault);
  this->is_new_global_fault_ = (current_global_fault != this->tas5805m_state_.faults.global_fault);
  if (this->is_new_global_fault_) {
    this->tas5805m_state_.faults.global_fault = current_global_fault;
  }

  this->tas5805m_state_.faults.clock_fault = (current_faults[1] & (1 << 2));
  //ESP_LOGD(TAG, "clock fault: 0x%02X", (current_faults[1] & (1 << 2)));
  // over temperature fault is only fault condition in global_fault2 register
  this->tas5805m_state_.faults.temperature_fault = current_faults[2];

  // over temperature warning is only fault condition in ot_warning register
  this->tas5805m_state_.faults.temperature_warning = current_faults[3];

  return true;
}


// low level functions

bool Tas5805mComponent::set_book_and_page_(uint8_t book, uint8_t page) {
  if (!this->tas5805m_write_byte_(TAS5805M_REG_PAGE_SET, TAS5805M_REG_PAGE_ZERO)) {
    ESP_LOGE(TAG, "%s page 0", ERROR);
    return false;
  }
  if (!this->tas5805m_write_byte_(TAS5805M_REG_BOOK_SET, book)) {
    ESP_LOGE(TAG, "%s book 0x%02X", ERROR, book);
    return false;
  }
  if (!this->tas5805m_write_byte_(TAS5805M_REG_PAGE_SET, page)) {
    ESP_LOGE(TAG, "%s page 0x%02X", ERROR, page);
    return false;
  }
  return true;
}

bool Tas5805mComponent::tas5805m_read_byte_(uint8_t a_register, uint8_t* data) {
  return tas5805m_read_bytes_(a_register, data, 1);
  // i2c::ErrorCode error_code;
  // error_code = this->write(&a_register, 1);
  // if (error_code != i2c::ERROR_OK) {
  //   ESP_LOGE(TAG, "Write error:: %i", error_code);
  //   this->i2c_error_ = (uint8_t)error_code;
  //   return false;
  // }
  // error_code = this->read_register(a_register, data, 1, true);
  // if (error_code != i2c::ERROR_OK) {
  //   ESP_LOGE(TAG, "Read error: %i", error_code);
  //   this->i2c_error_ = (uint8_t)error_code;
  //   return false;
  // }
  return true;
}

bool Tas5805mComponent::tas5805m_read_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes) {
  i2c::ErrorCode error_code;
  error_code = this->write(&a_register, 1);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Write error:: %i", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  error_code = this->read_register(a_register, data, number_bytes, true);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Read error: %i", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

bool Tas5805mComponent::tas5805m_write_byte_(uint8_t a_register, uint8_t data) {
  i2c::ErrorCode error_code = this->write_register(a_register, &data, 1, true);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Write error: %i", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

bool Tas5805mComponent::tas5805m_write_bytes_(uint8_t a_register, uint8_t* data, uint8_t len) {
  i2c::ErrorCode error_code = this->write_register(a_register, data, len, true);
  if (error_code != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Write error: %i", error_code);
    this->i2c_error_ = (uint8_t)error_code;
    return false;
  }
  return true;
}

}  // namespace tas5805m
}  // namespace esphome
