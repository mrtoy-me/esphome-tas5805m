#include "enable_dac_switch.h"
#include "esphome/core/log.h"

namespace esphome::tas5805m {

static const char *const TAG = "tas5805m.select";

void EqModeSelect::setup() {
  optional<bool> initial_state = this->get_initial_state_with_restore_mode();
  bool setup_state = initial_state.has_value() ? initial_state.value() : false;
  this->write_state(setup_state);
}

void EqModeSelect::dump_config() {
  ESP_LOGCONFIG(TAG, "Tas5805m Select:");
  LOG_SELECT("  ", "Eq Mode", this);
}

void EqModeSelect::control(size_t index) {
  this->publish_state(index);
  this->parent_->set_eq_mode_select(index);

}  // namespace esphome::tas5805m
