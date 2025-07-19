#include "esphome/core/log.h"
#include "enable_dac_switch.h"

namespace esphome {
namespace tas5805m {

static const char *const TAG = "tas5805m.switch";

void EnableDacSwitch::setup() {
  optional<bool> initial_state = this->get_initial_state_with_restore_mode();

  bool setup_state = initial_state.has_value() ? initial_state.value() : false;

  ESP_LOGD(TAG, "Enable DAC setup state: %s", ONOFF(setup_state));
  this->write_state(setup_state);
}

void EnableDacSwitch::dump_config() {
  LOG_SWITCH("", "Tas5805m Enable Dac switch:", this);
}

void EnableDacSwitch::write_state(bool state) {
  this->publish_state(state);
  this->parent_->enable_dac(state);
}

}  // namespace tas5805m
}  // namespace esphome