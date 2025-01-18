#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"
#include "esphome/core/gpio.h"
#include "esphome/core/application.h" // high frequency loop

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_DATETIME
#include "esphome/components/datetime/datetime_entity.h"
#endif

#include <queue>
#include <map>
#include <functional>

namespace esphome {
namespace sauna360 {

class SAUNA360Listener {
 public:
   virtual void on_temperature(uint16_t temperature) {};
   virtual void on_temperature_setting(uint16_t temperature_setting) {};
   virtual void on_remaining_time(uint16_t remaining_time) {};
   virtual void on_bath_time_setting(uint16_t bath_time_setting) {};
   virtual void on_total_uptime(uint16_t bath_time_setting) {};
   virtual void on_max_bath_temperature(uint16_t max_bath_temperature) {};
   virtual void on_overheating_pcb_limit(uint16_t overheating_pcb_limit) {};
   virtual void on_heater_status(bool heater_status) {};
   virtual void on_heater_state(std::string &fw) {};
   virtual void on_light_status(bool light_status) {};
   virtual void on_ready_status(bool ready_status) {};
   int current_target_temperature = -1; 
};

class SAUNA360Component : public uart::UARTDevice, public Component {

  #ifdef USE_NUMBER
    SUB_NUMBER(bath_time)
    SUB_NUMBER(bath_temperature)
    SUB_NUMBER(max_bath_temperature)
    SUB_NUMBER(overheating_pcb_limit)
  #endif
  #ifdef USE_SWITCH
    SUB_SWITCH(light_relay)
    SUB_SWITCH(heater_relay)
  #endif

  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    void initialize_defaults();
    void register_listener(SAUNA360Listener *listener) { this->listeners_.push_back(listener); }
    void set_bath_time_number(float value);
    void set_bath_time_default_value(float bath_time_default) { bath_time_default_ = bath_time_default; }
    void set_bath_temperature_number(float value);
    void set_bath_temperature_default_value(float bath_temperature_default) { bath_temperature_default_ = bath_temperature_default; }
    void set_max_bath_temperature_number(float value);
    void set_max_bath_temperature_default_value(float max_bath_temperature_default) { max_bath_temperature_default_ = max_bath_temperature_default; }
    void set_light_relay(bool enable);
    void set_heater_relay(bool enable);
    void process_heater_status(uint32_t data);
    void process_bath_time(uint32_t data);
    void process_pcb_limit(uint32_t data);
    void process_temperature(uint32_t data);
    void process_heater_error(uint32_t data);
    void process_light_heater(uint32_t data);
    void process_total_uptime(uint32_t data);
    void process_remaining_time(uint32_t data);
    void process_door_error(uint32_t data);
    void process_sensor_error(uint32_t data);

  protected:
    esphome::HighFrequencyLoopRequester high_freq_;
    std::vector<SAUNA360Listener *> listeners_{};
    std::vector<uint8_t> rx_message_;
    std::queue<std::vector<uint8_t>> tx_queue_;
    uint8_t  last_bytes_[2] = {0, 0};
    uint8_t decode_escape_sequence(uint8_t data);  
    uint32_t temperature_received_hex_;
    uint32_t setpoint_temperature_received_hex_;
    uint32_t bath_time_received_hex_;
    uint32_t max_bath_temperature_received_hex_;
    uint32_t overheating_pcb_limit_received_hex_;
    bool frame_flag_;
    bool state_changed_;
    bool heating_status_;
    bool defaults_initialized_ = false;
    float bath_time_default_{NAN};
    float bath_temperature_default_{NAN};
    float max_bath_temperature_default_{NAN};
    bool validate_packet(std::vector<uint8_t> &packet); 
    void handle_byte_(uint8_t byte);
    void handle_packet_(std::vector<uint8_t> packet);
    void handle_frame_(const std::vector<uint8_t> &frame);
    void send_data_();
    void create_send_data_(uint8_t type, uint16_t code, uint32_t data);
};

}  // namespace sauna360
}  // namespace esphome
