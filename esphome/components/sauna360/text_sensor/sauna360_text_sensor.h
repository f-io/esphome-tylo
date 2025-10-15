#pragma once
#include "../sauna360.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace sauna360 {

class SAUNA360TextSensor : public Component,
                           public text_sensor::TextSensor,
                           public SAUNA360Listener {
public:
  void set_heater_state_text_sensor(text_sensor::TextSensor *tsensor);
  void on_heater_state(const std::string &state) override;
  void dump_config() override;

protected:
  text_sensor::TextSensor *heater_state_text_sensor_{nullptr};
};

} // namespace sauna360
} // namespace esphome
