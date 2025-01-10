#pragma once

#include "../sauna360.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace sauna360 {

class Sauna360Climate : public climate::Climate, public Component, public SAUNA360Listener {
 public:
  void setup() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void set_controller(SAUNA360Component *controller);
  void set_bath_temperature_number(number::Number *bath_temperature_number);
  void set_heater_relay(switch_::Switch *heater_relay);

  void on_temperature(uint16_t temperature) override;
  void on_temperature_setting(uint16_t temperature_setting) override;
  void on_heater_status(bool heater_status) override;

 private:
  float current_temperature_ = NAN;
  float target_temperature_ = NAN;
  bool heater_on_ = false;

  SAUNA360Component *controller_ = nullptr;
  number::Number *bath_temperature_number_ = nullptr;
  switch_::Switch *heater_relay_ = nullptr;
};

}  // namespace sauna360
}  // namespace esphome
