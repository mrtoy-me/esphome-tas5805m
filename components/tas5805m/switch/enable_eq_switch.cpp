#include "esphome/core/log.h"
#include "enable_eq_switch.h"

namespace esphome {
namespace tas5805m {

static const char *const TAG = "tas5805m.switch";

void EnableEqSwitch::setup() {
  optional<bool> initial_state = this->get_initial_state_with_restore_mode();

  bool setup_state = initial_state.has_value() ? initial_state.value() : false;

  ESP_LOGD(TAG, "Enable EQ setup state: %s", ONOFF(setup_state));
  this->write_state(setup_state);

  // this allows a workaround to refresh eq gains where a sound is not played
  // workaround if on boot sound is not played through speaker mediplayer is:
  //  - audio_dac: tas5805m 'auto_refresh: EQ_SWITCH'
  //  - start playing music
  //  - turn Enable EQ Switch ON

  // if YAML configured auto_fresh: EQ_SWITCH then trigger refresh_settings
  if (this->parent_->use_eq_switch_refresh()) {
    this->trigger_refresh_settings_ = true;
  }
}

void EnableEqSwitch::dump_config() {
  LOG_SWITCH("", "Tas5805m Enable EQ Control switch:", this);
}

void EnableEqSwitch::write_state(bool state) {
  this->publish_state(state);
  this->parent_->set_eq(state);

  // normal condition
  if (!this->trigger_refresh_settings_) return;

  // when 'trigger_refresh_settings_' is set true by 'setup'
  // then effectively 'refresh_settings' triggers on first transition from Off to On
  // if 'refresh_settings' has already been called by YAML
  // it does not matter as'parent_->refresh_settings()'  will only run once
  if (state) {
    ESP_LOGD(TAG, "Triggering refresh settings");
    this->parent_->refresh_settings();
    this->trigger_refresh_settings_ = false;
  }
}

}  // namespace tas5805m
}  // namespace esphome