#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../sauna360.h"

namespace esphome
{
  namespace sauna360
  {

    class SAUNA360BinarySensor : public SAUNA360Listener, public Component, binary_sensor::BinarySensor
    {
    public:
      void dump_config() override;
      void set_heater_binary_sensor(binary_sensor::BinarySensor *bsensor) { this->heater_bsensor_ = bsensor; };
      void on_heater_status(bool heater_status) override
      {
        if (this->heater_bsensor_ != nullptr)
        {
          if (this->heater_bsensor_->state != heater_status)
          {
            this->heater_bsensor_->publish_state(heater_status);
          }
        }
      }

      void set_light_binary_sensor(binary_sensor::BinarySensor *bsensor) { this->light_bsensor_ = bsensor; };
      void on_light_status(bool light_status) override
      {
        if (this->light_bsensor_ != nullptr)
        {
          if (this->light_bsensor_->state != light_status)
          {
            this->light_bsensor_->publish_state(light_status);
          }
        }
      }

      void set_ready_binary_sensor(binary_sensor::BinarySensor *bsensor) { this->ready_bsensor_ = bsensor; };
      void on_ready_status(bool ready_status) override
      {
        if (this->ready_bsensor_ != nullptr)
        {
          if (this->ready_bsensor_->state != ready_status)
          {
            this->ready_bsensor_->publish_state(ready_status);
          }
        }
      }

    protected:
      binary_sensor::BinarySensor *heater_bsensor_{nullptr};
      binary_sensor::BinarySensor *light_bsensor_{nullptr};
      binary_sensor::BinarySensor *ready_bsensor_{nullptr};
    };

  } // namespace sauna360
} // namespace esphome