#include "sauna360_sensor.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sauna360 {

static const char *const TAG = "sauna360.sensor";

void SAUNA360Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "SAUNA360 Sensor:");
  LOG_SENSOR("  ", "Temperature",                 this->temperature_sensor_);
  LOG_SENSOR("  ", "Temperature Setting",         this->temperature_setting_sensor_);
  LOG_SENSOR("  ", "Remaining Time (min)",        this->remaining_time_sensor_);
  LOG_SENSOR("  ", "Setting Bath Time (min)",     this->bath_time_setting_sensor_);
  LOG_SENSOR("  ", "Total Uptime (min)",          this->total_uptime_sensor_);
  LOG_SENSOR("  ", "Max Bath Temperature (°C)",   this->max_bath_temperature_sensor_);
  LOG_SENSOR("  ", "Overheating PCB Limit (°C)",  this->overheating_pcb_limit_sensor_);
  LOG_SENSOR("  ", "Setting Humidity Step",       this->setting_humidity_step_sensor_);
  LOG_SENSOR("  ", "Setting Humidity (%)",        this->setting_humidity_percent_sensor_);
  LOG_SENSOR("  ", "Water Tank Level (%)",        this->water_tank_level_sensor_);
  LOG_SENSOR("  ", "Session Uptime (min)",        this->session_uptime_sensor_);
}

}  // namespace sauna360
}  // namespace esphome
