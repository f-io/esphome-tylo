#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

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

#include <cmath>
#include <queue>
#include <string>
#include <vector>

namespace esphome {
namespace sauna360 {

class SAUNA360Listener {
public:
  virtual void on_temperature(uint16_t) {};
  virtual void on_temperature_setting(uint16_t) {};
  virtual void on_remaining_time(uint16_t) {};
  virtual void on_bath_time_setting(uint16_t) {};
  virtual void on_total_uptime(uint32_t) {};
  virtual void on_max_bath_temperature(uint16_t) {};
  virtual void on_overheating_pcb_limit(uint16_t) {};
  virtual void on_heater_status(bool) {};
  virtual void on_heater_state(const std::string &state) {};
  virtual void on_light_status(bool) {};
  virtual void on_ready_status(bool) {};
  virtual void on_setting_humidity_step(uint16_t) {};
  virtual void on_setting_humidity_percent(uint16_t) {};
  virtual void on_water_tank_level(uint16_t) {};
  virtual void on_session_uptime(uint32_t) {};
  virtual void on_session_uptime_text(const std::string &) {};
  int current_target_temperature = -1;
};

class SAUNA360Component : public uart::UARTDevice, public Component {

#ifdef USE_NUMBER
  SUB_NUMBER(bath_time)
  SUB_NUMBER(bath_temperature)
  SUB_NUMBER(max_bath_temperature)
  SUB_NUMBER(overheating_pcb_limit)
  SUB_NUMBER(humidity_step)
  SUB_NUMBER(humidity_percent)
#endif
#ifdef USE_SWITCH
  SUB_SWITCH(light_relay)
  SUB_SWITCH(heater_relay)
#endif

public:
  enum class Mode : uint8_t { PURE = 0, COMBI = 1, ELITE = 2, COMBI_ELITE = 3 };
  void set_mode(Mode m) { mode_ = m; }

  void setup() override;
  void loop() override;
  void dump_config() override;

  void initialize_defaults();
  void register_listener(SAUNA360Listener *lst) { listeners_.push_back(lst); }

  void set_bath_time_number(float value);
  void set_bath_time_default_value(float v) { bath_time_default_ = v; }

  void set_bath_temperature_number(float value);
  void set_bath_temperature_default_value(float v) {
    bath_temperature_default_ = v;
  }

  void set_max_bath_temperature_number(float value);
  void set_max_bath_temperature_default_value(float v) {
    max_bath_temperature_default_ = v;
  }

  void set_humidity_step_number(float value);
  void set_humidity_step_default_value(float v) { humidity_step_default_ = v; }

  void set_humidity_percent_number(float value);
  void set_humidity_percent_default_value(float v) {
    humidity_percent_default_ = v;
  }

  void set_light_relay(bool enable);
  void set_heater_relay(bool enable);

  void process_heater_status(uint32_t data);
  void process_bath_time(uint32_t data);
  void process_pcb_limit(uint32_t data);
  void process_temperature(uint32_t data);
  void process_heater_error(uint32_t data);
  void process_relay_bitmap(uint32_t data);
  void process_total_uptime(uint32_t data);
  void process_remaining_time(uint32_t data);
  void process_door_error(uint32_t data);
  void process_sensor_error(uint32_t data);
  void process_tank_level(uint32_t data);
  void process_humidity_control(uint32_t data);
  void process_datetime(uint32_t data);
  void process_time_limit(uint32_t data);

protected:
  Mode mode_ = Mode::PURE;
  esphome::HighFrequencyLoopRequester high_freq_;
  int min_ifg_us_ = 520;
  uint32_t LIGHT_MASK_{0x00020000};
  uint32_t COILS_MASK_{0x0001C000};
  int COILS_SHIFT_{14};

  std::vector<SAUNA360Listener *> listeners_{};
  std::vector<uint8_t> rx_message_;
  std::queue<std::vector<uint8_t>> tx_queue_;

  uint8_t decode_escape_sequence(uint8_t data);
  bool validate_packet(std::vector<uint8_t> &packet);
  void handle_byte_(uint8_t byte);
  void handle_packet_(std::vector<uint8_t> packet);
  void handle_frame_(const std::vector<uint8_t> &frame);
  void send_data_();
  void create_send_data_(uint8_t type, uint16_t code, uint32_t data);

  uint32_t temperature_received_hex_{0};
  uint32_t setpoint_temperature_received_hex_{0};
  uint32_t bath_time_received_hex_{0};
  uint32_t max_bath_temperature_received_hex_{0};
  uint32_t overheating_pcb_limit_received_hex_{0};
  uint32_t humidity_received_hex_{0};
  uint32_t bath_type_priority_received_hex_{0};
  int last_humidity_step_target_{-1};
  uint32_t last_humidity_step_set_ms_{0};
  static constexpr int HUM_STEP_BASE = 40;
  static constexpr int HUM_STEP_SCALE = 8;

  bool frame_flag_{false};
  bool state_changed_{false};
  bool heating_status_{false};
  bool defaults_initialized_{false};

  bool relays_known_{false};
  bool last_light_on_{false};
  bool last_heater_on_{false};

  uint32_t last_light_cmd_ms_{0};
  uint32_t last_heater_cmd_ms_{0};

  float bath_time_default_{NAN};
  float bath_temperature_default_{NAN};
  float max_bath_temperature_default_{NAN};
  float humidity_step_default_{NAN};
  float humidity_percent_default_{NAN};

  int last_bath_time_target_{-1};
  uint32_t last_bath_time_set_ms_{0};

  int decode_bath_time_minutes_(uint16_t raw12) const;
  int encode_bath_time_raw_(int target_minutes) const;

  bool session_active_{false};
  uint32_t session_start_ms_{0};
  uint32_t session_frozen_s_{0};
  uint32_t last_session_pub_ms_{0};

  void publish_session_();
};

} // namespace sauna360
} // namespace esphome
