#include "enable_dac_switch.h"
#include "esphome/core/log.h"

namespace esphome::tas5805m {

static const char *const TAG = "tas5805m.switch";

void EnableDacSwitch::setup() {
  optional<bool> initial_state = this->get_initial_state_with_restore_mode();
  bool setup_state = initial_state.has_value() ? initial_state.value() : false;
  this->write_state(setup_state);
}

void EnableDacSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas5805m Switch:");
  LOG_SWITCH("  ", "Enable Dac", this);
}

void EnableDacSwitch::write_state(bool state) {
  this->publish_state(state);
  this->parent_->enable_dac(state);
}

}  // namespace esphome::tas5805m
