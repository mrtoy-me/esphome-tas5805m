#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "../tas5805m.h"

namespace esphome {
namespace tas5805m {

class EnableEqSwitch : public switch_::Switch, public Component, public Parented<Tas5805mComponent> {
public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

protected:
  void write_state(bool state) override;

  bool trigger_refresh_settings_{false};
};

}  // namespace tas5805m
}  // namespace esphome