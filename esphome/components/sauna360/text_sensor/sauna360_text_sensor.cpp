#include "sauna360_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sauna360 {

static const char *const TAG = "sauna360.text_sensor";

void SAUNA360TextSensor::set_heater_state_text_sensor(
    text_sensor::TextSensor *tsensor) {
  this->heater_state_text_sensor_ = tsensor;
}

void SAUNA360TextSensor::set_heat_waves_text_sensor(
    text_sensor::TextSensor *tsensor) {
  this->heat_waves_text_sensor_ = tsensor;
}

void SAUNA360TextSensor::on_heater_state(const std::string &state) {
  if (this->heater_state_text_sensor_ != nullptr) {
    this->heater_state_text_sensor_->publish_state(state);
  }
  this->publish_state(state);
}

void SAUNA360TextSensor::on_coils_active(uint8_t cnt) {
  static uint8_t prev = 0xFF;
  if (this->heat_waves_text_sensor_ == nullptr || cnt == prev) return;
  prev = cnt;

  // U+25CF "●" and U+25CB "○" as UTF-8
  static constexpr const char *ON  = "\xE2\x97\x8F"; // ●
  static constexpr const char *OFF = "\xE2\x97\x8B"; // ○

  std::string s;
  s.reserve(16);
  for (uint8_t i = 0; i < 3; ++i) {
    s += (i < cnt) ? ON : OFF;
    if (i < 2) s += ' ';
  }
  this->heat_waves_text_sensor_->publish_state(s);
}

void SAUNA360TextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "SAUNA360 TextSensor:");
  LOG_TEXT_SENSOR("  ", "Heater State", this->heater_state_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Heat Waves", this->heat_waves_text_sensor_);
}

} // namespace sauna360
} // namespace esphome
