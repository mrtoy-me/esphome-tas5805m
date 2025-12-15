#include "esphome/core/log.h"
#include "eq_gain_band315hz.h"

namespace esphome::tas5805m {

static const char *const TAG = "tas5805m.number";

void EqGainBand315hz::setup() {
  float value;
  this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
  if (!this->pref_.load(&value)) value = 0.0; // no saved gain so set to 0dB
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_315HZ, static_cast<int>(value));
}

void EqGainBand315hz::dump_config() {
  ESP_LOGCONFIG(TAG, "  315Hz Band '%s'", this->get_name().c_str());
}

void EqGainBand315hz::control(float value) {
  this->publish_state(value);
  this->parent_->set_eq_gain(BAND_315HZ, static_cast<int>(value));
  this->pref_.save(&value);
}

}  // namespace esphome::tas5805m
