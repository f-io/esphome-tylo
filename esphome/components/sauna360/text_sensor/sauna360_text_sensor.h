#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "../sauna360.h"

namespace esphome {
namespace sauna360 {

class SAUNA360TextSensor : public SAUNA360Listener, public Component, text_sensor::TextSensor {
 public:
  void dump_config() override;
  void set_heater_state_text_sensor(text_sensor::TextSensor *tsensor) { this->heater_state_text_sensor_ = tsensor; };
  void on_heater_state(std::string &fw) override {
    if (this->heater_state_text_sensor_ != nullptr) {
      this->heater_state_text_sensor_->publish_state(fw);
    }
  }

 protected:
  text_sensor::TextSensor *heater_state_text_sensor_{nullptr};
};

}  // namespace sauna360
}  // namespace esphome