#pragma once

#include "esphome/components/audio_dac/audio_dac.h"
#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/hal.h"
#include "tas5805m_cfg.h"

#ifdef USE_TAS5805M_EQ
#include "tas5805m_eq.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace tas5805m {

enum AutoRefreshMode : uint8_t {
    BY_GAIN   = 0,
    BY_SWITCH = 1,
};

class Tas5805mComponent : public audio_dac::AudioDac, public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }

  // save in 'tas5805m_state_' now and set in 'setup'
  void config_analog_gain(float analog_gain) { this->tas5805m_state_.analog_gain = analog_gain; }

  void config_auto_refresh(AutoRefreshMode auto_refresh) { this->auto_refresh_ = auto_refresh; }

  // save in 'tas5805m_state_' now and set in 'setup'
  void config_dac_mode(DacMode dac_mode) {this->tas5805m_state_.dac_mode = dac_mode; }

  // save in 'tas5805m_state_' now and set once in 'loop'
  // 'mixer_mode_configured_' used by 'loop' ensures it only runs once
  void config_mixer_mode(MixerMode mixer_mode) {this->tas5805m_state_.mixer_mode = mixer_mode; }

  void config_volume_max(float volume_max) {this->tas5805m_state_.volume_max = (int8_t)(volume_max); }
  void config_volume_min(float volume_min) {this->tas5805m_state_.volume_min = (int8_t)(volume_min); }

  #ifdef USE_BINARY_SENSOR
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
  #endif

  float volume() override;
  bool set_volume(float value) override;

  bool set_mute_off() override;
  bool set_mute_on() override;
  bool is_muted() override { return this->is_muted_; }

  bool set_deep_sleep_off();
  bool set_deep_sleep_on();

  uint32_t times_faults_cleared();
  uint8_t last_channel_fault();
  uint8_t last_global_fault();

  void enable_dac(bool enable);

  bool set_eq(bool enable);

  bool use_eq_gain_refresh();
  bool use_eq_switch_refresh();

  void refresh_settings();

  #ifdef USE_TAS5805M_EQ
  bool set_eq_gain(uint8_t band, int8_t gain);
  #endif

 protected:
   GPIOPin* enable_pin_{nullptr};

   bool configure_registers();

   // set analog gain in dB from -15.5 to 0 in 0.5dB increments
   bool set_analog_gain(float gain_db);

   // reads analog gain register and returns number 0-31
   bool get_analog_gain(uint8_t* raw_gain);

   bool get_dac_mode(DacMode* mode);
   bool set_dac_mode(DacMode mode);

   bool set_eq_on();
   bool set_eq_off();

   bool get_mixer_mode(MixerMode *mode);
   bool set_mixer_mode(MixerMode mode);

   bool get_digital_volume(uint8_t* raw_volume);
   bool set_digital_volume(uint8_t new_volume);

   bool get_state(ControlState* state);
   bool set_state(ControlState state);

   bool initialise_fault_registers();
   bool clear_fault_registers();
   bool read_fault_registers();

   #ifdef USE_TAS5805M_EQ
   bool get_eq(bool* enabled);
   #endif

   bool set_book_and_page(uint8_t book, uint8_t page);

   bool tas5805m_read_byte(uint8_t a_register, uint8_t* data);
   bool tas5805m_write_byte(uint8_t a_register, uint8_t data);
   bool tas5805m_write_bytes(uint8_t a_register, uint8_t *data, uint8_t len);

   enum ErrorCode {
     NONE = 0,
     CONFIGURATION_FAILED,
     WRITE_REGISTER_FAILED
   } error_code_{NONE};

   struct Tas5805mState {
    //bool               is_muted;                   // not used as esphome AudioDac component has its own is_muted variable
    //bool               is_powered;                 // currently not used
    float             analog_gain;
    int8_t            volume_max;
    int8_t            volume_min;
    uint8_t           raw_volume_max;
    uint8_t           raw_volume_min;
    DacMode           dac_mode;
    MixerMode         mixer_mode;

    ControlState      control_state;

    uint32_t          times_faults_cleared{0};
    uint8_t           last_channel_fault{0};
    uint8_t           last_global_fault{0};
    uint8_t           last_over_temperature_fault{0};
    uint8_t           last_over_temperature_warning{0};

    #ifdef USE_TAS5805M_EQ
    bool              eq_enabled;
    int8_t            eq_gain[TAS5805M_EQ_BANDS]{0};
    #endif

   } tas5805m_state_;

   AutoRefreshMode auto_refresh_;

   // does dac have any fault
   bool have_fault_{false};

   // only write mixer mode once
   bool mixer_mode_configured_{false};

   // indicates if mixer mode and eq gains have been written yet
   // used to ensure settings are only written once by 'loop'
   // only ever changed to true once when 'loop' has completed writing settings
   bool refresh_settings_complete_{false};

   // only ever changed to true once when 'refresh_settings()' is run
   // when true 'set_eq_gains' is allowed to write eq gains
   // when 'refresh_settings_complete_' is false and 'refresh_settings_triggered_' is true
   // 'loop' will write mixer mode and if setup in YAML, the eq gains
   bool refresh_settings_triggered_{false};

   bool update_delay_finished_{false};

   // has eq gains been configured in YAML
   #ifdef USE_TAS5805M_EQ
   bool using_eq_gains_{true};
   #else
   bool using_eq_gains_{false};
   #endif

   uint8_t refresh_band_{0};
   uint8_t i2c_error_{0};
   uint8_t loop_counter_{0};
   uint32_t last_update_time_;

   uint16_t number_registers_configured_{0};
};

}  // namespace tas5805m
}  // namespace esphome
