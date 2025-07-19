#include "esphome/core/log.h"
#include "fault_sensor.h"

namespace esphome {
namespace tas5805m {

static const char *const TAG = "tas5805m.sensor";

void  FaultSensor::dump_config() {
  LOG_SENSOR("", "Tas5805m Times Faults Cleared sensor:", this->times_faults_cleared_sensor_);
  LOG_UPDATE_INTERVAL(this);
}

void  FaultSensor::update() {
  if (this->times_faults_cleared_sensor_ != nullptr) {
    // only publish if different to last value but will publish first value
    uint32_t current_faults_cleared = this->parent_->times_faults_cleared();
    if (current_faults_cleared != this->last_faults_cleared_) {
      this->times_faults_cleared_sensor_->publish_state(current_faults_cleared);
      this->last_faults_cleared_ = current_faults_cleared;
    }
  }
}

}  // namespace tas5805m
}  // namespace esphome