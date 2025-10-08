#pragma once

#include "esphome/components/number/number.h"
#include "../sauna360.h"

namespace esphome
{
  namespace sauna360
  {

    class SAUNA360BathTimeNumber : public number::Number, public Parented<SAUNA360Component>
    {
    public:
      SAUNA360BathTimeNumber() = default;

    protected:
      void control(float value) override;
    };

    class SAUNA360BathTemperatureNumber : public number::Number, public Parented<SAUNA360Component>
    {
    public:
      SAUNA360BathTemperatureNumber() = default;

    protected:
      void control(float value) override;
    };

    class SAUNA360MaxBathTemperatureNumber : public number::Number, public Parented<SAUNA360Component>
    {
    public:
      SAUNA360MaxBathTemperatureNumber() = default;

    protected:
      void control(float value) override;
    };

    class SAUNA360HumidityStepNumber : public number::Number, public Parented<SAUNA360Component>
    {
    public:
      SAUNA360HumidityStepNumber() = default;

    protected:
      void control(float value) override;
    };

    class SAUNA360HumidityPercentNumber : public number::Number, public Parented<SAUNA360Component>
    {
    public:
      SAUNA360HumidityPercentNumber() = default;

    protected:
      void control(float value) override;
    };

  } // namespace sauna360
} // namespace esphome