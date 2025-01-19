#pragma once

#include "esphome/components/sensor/sensor.h"
#include "../sauna360.h"

namespace esphome
{
  namespace sauna360
  {

    class SAUNA360Sensor : public SAUNA360Listener, public Component, sensor::Sensor
    {
    public:
      void dump_config() override;
      void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }
      void on_temperature(uint16_t temperature) override
      {
        if (this->temperature_sensor_ != nullptr)
        {
          if (this->temperature_sensor_->get_state() != temperature)
          {
            this->temperature_sensor_->publish_state(temperature);
          }
        }
      }
      void set_temperature_setting_sensor(sensor::Sensor *sensor) { this->temperature_setting_sensor_ = sensor; }
      void on_temperature_setting(uint16_t temperature_setting) override
      {
        if (this->temperature_setting_sensor_ != nullptr)
        {
          if (this->temperature_setting_sensor_->get_state() != temperature_setting)
          {
            this->temperature_setting_sensor_->publish_state(temperature_setting);
          }
        }
      }
      void set_remaining_time_sensor(sensor::Sensor *sensor) { this->remaining_time_sensor_ = sensor; }
      void on_remaining_time(uint16_t remaining_time) override
      {
        if (this->remaining_time_sensor_ != nullptr)
        {
          if (this->remaining_time_sensor_->get_state() != remaining_time)
          {
            this->remaining_time_sensor_->publish_state(remaining_time);
          }
        }
      }
      void set_bath_time_setting_sensor(sensor::Sensor *sensor) { this->bath_time_setting_sensor_ = sensor; }
      void on_bath_time_setting(uint16_t bath_time_setting) override
      {
        if (this->bath_time_setting_sensor_ != nullptr)
        {
          if (this->bath_time_setting_sensor_->get_state() != bath_time_setting)
          {
            this->bath_time_setting_sensor_->publish_state(bath_time_setting);
          }
        }
      }
      void set_total_uptime_sensor(sensor::Sensor *sensor) { this->total_uptime_sensor_ = sensor; }
      void on_total_uptime(uint16_t total_uptime) override
      {
        if (this->total_uptime_sensor_ != nullptr)
        {
          if (this->total_uptime_sensor_->get_state() != total_uptime)
          {
            this->total_uptime_sensor_->publish_state(total_uptime);
          }
        }
      }
      void set_max_bath_temperature_sensor(sensor::Sensor *sensor) { this->max_bath_temperature_sensor_ = sensor; }
      void on_max_bath_temperature(uint16_t max_bath_temperature) override
      {
        if (this->max_bath_temperature_sensor_ != nullptr)
        {
          if (this->max_bath_temperature_sensor_->get_state() != max_bath_temperature)
          {
            this->max_bath_temperature_sensor_->publish_state(max_bath_temperature);
          }
        }
      }
      void set_overheating_pcb_limit_sensor(sensor::Sensor *sensor) { this->overheating_pcb_limit_sensor_ = sensor; }
      void on_overheating_pcb_limit(uint16_t overheating_pcb_limit) override
      {
        if (this->overheating_pcb_limit_sensor_ != nullptr)
        {
          if (this->overheating_pcb_limit_sensor_->get_state() != overheating_pcb_limit)
          {
            this->overheating_pcb_limit_sensor_->publish_state(overheating_pcb_limit);
          }
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