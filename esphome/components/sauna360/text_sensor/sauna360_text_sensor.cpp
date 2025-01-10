#include "sauna360_text_sensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sauna360 {

static const char *const TAG = "SAUNA360.text_sensor";

void SAUNA360TextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "SAUNA360 TextSensor:");
  LOG_TEXT_SENSOR("  ", "Heater State", this->heater_state_text_sensor_);
}


}  // namespace sauna360
}  // namespace esphome