#pragma once

#include "../sauna360.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace sauna360 {

class SAUNA360Sensor : public SAUNA360Listener, public Component {
public:
  void dump_config() override;

  void set_temperature_sensor(sensor::Sensor *s) {
    this->temperature_sensor_ = s;
  }
  void on_temperature(uint16_t v) override {
    if (this->temperature_sensor_ != nullptr) {
      if (this->temperature_sensor_->get_state() != v)
        this->temperature_sensor_->publish_state(v);
    }
  }

  void set_temperature_setting_sensor(sensor::Sensor *s) {
    this->temperature_setting_sensor_ = s;
  }
  void on_temperature_setting(uint16_t v) override {
    if (this->temperature_setting_sensor_ != nullptr) {
      if (this->temperature_setting_sensor_->get_state() != v)
        this->temperature_setting_sensor_->publish_state(v);
    }
  }

  void set_remaining_time_sensor(sensor::Sensor *s) {
    this->remaining_time_sensor_ = s;
  }
  void on_remaining_time(uint16_t v) override {
    if (this->remaining_time_sensor_ != nullptr) {
      if (this->remaining_time_sensor_->get_state() != v)
        this->remaining_time_sensor_->publish_state(v);
    }
  }

  void set_bath_time_setting_sensor(sensor::Sensor *s) {
    this->bath_time_setting_sensor_ = s;
  }
  void on_bath_time_setting(uint16_t v) override {
    if (this->bath_time_setting_sensor_ != nullptr) {
      if (this->bath_time_setting_sensor_->get_state() != v)
        this->bath_time_setting_sensor_->publish_state(v);
    }
  }

  void set_total_uptime_sensor(sensor::Sensor *s) {
    this->total_uptime_sensor_ = s;
  }
  void on_total_uptime(uint32_t v) override { // <-- uint32_t!
    if (this->total_uptime_sensor_ != nullptr) {
      if (this->total_uptime_sensor_->get_state() != static_cast<float>(v))
        this->total_uptime_sensor_->publish_state(static_cast<float>(v));
    }
  }

  void set_max_bath_temperature_sensor(sensor::Sensor *s) {
    this->max_bath_temperature_sensor_ = s;
  }
  void on_max_bath_temperature(uint16_t v) override {
    if (this->max_bath_temperature_sensor_ != nullptr) {
      if (this->max_bath_temperature_sensor_->get_state() != v)
        this->max_bath_temperature_sensor_->publish_state(v);
    }
  }

  void set_overheating_pcb_limit_sensor(sensor::Sensor *s) {
    this->overheating_pcb_limit_sensor_ = s;
  }
  void on_overheating_pcb_limit(uint16_t v) override {
    if (this->overheating_pcb_limit_sensor_ != nullptr) {
      if (this->overheating_pcb_limit_sensor_->get_state() != v)
        this->overheating_pcb_limit_sensor_->publish_state(v);
    }
  }

protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *temperature_setting_sensor_{nullptr};
  sensor::Sensor *remaining_time_sensor_{nullptr};
  sensor::Sensor *bath_time_setting_sensor_{nullptr};
  sensor::Sensor *total_uptime_sensor_{nullptr};
  sensor::Sensor *max_bath_temperature_sensor_{nullptr};
  sensor::Sensor *overheating_pcb_limit_sensor_{nullptr};
};

} // namespace sauna360
} // namespace esphome
