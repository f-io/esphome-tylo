#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "esphome/core/preferences.h"
#include "esphome/core/automation.h"
#include "esphome/core/time.h"
#include "sauna360.h"

namespace esphome
{
  namespace sauna360
  {

    static const char *TAG = "sauna360";

    void SAUNA360Component::setup()
    {
      if (this->light_relay_switch_ != nullptr) {
        this->light_relay_switch_->publish_state(false);
      }
      if (this->heater_relay_switch_ != nullptr) {
        this->heater_relay_switch_->publish_state(false);
      }
      if (this->bath_temperature_number_ != nullptr) {
        this->bath_temperature_number_->publish_state(0.0);
      }
      for (auto &listener : listeners_) {
        listener->on_ready_status(true);
      }
      this->high_freq_.start(); // start high frequency loop
    }

    void SAUNA360Component::loop() {
      while (this->available()) {
        uint8_t byte;
        this->read_byte(&byte);

        // sequence check for (0x98, 0x40) 0x6D, 0x3A, 0x9C
        bool is_sequence_match = (byte == 0x9C && this->last_bytes_[1] == 0x3A && this->last_bytes_[0] == 0x6D);
        bool is_tx_ready = (!this->tx_queue_.empty());

        if (is_sequence_match && is_tx_ready) {
          send_data_();
          return;
        }

        this->handle_byte_(byte);

        this->last_bytes_[0] = this->last_bytes_[1];
        this->last_bytes_[1] = byte;
      }
    }

    void SAUNA360Component::handle_byte_(uint8_t c){
      if (c == 0x98) {
        this->frame_flag_ = true;
      }
      if (c == 0x98 && this->frame_flag_ == true) {
        this->rx_message_.clear();
      }
      if (c == 0x9C) {
        this->rx_message_.push_back(c);
        std::vector<uint8_t> frame(this->rx_message_.begin(), this->rx_message_.end());
        size_t len = frame.size();
        if (len > 6) {
          this->handle_frame_(frame);
        }
        this->rx_message_.clear();
        this->frame_flag_ = false;
        return;
      }
      if (this->frame_flag_ == true) {
        this->rx_message_.push_back(c);
      }
    }

    void SAUNA360Component::handle_frame_(const std::vector<uint8_t> &frame) {
      std::vector<uint8_t> packet;
      bool is_escaped = false;

      for (size_t i = 1; i < frame.size() - 1; ++i) {
        uint8_t data = frame[i];

        if (data == 0x91) {
            is_escaped = true;
            continue;
        }

        if (is_escaped) {
            data = decode_escape_sequence(data);
            is_escaped = false;
        }

        packet.push_back(data);
      }

      if (!validate_packet(packet)) {
        ESP_LOGI(TAG, "Invalid packet size or CRC error");
        return;
      }

      this->handle_packet_(packet);
    }

    uint8_t SAUNA360Component::decode_escape_sequence(uint8_t data) {
      switch (data) {
        case 0x63: return 0x9C; // EOF
        case 0x67: return 0x98; // SOF
        case 0x6E: return 0x91; // ESC
        default:
            ESP_LOGI(TAG, "Unknown escape sequence: %02X", data);
            return 0x91; // Return original escape character for unknown sequences
      }
    }

    bool SAUNA360Component::validate_packet(std::vector<uint8_t> &packet) {
      if (packet.size() < 2) {
        return false;
      }

      uint16_t crc = (packet.end()[-2] << 8) | packet.end()[-1];
      packet.resize(packet.size() - 2);

      uint16_t calculated_crc = crc16be(packet.data(), packet.size(), 0xFFFF, 0x90D9, false, false);
      if (crc != calculated_crc) {
        std::string packet_str;
        for (size_t i = 0; i < packet.size(); ++i) {
          char buffer[4];
          snprintf(buffer, sizeof(buffer), " %02X", packet[i]);
          packet_str += buffer;
        }
        ESP_LOGI(TAG, "CRC ERROR: Expected %04X, got %04X. Full packet:[%s]", crc, calculated_crc, packet_str.c_str());
        return false;
      }
      return true;
    }

