#pragma once

#include "esphome/components/audio_dac/audio_dac.h"
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/hal.h"
#include "tas5805m_cfg.h"

#ifdef USE_TAS5805M_EQ
#include "tas5805m_eq.h"
#endif

#ifdef USE_TAS5805M_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome::tas5805m {

enum AutoRefreshMode : uint8_t {
    BY_GAIN   = 0,
    BY_SWITCH = 1,
};

enum ExcludeIgnoreMode : uint8_t {
    NONE        = 0,
    CLOCK_FAULT = 1,
};

class Tas5805mComponent : public audio_dac::AudioDac, public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;

  void loop() override;
  void update() override;

  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::IO; }

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  // optional YAML config

  void config_analog_gain(float analog_gain) { this->tas5805m_analog_gain_ = analog_gain; }

  void config_dac_mode(DacMode dac_mode) {this->tas5805m_dac_mode_ = dac_mode; }

  void config_ignore_fault_mode(ExcludeIgnoreMode ignore_fault_mode) {
    this->ignore_clock_faults_when_clearing_faults_ = (ignore_fault_mode == ExcludeIgnoreMode::CLOCK_FAULT);
  }

  void config_mixer_mode(MixerMode mixer_mode) {this->tas5805m_mixer_mode_ = mixer_mode; }

  void config_refresh_eq(AutoRefreshMode auto_refresh) { this->auto_refresh_ = auto_refresh; }

  void config_volume_max(float volume_max) {this->tas5805m_volume_max_ = (int8_t)(volume_max); }
  void config_volume_min(float volume_min) {this->tas5805m_volume_min_ = (int8_t)(volume_min); }

  #ifdef USE_TAS5805M_BINARY_SENSOR
  SUB_BINARY_SENSOR(have_fault)
  SUB_BINARY_SENSOR(left_channel_dc_fault)
  SUB_BINARY_SENSOR(right_channel_dc_fault)
  SUB_BINARY_SENSOR(left_channel_over_current_fault)
  SUB_BINARY_SENSOR(right_channel_over_current_fault)

  SUB_BINARY_SENSOR(otp_crc_check_error)
  SUB_BINARY_SENSOR(bq_write_failed_fault)
  SUB_BINARY_SENSOR(clock_fault)
  SUB_BINARY_SENSOR(pvdd_over_voltage_fault)
  SUB_BINARY_SENSOR(pvdd_under_voltage_fault)

  SUB_BINARY_SENSOR(over_temperature_shutdown_fault)
  SUB_BINARY_SENSOR(over_temperature_warning)

  void config_exclude_fault(ExcludeIgnoreMode exclude_fault) {
    this->exclude_clock_fault_from_have_faults_ = (exclude_fault == ExcludeIgnoreMode::CLOCK_FAULT);
  }
  #endif

  void enable_dac(bool enable);

  bool enable_eq(bool enable);

  #ifdef USE_TAS5805M_EQ
  bool set_eq_gain(uint8_t band, int8_t gain);
  #endif

  bool is_muted() override { return this->is_muted_; }
  bool set_mute_off() override;
  bool set_mute_on() override;

  void refresh_settings();

  uint32_t times_faults_cleared();

  bool use_eq_gain_refresh();
  bool use_eq_switch_refresh();

  float volume() override;
  bool set_volume(float value) override;

 protected:
   GPIOPin* enable_pin_{nullptr};

   bool configure_registers_();

   bool get_analog_gain_(uint8_t* raw_gain);
   bool set_analog_gain_(float gain_db);

   bool get_dac_mode_(DacMode* mode);
   bool set_dac_mode_(DacMode mode);

   bool set_deep_sleep_off_();
   bool set_deep_sleep_on_();

   bool get_digital_volume_(uint8_t* raw_volume);
   bool set_digital_volume_(uint8_t new_volume);

   #ifdef USE_TAS5805M_EQ
   bool get_eq_(bool* enabled);
   #endif

   bool set_eq_on_();
   bool set_eq_off_();

   bool get_mixer_mode_(MixerMode *mode);
   bool set_mixer_mode_(MixerMode mode);

   bool get_state_(ControlState* state);
   bool set_state_(ControlState state);

   // manage faults
   bool clear_fault_registers_();
   bool read_fault_registers_();
   #ifdef USE_TAS5805M_BINARY_SENSOR
   void publish_faults_();
   void publish_channel_faults_();
   void publish_global_faults_();
   #endif

   // low level functions
   bool set_book_and_page_(uint8_t book, uint8_t page);

   bool tas5805m_read_byte_(uint8_t a_register, uint8_t* data);
   bool tas5805m_read_bytes_(uint8_t a_register, uint8_t* data, uint8_t number_bytes);
   bool tas5805m_write_byte_(uint8_t a_register, uint8_t data);
   bool tas5805m_write_bytes_(uint8_t a_register, uint8_t *data, uint8_t len);

   enum ErrorCode {
     NONE = 0,
     CONFIGURATION_FAILED,
   } error_code_{NONE};

   // configured by YAML
   AutoRefreshMode auto_refresh_;  // default 'BY_GAIN' = 0

   #ifdef USE_TAS5805M_BINARY_SENSOR
   bool exclude_clock_fault_from_have_faults_; // default = false
   #endif

   bool ignore_clock_faults_when_clearing_faults_; // default = false

   DacMode tas5805m_dac_mode_;

   float tas5805m_analog_gain_;

   int8_t tas5805m_volume_max_;
   int8_t tas5805m_volume_min_;

   MixerMode tas5805m_mixer_mode_;

   // used if eq gain numbers are defined in YAML
   #ifdef USE_TAS5805M_EQ
   bool tas5805m_eq_enabled_;
   int8_t tas5805m_eq_gain_[NUMBER_EQ_BANDS]{0};
   #endif

   // initialised in setup
   ControlState tas5805m_control_state_;

   uint8_t tas5805m_raw_volume_max_;
   uint8_t tas5805m_raw_volume_min_;

   // fault processing
   bool have_fault_to_clear_{false}; // false so clear fault registers is skipped on first update

   // has the state of any fault in group changed - used to conditionally publish binary sensors
   // true so all binary sensors are published on first update
   bool is_new_channel_fault_{true};
   bool is_new_common_fault_{true};
   bool is_new_global_fault_{true};
   bool is_new_over_temperature_issue_{true};

   // current state of faults
   Tas5805mFault tas5805m_faults_;

   // counts number of times the faults register is cleared (used for publishing to sensor)
   uint32_t times_faults_cleared_{0};

   // only ever changed to true once when mixer mode is written
   // used by 'loop'
   bool mixer_mode_configured_{false};

   // only ever changed to true once when 'loop' has completed refreshing settings
   // used to trigger disabling of 'loop'
   bool refresh_settings_complete_{false};

   // only ever changed to true once to trigger 'refresh_settings()'
   // when true 'set_eq_gains' is allowed to write eq gains
   // when 'refresh_settings_complete_' is false and 'refresh_settings_triggered_' is true
   // 'loop' will write mixer mode and if setup in YAML, also eq gains
   bool refresh_settings_triggered_{false};

   // use to indicate if delay before starting 'update' starting is complete
   bool update_delay_finished_{false};

   // are eq gain numbers configured in YAML
   #ifdef USE_TAS5805M_EQ
   bool using_eq_gains_{true};
   #else
   bool using_eq_gains_{false};
   #endif

   // eq band currently being refreshed
   uint8_t refresh_band_{0};

   // last i2c error, if there is error shown by 'dump_config'
   uint8_t i2c_error_{0};

   // used for counting number of 'loops' iterations for delay of starting 'loop'
   uint8_t loop_counter_{0};

   // number tas5805m registers configured during 'setup'
   uint16_t number_registers_configured_{0};

   // initialised in loop, used for delay in starting 'update'
   uint32_t start_time_;
};

}  // namespace esphome::tas5805m
