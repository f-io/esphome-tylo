#include "sauna360_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sauna360 {

static const char *const TAG = "sauna360.text_sensor";

void SAUNA360TextSensor::set_heater_state_text_sensor(
    text_sensor::TextSensor *tsensor) {
  this->heater_state_text_sensor_ = tsensor;
}

void SAUNA360TextSensor::on_heater_state(const std::string &state) {
  if (this->heater_state_text_sensor_ != nullptr) {
    this->heater_state_text_sensor_->publish_state(state);
  }
  this->publish_state(state);
}

void SAUNA360TextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "SAUNA360 TextSensor:");
  LOG_TEXT_SENSOR("  ", "Heater State", this->heater_state_text_sensor_);
}

} // namespace sauna360
} // namespace esphome