    void SAUNA360Component::handle_packet_(std::vector<uint8_t> packet) {
      uint8_t address = packet[0];
      uint8_t packet_type = packet[1];
      uint16_t code = encode_uint16(packet[2], packet[3]);
      uint32_t data = encode_uint32(packet[4], packet[5], packet[6], packet[7]);

      if ((packet_type == 0x07) || (packet_type == 0x09)) {
        ESP_LOGD(TAG, "%s [ HEATER <-- PANEL ] CODE %04X DATA 0x%08X", format_hex_pretty(packet).c_str(), code, data);
        packet.clear();
        return;
      }

      ESP_LOGD(TAG, "%s [ HEATER --> PANEL ] CODE %04X DATA 0x%08X", format_hex_pretty(packet).c_str(), code, data);
      switch (code) {
        case 0x3400:
          this->process_heater_status(data);
          break;
        case 0x3801:
        // temperature and humidity sensor data (Combi/Elite)
          break;
        case 0x4002:
          this->process_bath_time(data);
          break;
        case 0x4003:
          this->process_pcb_limit(data);
          break;
        case 0x6000:
          this->process_temperature(data);
          break;
        case 0x7000:
          this->process_heater_error(data);
          break;
        case 0x7180:
          this->process_light_heater(data);
          break;
        case 0x9400:
          this->process_total_uptime(data);
          break;
        case 0x9401:
          this->process_remaining_time(data);
          break;
        case 0xB000:
          this->process_door_error(data);
          break;
        default:
          if ((code & 0xFF00) == 0xB600) {
           this->process_sensor_error(data);
          }
          else {
            ESP_LOGD(TAG, "Unhandled packet code: %04X", code);
          }
          break;
      }
    }  

    void SAUNA360Component::process_heater_status(uint32_t data) {
      std::string value;
      for (auto &listener : listeners_) {
        listener->on_light_status((data >> 3) & 1);
      }
      // sync light switch
      this->light_relay_switch_->publish_state((data >> 3) & 1);

      for (auto &listener : listeners_) {
        listener->on_heater_status((data >> 4) & 1);
      }
      // sync heater switch
      this->heater_relay_switch_->publish_state((data >> 4) & 1);

      this->state_changed_ = true;
      this->heating_status_ = ((data >> 4) & 1);

      if ((data >> 4) & 1) {
        for (auto &listener : listeners_) {
          listener->on_ready_status(false);
        }
        value = "On";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
      }
      else {
        value = "Standby";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
      }
      if ((data >> 5) & 1) {
        value = "Standby";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
      }
    }

    void SAUNA360Component::process_bath_time(uint32_t data) {
      int value = (data & 0xFFF);
      this->bath_time_received_hex_ = value;

      const std::vector<int> adjustments = {0, 0, -4, -8, -12, -16, -20, -24};
      int index = (value / 64) - 1;
      if (index >= 0 && index < adjustments.size()) {
        value += adjustments[index];
      }

      for (auto &listener : listeners_) {
        listener->on_bath_time_setting(value);
      }
      this->bath_time_number_->publish_state(value);

      int max_bath_temperature = ((data >> 20) & 0x00FFFFF) / 18;
      this->max_bath_temperature_received_hex_ = ((data >> 20) & 0x00FFFFF);

      for (auto &listener : listeners_) {
        listener->on_max_bath_temperature(max_bath_temperature);
      }
      this->max_bath_temperature_number_->publish_state(max_bath_temperature);
    }

    void SAUNA360Component::process_pcb_limit(uint32_t data) {
      int overheating_pcb_limit = ((data >> 11) & 0x00007FF) / 18;
      this->overheating_pcb_limit_received_hex_ = ((data) & 0x007FFFFF);
      for (auto &listener : listeners_) {
        listener->on_overheating_pcb_limit(overheating_pcb_limit);
      }
    }

