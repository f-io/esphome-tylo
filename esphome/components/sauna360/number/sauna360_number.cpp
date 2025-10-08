#include "sauna360_number.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace sauna360
    {
        void SAUNA360BathTimeNumber::control(float value) { this->parent_->set_bath_time_number(value); }
        void SAUNA360BathTemperatureNumber::control(float value) { this->parent_->set_bath_temperature_number(value); }
        void SAUNA360MaxBathTemperatureNumber::control(float value) { this->parent_->set_max_bath_temperature_number(value); }
        void SAUNA360HumidityStepNumber::control(float value) { this->parent_->set_humidity_step_number(value); }
        void SAUNA360HumidityPercentNumber::control(float value) { this->parent_->set_humidity_percent_number(value); }

    } // namespace sauna360
} // namespace esphome