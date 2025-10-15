#include "sauna360_binary_sensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace sauna360
  {

    static const char *const TAG = "sauna360.binary_sensor";

    void SAUNA360BinarySensor::dump_config()
    {
      ESP_LOGCONFIG(TAG, "SAUNA360 BinarySensor:");
      LOG_BINARY_SENSOR("  ", "HEATER STATUS", this->heater_bsensor_);
      LOG_BINARY_SENSOR("  ", "LIGHT STATUS", this->light_bsensor_);
      LOG_BINARY_SENSOR("  ", "READY STATUS", this->ready_bsensor_);
    }

  } // namespace sauna360
} // namespace esphome