    void SAUNA360Component::process_temperature(uint32_t data) {
      int actual_temp = (data & 0x00007FF) / 9.0;
      this->temperature_received_hex_ = (data & 0x00007FF);
  
      for (auto &listener : listeners_) {
        listener->on_temperature(actual_temp);
      }
  
      int setpoint_temp = ((data >> 11) & 0x00007FF) / 9.0;
      this->setpoint_temperature_received_hex_ = ((data >> 11) & 0x00007FF);

      if (this->bath_temperature_number_ != nullptr) {
        if (this->bath_temperature_number_->state != setpoint_temp) {
          this->bath_temperature_number_->publish_state(setpoint_temp);
        }
      }

      for (auto &listener : listeners_) {
        if (listener->current_target_temperature != setpoint_temp) {
          listener->on_temperature_setting(setpoint_temp); 
          listener->current_target_temperature = setpoint_temp;
        }
      }

      ESP_LOGI(TAG, "Temperature = %d°C, Target Temperature = %d°C", actual_temp, setpoint_temp);
    }

    void SAUNA360Component::process_heater_error(uint32_t data) {
      if (!this->state_changed_ && !this->heating_status_) {
        std::string value = "Operation blocked by not allowed start";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
      }
      this->state_changed_ = false;
    }

    void SAUNA360Component::process_light_heater(uint32_t data) {
      bool light_state = (data & 0x00020000) != 0;           // Bit 17 light state
      bool heater_state = (data & 0x0001C000) == 0x0001C000; // Bit 14-16 heater state
      ESP_LOGI(TAG, "Light: %s, Heater Status: %s", ONOFF(light_state), heater_state ? "Heating" : "Standby");;
      std::string value = heater_state ? "Heating" : "Standby";
      for (auto &listener : listeners_) {
        listener->on_heater_state(value);
      }
    }

    void SAUNA360Component::process_total_uptime(uint32_t data) {
      for (auto &listener : listeners_) {
        listener->on_total_uptime(data);
      }
      ESP_LOGI(TAG, "Total uptime: %d minutes", data);

      if (!this->oven_ready_) {
          this->oven_ready_ = true;
          ESP_LOGI(TAG, "Oven is now ready to receive data.");
      } 
      if (this->oven_ready_ && !this->defaults_initialized_) {
            initialize_defaults();
      }   
    }

    void SAUNA360Component::process_remaining_time(uint32_t data) {
      for (auto &listener : listeners_) {
        listener->on_remaining_time(data);
      }
      ESP_LOGI(TAG, "Remaining time: %d minutes", data);
    }

    void SAUNA360Component::process_door_error(uint32_t data) {
      for (auto &listener : listeners_) {
        listener->on_ready_status((~data) & 1);
      }
      if (data == 0x00060001) {
        std::string value = "Operation blocked by not allowed start";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
        this->create_send_data_(0x07, 0xB000, 0x00060101);
      }
      if (data == 0x00130001) {
        std::string value = "Operation blocked by not allowed start";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
        this->create_send_data_(0x07, 0xB000, 0x00130101);
      }
      if (data == 0x00130003) {
        std::string value = "Door opened too long, bath cancelled";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
        this->create_send_data_(0x07, 0xB000, 0x00130103);
      }
      if (data == 0x00140003) {
        std::string value = "Door has been open, check sauna";
        for (auto &listener : listeners_) {
          listener->on_heater_state(value);
        }
        this->create_send_data_(0x07, 0xB000, 0x00140000);
      }
    }

    void SAUNA360Component::process_sensor_error(uint32_t data) {
      std::string error_message;
      if ((data & 0xF0000000) == 0x30000000) {
        error_message = "Room temperature sensor not connected or malfunctioning";
      }
      else if ((data & 0xF0000000) == 0x10000000) {
        error_message = "High temperature limit control tripped, must be reset";
      }

      if (!error_message.empty()) {
        for (auto &listener : listeners_) {
          listener->on_heater_state(error_message);
        }
        ESP_LOGI(TAG, "Sensor error: %s", error_message.c_str());
      }
    }

    void SAUNA360Component::set_bath_time_number(float value) {
      const int BATH_TIME_OFFSETS[] = {4, 8, 12, 16, 20};
      const float BATH_TIME_RANGES[] = {60, 120, 180, 240, 300};
    
      for (size_t i = 0; i < 4; ++i) {
        if (value > BATH_TIME_RANGES[i] && value < BATH_TIME_RANGES[i + 1]) {
            value += BATH_TIME_OFFSETS[i];
          break;
        }
      }

      if (value >= 300) {
        value += BATH_TIME_OFFSETS[4];
      }

      uint32_t data = static_cast<uint32_t>(value);
      if (this->max_bath_temperature_received_hex_) {
        data |= (this->max_bath_temperature_received_hex_ << 20);
      } else {
        data |= (static_cast<uint32_t>(this->max_bath_temperature_default_ * 18) << 20);
      }

      this->create_send_data_(0x07, 0x4002, data);
      ESP_LOGI(TAG, "SENT: Bath time: %.0f minutes", value);
    }

