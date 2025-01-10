#include "sauna360_switch.h"

namespace esphome {
namespace sauna360 {

void SAUNA360LightRelaySwitch::write_state(bool state) {
    this->publish_state(state);
    this->parent_->set_light_relay(state);
  }
void SAUNA360HeaterRelaySwitch::write_state(bool state) {
    this->publish_state(state);
    this->parent_->set_heater_relay(state);
  }  
}  // namespace sauna360
}  // namespace esphome