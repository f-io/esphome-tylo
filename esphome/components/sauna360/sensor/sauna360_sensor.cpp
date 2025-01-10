#include "sauna360_sensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sauna360 {

static const char *const TAG = "SAUNA360.sensor";

void SAUNA360Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "SAUNA360 Sensor:");
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
}

}  // namespace sauna360
}  // namespace esphome