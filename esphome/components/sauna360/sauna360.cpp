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

namespace esphome {
namespace sauna360 {

static const char *TAG = "sauna360";

void SAUNA360Component::setup() {
  if (this->light_relay_switch_ != nullptr) {
    this->light_relay_switch_->publish_state(false);
  }
  if (this->heater_relay_switch_ != nullptr) {
    this->heater_relay_switch_->publish_state(false);
  }
  if (this->light_relay_switch_ == nullptr) {
  }
  if (this->heater_relay_switch_ == nullptr) {
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

void SAUNA360Component::handle_byte_(uint8_t c) {
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

void SAUNA360Component::handle_frame_(std::vector<uint8_t> frame) {
  std::vector<uint8_t> packet;
  bool is_escaped = false;
  for (int i = 1; i < frame.size() - 1; i++) {
    uint8_t d = frame[i];
    if (d == 0x91) {
      is_escaped = true;
      continue;
    } else if (is_escaped) {
      is_escaped = false;
      if (d == 0x63) {
        d = 0x9C; // EOF
      } else if (d == 0x67) {
        d = 0x98; // SOF
      } else if (d == 0x6E) {
        d = 0x91; // ESC
      } else {
        ESP_LOGCONFIG(TAG, "Unknown escape sequence: %02X", d);
        packet.push_back(0x91);
      }
    }
    packet.push_back(d);
  }
  uint16_t crc = packet[packet.size() - 1];
  crc |= (packet[packet.size() - 2]) << 8;
  packet.pop_back();
  packet.pop_back();
  uint16_t crc_calculated = crc16be(packet.data(), packet.size(), 0xffff, 0x90d9, false, false);
  if (crc == crc_calculated) {
    this->handle_packet_(packet);
  } else {
    ESP_LOGCONFIG(TAG, "%s CRC ERROR", format_hex_pretty(packet).c_str());
  }
  frame.clear();
}

void SAUNA360Component::handle_packet_(std::vector<uint8_t> packet) {
  uint8_t address = packet[0];
  uint8_t packet_type = packet[1];
  uint16_t code = encode_uint16(packet[2], packet[3]);
  uint32_t data = encode_uint32(packet[4], packet[5], packet[6], packet[7]);

  if ((packet_type == 0x07) || (packet_type == 0x09)) {
    ESP_LOGCONFIG(TAG, "%s [ HEATER <-- PANEL ] CODE %04X DATA 0x%08X", format_hex_pretty(packet).c_str(), code, data);
    packet.clear();
    return;
  }

  ESP_LOGCONFIG(TAG, "%s [ HEATER --> PANEL ] CODE %04X DATA 0x%08X", format_hex_pretty(packet).c_str(), code, data);

  if (code == 0x3400) {
    std::string value;
    for (auto &listener : listeners_) {listener->on_light_status((data >> 3) & 1);}
    this->light_relay_switch_->publish_state((data >> 3) & 1);
    for (auto &listener : listeners_) {listener->on_heater_status((data >> 4) & 1);}
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
      value = "Idle";
      for (auto &listener : listeners_) {
        listener->on_heater_state(value);
      }
      this->heater_relay_switch_->publish_state((data >> 4) & 1);
    }
    if ((data >> 5) & 1){
      value = "Standby";
      for (auto &listener : listeners_) {listener->on_heater_state(value); }
    }
  }

  if (code == 0x3801) {
    // sensor data (ELITE TEMP/RH SENSOR)
  }

  else if (code == 0x4002) {
    int value = (data & 0xFFF);
    this->bath_time_received_hex_ = value;
    if ((value > 64) && (value < 124)) {
      value -= 4;
    } else if ((value >= 128) && (value < 188)) {
      value -= 8;
    } else if ((value >= 192) && (value < 252)) {
      value -= 12;
    } else if ((value >= 256) && (value < 316)) {
      value -= 16;
    } else if (value == 384) {
      value -= 24;
    } else if (value >= 230) {
      value -= 20;
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

  else if (code == 0x4003) {
    int overheating_pcb_limit = ((data >> 11) & 0x00007FF) / 18;
    this->overheating_pcb_limit_received_hex_ = ((data) & 0x007FFFFF);
    for (auto &listener : listeners_) {
      listener->on_overheating_pcb_limit(overheating_pcb_limit);
    }
    this->overheating_pcb_limit_number_->publish_state(overheating_pcb_limit);
  }

else if (code == 0x6000) {
    int actual_temp = (data & 0x00007FF) / 9.0;
    this->temperature_received_hex_ = (data & 0x00007FF);

    // actual temp -> climate
    for (auto &listener : listeners_) {
        listener->on_temperature(actual_temp);
    }

    int setpoint_temp = ((data >> 11) & 0x00007FF) / 9.0;
    this->setpoint_temperature_received_hex_ = ((data >> 11) & 0x00007FF);

    // only on change
    if (this->bath_temperature_number_ != nullptr) {
        if (this->bath_temperature_number_->state != setpoint_temp) {
            this->bath_temperature_number_->publish_state(setpoint_temp);
        }
    }

    // target temp -> listener
    for (auto &listener : listeners_) {
        listener->on_temperature_setting(setpoint_temp);
    }

    ESP_LOGCONFIG(TAG, "Updated Climate and Number component: Current Temperature = %d°C, Target Temperature = %d°C", actual_temp, setpoint_temp);
}

  else if (code == 0x7000){
    if (!this->state_changed_ && !this->heating_status_) {
      std::string value = "Operation blocked by not allowed start";
      for (auto &listener : listeners_) {listener->on_heater_state(value);}
    }
    this->state_changed_ = false;
  }
  else if (code == 0x7180) {
    bool light_state = (data & 0x00020000) != 0; // Bit 17 light state
    bool heater_state = (data & 0x0001C000) == 0x0001C000; // Bit 14-16 heater state

    ESP_LOGCONFIG(TAG, "0x7180 Received: Light: %s, Heater: %s", ONOFF(light_state), ONOFF(heater_state));

    // sync light switch state
    if (this->light_relay_switch_ != nullptr) {
        this->light_relay_switch_->publish_state(light_state);
    }
    
    std::string value = heater_state ? "On" : "Idle";

    for (auto &listener : listeners_) {
        listener->on_heater_state(value);
    }
}

  else if (code == 0x9400) {
    for (auto &listener : listeners_) {
      listener->on_total_uptime(data);
    }
  }

  else if (code == 0x9401) {
    for (auto &listener : listeners_) {
      listener->on_remaining_time(data);
    }
  }

  else if (code == 0xB000) {
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

  else if ((code & 0xFF00) == 0xB600) {
    if ((data & 0xF0000000) == 0x30000000) {
      std::string value = "Room temperature sensor not connected or malfunctioning";
      for (auto &listener : listeners_) {
        listener->on_heater_state(value);
      }
    }
    if ((data & 0xF0000000) == 0x10000000) {
      std::string value = "High temperature limit control have tripout, must be reset";
      for (auto &listener : listeners_) {
        listener->on_heater_state(value);
      }
    }
  }

  else {
    ESP_LOGCONFIG(TAG, "^^^^^^^^^^^^^^^^^^^^^^^ PACKET NOT HANDLED YET ");
  }

  packet.clear();
}

void SAUNA360Component::set_bath_time_number(float value) {
  if ((value > 60) && (value < 120)) {
    value += 4;
  } else if ((value >= 120) && (value < 180)) {
    value += 8;
  } else if ((value >= 180) && (value < 240)) {
    value += 12;
  } else if ((value >= 240) && (value < 300)) {
    value += 16;
  } else if (value >= 300) {
    value += 20;
  }
  uint32_t data = ((uint32_t)value);
  if (this->max_bath_temperature_received_hex_) {
    data |= (this->max_bath_temperature_received_hex_ << 20);
  } else {
    data |= (((uint32_t)this->max_bath_temperature_default_ * 18) << 20);
  }
  this->create_send_data_(0x07, 0x4002, data);
}

void SAUNA360Component::set_max_bath_temperature_number(float value) {
  uint32_t data = this->bath_time_received_hex_;
  data |= (((uint32_t)value * 18) << 20);
  this->create_send_data_(0x07, 0x4002, data);
}

void SAUNA360Component::set_bath_temperature_number(float value) {
  uint32_t data = 0;
  data |= (((uint32_t)value * 9) << 11);
  data |= this->temperature_received_hex_;
  this->create_send_data_(0x07, 0x6000, data);
}

void SAUNA360Component::set_overheating_pcb_limit_number(float value) {
  uint32_t data = (((uint32_t)value * 18) << 11);
  this->create_send_data_(0x07, 0x4003, data);
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
  ESP_LOGCONFIG(TAG, "CREATING SEND DATA TYPE:%s CODE:%s DATA:%s", format_hex_pretty(type).c_str(), format_hex_pretty(code).c_str(), format_hex_pretty(data).c_str());
  std::vector<uint8_t> packet;
  uint8_t id = 0x40;
  packet.push_back(id);
  packet.push_back(type);
  std::array<uint8_t, 2> code_array = decode_value(code);
  packet.push_back(code_array[0]);
  packet.push_back(code_array[1]);
  std::array<uint8_t, 4> data_array = decode_value(data);
  packet.push_back(data_array[0]);
  packet.push_back(data_array[1]);
  packet.push_back(data_array[2]);
  packet.push_back(data_array[3]);
  uint16_t crc_calculated = crc16be(packet.data(), packet.size(), 0xffff, 0x90d9, false, false);
  std::array<uint8_t, 2> crc_array = decode_value(crc_calculated);
  packet.push_back(crc_array[0]);
  packet.push_back(crc_array[1]);
  uint8_t eof = 0x9C;
  packet.insert(packet.begin(), 0x98); // SOF
  packet.push_back(eof);               // EOF
  this->tx_queue_.push(packet);
}

void SAUNA360Component::send_data_() {
    auto packet = std::move(this->tx_queue_.front());
    this->tx_queue_.pop();
    this->write_array(packet);
    this->flush();
}

void SAUNA360Component::dump_config(){
    ESP_LOGCONFIG(TAG, "UART component");
    ESP_LOGCONFIG(TAG, "Default Bath Time: %f", bath_time_default_);
}

}  // namespace sauna360
}  // namespace esphome
