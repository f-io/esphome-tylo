#pragma once

#include "esphome/components/switch/switch.h"
#include "../sauna360.h"

namespace esphome
{
  namespace sauna360
  {

    class SAUNA360LightRelaySwitch : public switch_::Switch, public Parented<SAUNA360Component>
    {
    public:
      SAUNA360LightRelaySwitch() = default;

    protected:
      void write_state(bool state) override;
    };

    class SAUNA360HeaterRelaySwitch : public switch_::Switch, public Parented<SAUNA360Component>
    {
    public:
      SAUNA360HeaterRelaySwitch() = default;

    protected:
      void write_state(bool state) override;
    };

  } // namespace sauna360
} // namespace esphome