    void SAUNA360Component::set_max_bath_temperature_number(float value) {
      uint32_t data = this->bath_time_received_hex_;
      data |= (((uint32_t)value * 18) << 20);
      this->create_send_data_(0x07, 0x4002, data);
      ESP_LOGI(TAG, "SENT: Max bath temperature: %.0f°C", value);
    }

    void SAUNA360Component::set_bath_temperature_number(float value) {
      float current_set_temp = ((this->setpoint_temperature_received_hex_ & 0x00007FF) / 9.0);
      // homekit allow .5° values
      value = round(value);
      if (value != current_set_temp) {
        uint32_t data = 0;
        data |= (((uint32_t)value * 9) << 11);
        data |= this->temperature_received_hex_;
        this->create_send_data_(0x07, 0x6000, data);
        ESP_LOGI(TAG, "SENT: Bath temperature: %.0f°C", value);
      }
    }

    void SAUNA360Component::set_light_relay(bool enable) {
      // Light toggle command
      this->create_send_data_(0x07, 0x7000, enable ? 0x2 : 0x2);
    }

    void SAUNA360Component::set_heater_relay(bool enable) {
      // Heater toggle command
      this->create_send_data_(0x07, 0x7000, enable ? 0x1 : 0x1);
    }
    

    void SAUNA360Component::create_send_data_(uint8_t type, uint16_t code, uint32_t data) {
      ESP_LOGD(TAG, "CREATING SEND DATA TYPE:%s CODE:%s DATA:%s",
                    format_hex_pretty(type).c_str(),
                    format_hex_pretty(code).c_str(),
                    format_hex_pretty(data).c_str());

      std::vector<uint8_t> packet = {0x40, type};
      std::array<uint8_t, 2> code_array = decode_value(code);
      std::array<uint8_t, 4> data_array = decode_value(data);

      packet.insert(packet.end(), code_array.begin(), code_array.end());
      packet.insert(packet.end(), data_array.begin(), data_array.end());

      uint16_t crc_calculated = crc16be(packet.data(), packet.size(), 0xffff, 0x90d9, false, false);
      std::array<uint8_t, 2> crc_array = decode_value(crc_calculated);
      packet.insert(packet.end(), crc_array.begin(), crc_array.end());

      packet.insert(packet.begin(), 0x98); // SOF
      packet.push_back(0x9C);              // EOF

      this->tx_queue_.push(packet);
    }

    void SAUNA360Component::send_data_() {
      auto packet = std::move(this->tx_queue_.front());
      this->tx_queue_.pop();
      this->write_array(packet);
      this->flush();
    }

    void SAUNA360Component::initialize_defaults() {
      ESP_LOGI(TAG, "=========== Initialized to default values ===========");
      ESP_LOGI(TAG, "Max bath temperature: %.0f°C", max_bath_temperature_default_);
      ESP_LOGI(TAG, "Bath time: %.0f minutes", bath_time_default_);
      ESP_LOGI(TAG, "Bath temperature: %.0f°C", bath_temperature_default_);
      ESP_LOGI(TAG, "=======================================================");
      if (!std::isnan(this->max_bath_temperature_default_)) {
        this->set_max_bath_temperature_number(max_bath_temperature_default_);
      }
      if (!std::isnan(this->bath_time_default_)) {
        this->set_bath_time_number(bath_time_default_);
      }
      if (!std::isnan(this->bath_temperature_default_)) {
        this->set_bath_temperature_number(bath_temperature_default_);
      }
      this->defaults_initialized_ = true;
    }

    void SAUNA360Component::dump_config() {
      ESP_LOGCONFIG(TAG, "UART component");
    }

  } // namespace sauna360
} // namespace esphome
