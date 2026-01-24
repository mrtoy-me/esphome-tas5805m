#pragma once

#include "../tas5805m.h"
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

namespace esphome::tas5805m {

class EqModeSelect : public select::Select, public Component, public Parented<Tas5805mComponent> {
public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

protected:
  void control(size_t index) override;
};

}  // namespace esphome::tas5805m
