#include "sauna360_climate.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sauna360 {

static const char *TAG = "sauna360_climate";

void Sauna360Climate::setup() {
  this->target_temperature_ = NAN;
  if (this->controller_ == nullptr) {
  } else {
    this->controller_->register_listener(this);
  }
}

climate::ClimateTraits Sauna360Climate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supported_modes({climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_OFF});
  traits.set_visual_min_temperature(40.0f);
  traits.set_visual_max_temperature(110.0f);
  traits.set_visual_temperature_step(1.0f);
  return traits;
}

void Sauna360Climate::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float target_temp = *call.get_target_temperature();
    if (std::isnan(this->target_temperature_) || this->target_temperature_ != target_temp) {
      this->target_temperature_ = target_temp;
      if (this->bath_temperature_number_ != nullptr) {
        this->bath_temperature_number_->make_call().set_value(target_temp).perform();
      }
    } else {
      ESP_LOGD(TAG, "Target temperature unchanged. Skipping update.");
    }
  }

  if (call.get_mode().has_value()) {
    auto mode = *call.get_mode();
    if (mode == climate::CLIMATE_MODE_OFF && this->heater_relay_ != nullptr) {
      this->heater_relay_->turn_off();
    } else if (mode == climate::CLIMATE_MODE_HEAT && this->heater_relay_ != nullptr) {
      this->heater_relay_->turn_on();
    }
  }
  this->publish_state();
}

void Sauna360Climate::on_temperature(uint16_t temperature) {
  float new_temperature = static_cast<float>(temperature);
  this->current_temperature_ = new_temperature;
  this->current_temperature = this->current_temperature_;
  this->publish_state();
}

void Sauna360Climate::on_temperature_setting(uint16_t temperature_setting) {
  float new_target_temperature = static_cast<float>(temperature_setting);
  this->target_temperature_ = new_target_temperature;
  this->target_temperature = new_target_temperature;
  this->publish_state();
}

void Sauna360Climate::on_heater_status(bool heater_status) {
  this->heater_on_ = heater_status;
  if (heater_status) {
    this->mode = climate::CLIMATE_MODE_HEAT;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }
  this->publish_state();
}

void Sauna360Climate::set_controller(SAUNA360Component *controller) {
  this->controller_ = controller;
}

void Sauna360Climate::set_bath_temperature_number(number::Number *bath_temperature_number) {
  this->bath_temperature_number_ = bath_temperature_number;
  bath_temperature_number->add_on_state_callback([this](float value) { 
    this->target_temperature_ = value;
    this->target_temperature = value;
    if (this->controller_ != nullptr) {
      this->controller_->set_bath_temperature_number(static_cast<uint16_t>(value));
    }
    this->publish_state();
  });
}

void Sauna360Climate::set_heater_relay(switch_::Switch *heater_relay) {
  this->heater_relay_ = heater_relay;  
  heater_relay->add_on_state_callback([this](bool state) {this->on_heater_status(state);});
}

}  // namespace sauna360
}  // namespace esphome
