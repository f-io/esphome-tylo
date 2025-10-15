#include "sauna360.h"
#include "esphome/components/number/number.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/core/application.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include "esphome/core/time.h"

#include "driver/uart.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace esphome {
namespace sauna360 {

static const char *TAG = "sauna360";

void SAUNA360Component::setup() {
  this->min_ifg_us_ = (this->mode_ == Mode::PURE) ? 520 : 7000;

  const char *mode_str = nullptr;
  switch (this->mode_) {
  case Mode::PURE:
    mode_str = "PURE";
    break;
  case Mode::COMBI:
    mode_str = "COMBI";
    break;
  case Mode::ELITE:
    mode_str = "ELITE";
    break;
  case Mode::COMBI_ELITE:
    mode_str = "COMBI_ELITE";
    break;
  default:
    mode_str = "PURE";
    break;
  }

  ESP_LOGI(TAG, "IFG selected: %d us (%s)", this->min_ifg_us_, mode_str);

  // Model-specific bitmasks
  // PURE
  if (this->mode_ == Mode::PURE) {
    this->LIGHT_MASK_ = 0x00020000; // bit 17
    this->COILS_MASK_ = 0x0001C000; // bits 16..14
    this->COILS_SHIFT_ = 14;
    // COMBI, ELITE, COMBI_ELITE
  } else {
    this->LIGHT_MASK_ = 0x00000020; // bit 5
    this->COILS_MASK_ = 0x00000007; // bits 2..0
    this->COILS_SHIFT_ = 0;
  }
  ESP_LOGI(TAG, "Relay bitmasks: LIGHT=0x%08X COILS=0x%08X (shift=%d)",
           (unsigned)this->LIGHT_MASK_, (unsigned)this->COILS_MASK_,
           this->COILS_SHIFT_);

  // Initial UI state
  if (this->light_relay_switch_ != nullptr)
    this->light_relay_switch_->publish_state(false);
  if (this->heater_relay_switch_ != nullptr)
    this->heater_relay_switch_->publish_state(false);
  if (this->bath_temperature_number_ != nullptr)
    this->bath_temperature_number_->publish_state(0.0f);
  for (auto &l : listeners_)
    l->on_ready_status(true);

  // Keep UART hot
  this->high_freq_.start();

  // UART low-latency
  static constexpr uart_port_t PORT = UART_NUM_0;
  uart_set_rx_full_threshold(PORT, 1);
  uart_set_rx_timeout(PORT, 1);

  TaskHandle_t rx_task = nullptr;
#if CONFIG_FREERTOS_UNICORE
  const BaseType_t core_id = 0;
#else
  const BaseType_t core_id = 1;
#endif

  // RX/flow-control task
  xTaskCreatePinnedToCore(
      [](void *ctx) {
        auto *self = static_cast<SAUNA360Component *>(ctx);
        static constexpr uart_port_t PORT = UART_NUM_0;

        uint8_t b = 0, w0 = 0, w1 = 0, w2 = 0, w3 = 0, w4 = 0, w5 = 0;

        for (;;) {
          int n = uart_read_bytes(PORT, &b, 1, portMAX_DELAY);
          if (n != 1)
            continue;

          // Detect panel EOF: 98 40 07 FD E3 9C
          w0 = w1;
          w1 = w2;
          w2 = w3;
          w3 = w4;
          w4 = w5;
          w5 = b;
          const bool panel_eof = (w0 == 0x98 && w1 == 0x40 && w2 == 0x07 &&
                                  w3 == 0xFD && w4 == 0xE3 && w5 == 0x9C);

          if (panel_eof && !self->tx_queue_.empty()) {
            esp_rom_delay_us(self->min_ifg_us_);
            size_t rx_avail = 0;
            (void)uart_get_buffered_data_len(PORT, &rx_avail);
            if (rx_avail == 0)
              self->send_data_();
          }

          self->handle_byte_(b);
        }
      },
      "sauna_rx_fast", 4096, this, configMAX_PRIORITIES - 3, &rx_task, core_id);

  if (!this->defaults_initialized_) {
    this->initialize_defaults();
    this->defaults_initialized_ = true;
  }
}

void SAUNA360Component::loop() {
  const uint32_t now = millis();
  if ((now - this->last_session_pub_ms_) >= 1000u) {
    if (this->session_active_) {
      this->publish_session_();
    }
    this->last_session_pub_ms_ = now;
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
    std::vector<uint8_t> frame(this->rx_message_.begin(),
                               this->rx_message_.end());
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
  case 0x63:
    return 0x9C; // EOF
  case 0x67:
    return 0x98; // SOF
  case 0x6E:
    return 0x91; // ESC
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

  uint16_t calculated_crc =
      crc16be(packet.data(), packet.size(), 0xFFFF, 0x90D9, false, false);
  if (crc != calculated_crc) {
    std::string packet_str;
    for (size_t i = 0; i < packet.size(); ++i) {
      char buffer[4];
      snprintf(buffer, sizeof(buffer), " %02X", packet[i]);
      packet_str += buffer;
    }
    ESP_LOGI(TAG, "CRC ERROR: Expected %04X, got %04X. Full packet:[%s]", crc,
             calculated_crc, packet_str.c_str());
    return false;
  }
  return true;
}

void SAUNA360Component::handle_packet_(std::vector<uint8_t> packet) {
  if (packet.size() < 8) {
    ESP_LOGW(TAG, "Packet too short: %u", (unsigned)packet.size());
    return;
  }
  uint8_t packet_type = packet[1];
  uint16_t code = encode_uint16(packet[2], packet[3]);
  uint32_t data = encode_uint32(packet[4], packet[5], packet[6], packet[7]);

  const bool from_panel = (packet_type == 0x07) || (packet_type == 0x09);

  if (from_panel) {
    ESP_LOGD(TAG, "%s [ HEATER <-- PANEL ] CODE %04X DATA 0x%08X",
             format_hex_pretty(packet).c_str(), code, data);
    packet.clear();
    return;
  }

  ESP_LOGD(TAG, "%s [ HEATER --> PANEL ] CODE %04X DATA 0x%08X",
           format_hex_pretty(packet).c_str(), code, data);

  switch (code) {
  case 0x3400:
    this->process_heater_status(data);
    break;
  case 0x3801: /* combi sensors */
    break;
  case 0x4002:
    this->process_bath_time(data);
    break;
  case 0x4003:
    this->process_pcb_limit(data);
    break;
  case 0x4200:
    this->process_datetime(data);
    break;
  case 0x5200: /* relay */
    break;
  case 0x5201: /* relay */
    break;
  case 0x5202: /* relay */
    break;
  case 0x6000:
    this->process_temperature(data);
    break;
  case 0x6001:
    this->process_humidity_control(data);
    break;
  case 0x7000:
    this->process_heater_error(data);
    break;
  case 0x7180:
    this->process_relay_bitmap(data);
    break;
  case 0x7280:
    this->process_tank_level(data);
    break;
  case 0x9000:
    this->process_time_limit(data);
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
    if ((code & 0xFF00) == 0xB600)
      this->process_sensor_error(data);
    else
      ESP_LOGD(TAG, "Unhandled packet code: %04X", code);
    break;
  }
}

void SAUNA360Component::process_heater_status(uint32_t data) {
  this->heating_status_ = ((data >> 4) & 1); // "enabled"
  this->state_changed_ = true;
  if (this->heating_status_) {
    for (auto &l : listeners_)
      l->on_ready_status(false);
  }
}

// Raw 12-bit -> minutes (bucketed)
int SAUNA360Component::decode_bath_time_minutes_(uint16_t raw12) const {
  int raw = static_cast<int>(raw12);
  if (raw < 0)
    raw = 0;
  if (raw > 4095)
    raw = 4095;

  if (raw < 64)
    return raw;

  int k = (raw / 64) - 1; // 0..7 for raw >= 64
  if (k < 0)
    k = 0;
  if (k > 7)
    k = 7;
  int offset = 4 * (k + 1); // 4,8,12,16,20,24,28,32

  int minutes = raw - offset;
  if (minutes < 0)
    minutes = 0;
  if (minutes > 575)
    minutes = 575;
  return minutes;
}

// minutes -> Raw 12-bit (inverse of above)
int SAUNA360Component::encode_bath_time_raw_(int target_minutes) const {
  int t = target_minutes;
  if (t < 0)
    t = 0;
  if (t > 575)
    t = 575;

  if (t <= 63)
    return t;

  int k = (t / 64) - 1; // 0..7 for t >= 64
  if (k < 0)
    k = 0;
  if (k > 7)
    k = 7;
  int offset = 4 * (k + 1);

  int raw = t + offset;
  // Raw field is 12-bit in low bits of 0x4002
  if (raw < 0)
    raw = 0;
  if (raw > 4095)
    raw = 4095;
  return raw;
}

void SAUNA360Component::process_bath_time(uint32_t data) {
  const uint16_t raw = static_cast<uint16_t>(data & 0x0FFF);
  this->bath_time_received_hex_ = raw;

  const int decoded = this->decode_bath_time_minutes_(raw);

  // Snap logic
  constexpr uint32_t SNAP_WINDOW_MS = 1000; // 1s
  constexpr int SNAP_TOL_MIN = 8;           // ±8 min

  int to_publish = decoded;

  const bool have_intent = (this->last_bath_time_target_ >= 0);
  const uint32_t dt = millis() - this->last_bath_time_set_ms_;
  const bool within_window = have_intent && (dt <= SNAP_WINDOW_MS);
  const bool close_to_target =
      have_intent &&
      (std::abs(decoded - this->last_bath_time_target_) <= SNAP_TOL_MIN);

  const bool heater_on = this->heating_status_ || this->last_heater_on_;

  if (!heater_on && !(within_window || close_to_target)) {
    if (have_intent) {
      to_publish = this->last_bath_time_target_;
    } else {
    }
  } else if (within_window || close_to_target) {
    to_publish = this->last_bath_time_target_;
  }

  // Publish
  for (auto &listener : listeners_)
    listener->on_bath_time_setting(to_publish);

  if (this->bath_time_number_ != nullptr) {
    if (this->bath_time_number_->state != static_cast<float>(to_publish)) {
      this->bath_time_number_->publish_state(static_cast<float>(to_publish));
    }
  }

  // High bits: max bath temperature (12 Bit)
  const uint32_t hi = (data >> 20) & 0xFFF;
  const int max_bath_temperature = static_cast<int>(hi / 18);
  this->max_bath_temperature_received_hex_ = hi;

  for (auto &listener : listeners_)
    listener->on_max_bath_temperature(max_bath_temperature);

  if (this->max_bath_temperature_number_ != nullptr)
    this->max_bath_temperature_number_->publish_state(
        static_cast<float>(max_bath_temperature));

  ESP_LOGI(TAG, "Bath time RX: raw=%u -> decoded=%d, published=%d%s%s",
           static_cast<unsigned>(raw), decoded, to_publish,
           (to_publish != decoded) ? " (snap)" : "",
           (!heater_on && !(within_window || close_to_target))
               ? " (held while OFF)"
               : "");
}

void SAUNA360Component::process_pcb_limit(uint32_t data) {
  int overheating_pcb_limit = ((data >> 11) & 0x00007FF) / 18;
  this->overheating_pcb_limit_received_hex_ = ((data) & 0x007FFFFF);
  for (auto &listener : listeners_) {
    listener->on_overheating_pcb_limit(overheating_pcb_limit);
  }
}

void SAUNA360Component::process_datetime(uint32_t data) {
  int minute = (data & 0x3F);
  int hour = ((data >> 6) & 0x1F);
  int day = ((data >> 12) & 0x1F);
  int month = ((data >> 17) & 0x0F);
  int year = ((data >> 21) & 0x1F) + 2000;

  ESP_LOGI(TAG, "Datetime from heater: %04d-%02d-%02d %02d:%02d", year, month,
           day, hour, minute);
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

  ESP_LOGI(TAG, "Temperature = %d°C, Target Temperature = %d°C", actual_temp,
           setpoint_temp);
}

void SAUNA360Component::process_humidity_control(uint32_t data) {
  // Cache context for future TX on 0x6001
  this->humidity_received_hex_ = data;
  if ((data & 0xF0000000) == 0) {
    // Step mode: keep priority bits
    this->bath_type_priority_received_hex_ = (data & 0x0000F000);
  } else {
    // Percent mode: keep high nibble + priority + flags per observed frames
    this->bath_type_priority_received_hex_ = (data & 0xF000C000);
  }

  if ((data & 0xF0000000) == 0) {
    // --- STEP MODE (0..10) ---
    int raw = static_cast<int>((data >> 4) & 0xFF);
    int step = (raw - HUM_STEP_BASE) / HUM_STEP_SCALE;
    if (step < 0)
      step = 0;
    if (step > 10)
      step = 10;

    for (auto &listener : listeners_)
      listener->on_setting_humidity_step(static_cast<uint16_t>(step));

#ifdef USE_NUMBER
    if (this->humidity_step_number_ != nullptr) {
      if (this->humidity_step_number_->state != static_cast<float>(step))
        this->humidity_step_number_->publish_state(static_cast<float>(step));
    }
#endif
    ESP_LOGI(TAG, "Humidity step: %d", step);
  } else {
    // --- PERCENT MODE (0..63 encoded) ---
    int target_pct = static_cast<int>((data >> 7) & 0x3F);
    int current_pct = static_cast<int>(data & 0x7F);
    if (target_pct < 0)
      target_pct = 0;
    if (target_pct > 100)
      target_pct = 100;

    for (auto &listener : listeners_)
      listener->on_setting_humidity_percent(static_cast<uint16_t>(target_pct));

#ifdef USE_NUMBER
    if (this->humidity_percent_number_ != nullptr) {
      if (this->humidity_percent_number_->state !=
          static_cast<float>(target_pct))
        this->humidity_percent_number_->publish_state(
            static_cast<float>(target_pct));
    }
#endif
    ESP_LOGI(TAG, "Humidity: target %d%%, current %d%%", target_pct,
             current_pct);
  }

  int pr = static_cast<int>((data >> 14) & 0x3);
  const char *priority = (pr == 0)   ? "Automatic"
                         : (pr == 1) ? "Temperature"
                         : (pr == 2) ? "Humidity"
                                     : "Unknown";
  ESP_LOGI(TAG, "Priority: %s", priority);
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

void SAUNA360Component::process_relay_bitmap(uint32_t data) {
  // Remember previous heater state
  const bool prev_heater_on = this->last_heater_on_;

  // Decode light using model-specific mask
  const bool light_on = (data & this->LIGHT_MASK_) != 0;

  // Decode coils using model-specific mask/shift
  const uint8_t coilmap =
      static_cast<uint8_t>((data & this->COILS_MASK_) >> this->COILS_SHIFT_);
  const bool c1 = (coilmap & 0x01) != 0;
  const bool c2 = (coilmap & 0x02) != 0;
  const bool c3 = (coilmap & 0x04) != 0;
  const int active_coils = (c1 ? 1 : 0) + (c2 ? 1 : 0) + (c3 ? 1 : 0);
  const bool any_coil = (active_coils > 0);

  // Consider heater enabled if status flag or any coil energized
  const bool heater_enabled = this->heating_status_ || any_coil;

  // Optional AUX (we only log if present for COMBI/ELITE)
  const uint32_t AUX1_MASK = 0x00040000;
  const uint32_t AUX2_MASK = 0x00080000;
  const uint32_t AUX3_MASK = 0x00100000;
  const uint32_t AUX_MASK = (AUX1_MASK | AUX2_MASK | AUX3_MASK);
  const bool aux1_on = (data & AUX1_MASK) != 0;
  const bool aux2_on = (data & AUX2_MASK) != 0;
  const bool aux3_on = (data & AUX3_MASK) != 0;

  // Raw frame log
  ESP_LOGI(TAG, "0x7180 raw: 0x%08X (bits:%02X.%02X.%02X.%02X)", data,
           (data >> 24) & 0xFF, (data >> 16) & 0xFF, (data >> 8) & 0xFF,
           data & 0xFF);

  // Pretty coil tuple and binary map (C3 C2 C1)
  char coil_tuple[32];
  snprintf(coil_tuple, sizeof(coil_tuple), "(%s,%s,%s)", c1 ? "ON" : "OFF",
           c2 ? "ON" : "OFF", c3 ? "ON" : "OFF");

  char coilmap_bin[4];
  coilmap_bin[0] = (coilmap & 0x04) ? '1' : '0';
  coilmap_bin[1] = (coilmap & 0x02) ? '1' : '0';
  coilmap_bin[2] = (coilmap & 0x01) ? '1' : '0';
  coilmap_bin[3] = '\0';

  const char *derived_state =
      !heater_enabled ? "OFF" : (any_coil ? "Heating" : "Standby");

  ESP_LOGI(
      TAG, "Light: %s, Heater: %s, Coils: %s (active: %d, map: 0b%s [C3C2C1])",
      ONOFF(light_on), derived_state, coil_tuple, active_coils, coilmap_bin);

  if (this->mode_ == Mode::COMBI || this->mode_ == Mode::ELITE) {
    if ((data & AUX_MASK) != 0) {
      ESP_LOGI(TAG, "AUX relays: [%s,%s,%s]", ONOFF(aux1_on), ONOFF(aux2_on),
               ONOFF(aux3_on));
    } else {
      ESP_LOGI(TAG, "AUX relays: [-]");
    }
  }

  // Session timer: track OFF -> ON and ON -> OFF
  if (!prev_heater_on && heater_enabled) {
    this->session_active_ = true;
    this->session_start_ms_ = millis();
    this->session_frozen_s_ = 0;
    this->last_session_pub_ms_ = millis();
    this->publish_session_();
  } else if (prev_heater_on && !heater_enabled) {
    if (this->session_active_) {
      const uint32_t now = millis();
      this->session_frozen_s_ = (now - this->session_start_ms_) / 1000u;
      this->session_active_ = false;
      this->publish_session_();
    }
  }

  // Publish to HA switches
  if (this->light_relay_switch_ != nullptr)
    this->light_relay_switch_->publish_state(light_on);
  if (this->heater_relay_switch_ != nullptr)
    this->heater_relay_switch_->publish_state(heater_enabled);

  // Cache the truth from 0x7180
  this->last_light_on_ = light_on;
  this->last_heater_on_ = heater_enabled;
  if (!this->relays_known_) {
    ESP_LOGI(
        TAG,
        "Relay map learned from 0x7180 — relay control is now idempotent.");
  }
  this->relays_known_ = true;

  // resync bath time to last user intent
  if (!prev_heater_on && heater_enabled) {
    if (this->last_bath_time_target_ >= 0) {
      const int current_minutes = this->decode_bath_time_minutes_(
          this->bath_time_received_hex_ & 0x0FFF);
      const int target_minutes = this->last_bath_time_target_;
      if (std::abs(current_minutes - target_minutes) >= 2) {
        this->set_bath_time_number(static_cast<float>(target_minutes));
        ESP_LOGI(TAG, "Bath time resynced after ON: target=%d (was %d)",
                 target_minutes, current_minutes);
      }
    }
  }

  // Notify listeners
  std::string state_str(derived_state);
  for (auto &listener : listeners_) {
    listener->on_light_status(light_on);
    listener->on_heater_status(heater_enabled);
    listener->on_heater_state(state_str);
  }
}

void SAUNA360Component::process_tank_level(uint32_t data) {
  const uint32_t lvl = (data & 0x00001800);

  int level_pct = -1;
  switch (lvl) {
  case 0x0000:
    level_pct = 0;
    break;
  case 0x0800:
    level_pct = 50;
    break;
  case 0x1000:
    level_pct = 100;
    break;
  case 0x1800:
    ESP_LOGI(TAG, "Tank level error: (0x%08X)", data);
    return;
  default:
    ESP_LOGI(TAG, "Tank level unknown: 0x%08X", data);
    return;
  }
  ESP_LOGI(TAG, "Tank level: %d%%", level_pct);

  for (auto &listener : listeners_) {
    listener->on_water_tank_level(static_cast<uint16_t>(level_pct));
  }
}

void SAUNA360Component::process_time_limit(uint32_t data) {
  int from_min = (data & 0x3F);
  int from_hour = ((data >> 6) & 0x1F);
  int until_min = ((data >> 11) & 0x3F);
  int until_hour = ((data >> 17) & 0x1F);
  bool active = ((data >> 22) & 0x01);

  ESP_LOGI(TAG, "Not-allowed start window: %02d:%02d -> %02d:%02d (active: %s)",
           from_hour, from_min, until_hour, until_min, active ? "yes" : "no");
}

void SAUNA360Component::process_total_uptime(uint32_t data) {
  for (auto &listener : listeners_) {
    listener->on_total_uptime(data);
  }
  ESP_LOGI(TAG, "Total uptime: %d minutes", data);
}

void SAUNA360Component::process_remaining_time(uint32_t data) {
  const uint16_t raw = static_cast<uint16_t>(data & 0xFFFF);

  // Normalize sentinels to 0 minutes
  const bool is_sentinel =
      (raw == 0xFFFC) || (raw == 0xFFFD) || (raw == 0xFFFE) || (raw == 0xFFFF);
  const uint16_t minutes = is_sentinel ? 0 : raw;

  // Publish to listeners / sensors
  for (auto &listener : listeners_) {
    listener->on_remaining_time(minutes);
  }

  // Log as unsigned to avoid negative prints
  ESP_LOGI(TAG, "Remaining time: %u minutes (raw=0x%04X%s)",
           static_cast<unsigned>(minutes), static_cast<unsigned>(raw),
           is_sentinel ? ", normalized from sentinel" : "");
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
  } else if ((data & 0xF0000000) == 0x10000000) {
    error_message = "High temperature limit control tripped, must be reset";
  }

  if (!error_message.empty()) {
    for (auto &listener : listeners_) {
      listener->on_heater_state(error_message);
    }
    ESP_LOGI(TAG, "Sensor error: %s", error_message.c_str());
  }
}

void SAUNA360Component::set_max_bath_temperature_number(float value) {
  uint32_t minutes_low12 = 0;
  if (this->last_bath_time_target_ >= 0) {
    minutes_low12 =
        static_cast<uint32_t>(this->last_bath_time_target_) & 0x0FFF;
  } else {
    minutes_low12 = this->bath_time_received_hex_ & 0x0FFF;
  }

  uint32_t data = minutes_low12 & 0x0FFF;
  data |= (((static_cast<uint32_t>(value) * 18) & 0x0FFF) << 20);

  this->create_send_data_(0x07, 0x4002, data);
  ESP_LOGI(TAG, "SENT: Max bath temperature: %.0f°C (preserved minutes=%u)",
           value, (unsigned)minutes_low12);
}

void SAUNA360Component::set_bath_time_number(float value) {
  int target = static_cast<int>(std::lround(value));
  if (target < 0)
    target = 0;
  if (target > 575)
    target = 575;

  this->last_bath_time_target_ = target;
  this->last_bath_time_set_ms_ = millis();

  const int encoded = this->encode_bath_time_raw_(target);
  uint32_t payload = static_cast<uint32_t>(encoded) & 0x0FFF;

  // High bits: max bath temperature (12 Bit) keep/set
  if (this->max_bath_temperature_received_hex_) {
    payload |= ((this->max_bath_temperature_received_hex_ & 0xFFF) << 20);
  } else {
    const uint32_t hi =
        static_cast<uint32_t>(this->max_bath_temperature_default_ * 18) & 0xFFF;
    payload |= (hi << 20);
  }

  this->create_send_data_(0x07, 0x4002, payload);
  ESP_LOGI(TAG, "SENT: Bath time target=%d min -> raw=%d (0x%03X)", target,
           encoded, encoded);
}

void SAUNA360Component::set_bath_temperature_number(float value) {
  float current_set_temp =
      (this->setpoint_temperature_received_hex_ & 0x00007FF) / 9.0f;
  value = std::round(value);
  if (static_cast<int>(value) == static_cast<int>(current_set_temp)) {
    return; // nothing to do
  }

  uint32_t data = 0;
  data |= (static_cast<uint32_t>(value) * 9) << 11;       // encode target
  data |= (this->temperature_received_hex_ & 0x000007FF); // keep current temp

  this->create_send_data_(0x07, 0x6000, data);
  ESP_LOGI(TAG, "SENT: Bath temperature: %.0f°C", value);
}

void SAUNA360Component::set_light_relay(bool enable) {
  const uint32_t now = millis();

  if (this->relays_known_) {
    if (this->last_light_on_ == enable) {
      ESP_LOGD(TAG, "Light already %s (0x7180 truth) — no toggle.",
               enable ? "ON" : "OFF");
      return;
    }
  } else {
    if ((now - this->last_light_cmd_ms_) < 250) {
      ESP_LOGD(TAG, "Light toggle suppressed (no relay truth yet; debounce).");
      return;
    }
  }

  this->create_send_data_(0x07, 0x7000, 0x2);
  this->last_light_cmd_ms_ = now;
  ESP_LOGI(TAG, "LIGHT toggle sent (target=%s)", enable ? "ON" : "OFF");
}

void SAUNA360Component::set_heater_relay(bool enable) {
  const uint32_t now = millis();

  if (this->relays_known_) {
    if (this->last_heater_on_ == enable) {
      ESP_LOGD(TAG, "Heater already %s (0x7180 truth) — no toggle.",
               enable ? "ON" : "OFF");
      return;
    }
  } else {
    if ((now - this->last_heater_cmd_ms_) < 250) {
      ESP_LOGD(TAG, "Heater toggle suppressed (no relay truth yet; debounce).");
      return;
    }
  }

  this->create_send_data_(0x07, 0x7000, 0x1);
  this->last_heater_cmd_ms_ = now;
  ESP_LOGI(TAG, "HEATER toggle sent (target=%s)", enable ? "ON" : "OFF");
}

void SAUNA360Component::set_humidity_step_number(float value) {
  int v = static_cast<int>(std::lround(value));
  if (v < 0)
    v = 0;
  if (v > 10)
    v = 10;

  uint32_t payload = (this->bath_type_priority_received_hex_ & 0x0FFFFFFF);

  uint32_t raw =
      static_cast<uint32_t>(v * HUM_STEP_SCALE + HUM_STEP_BASE) & 0xFF;
  payload &= ~(0xFFu << 4);
  payload |= (raw << 4);

  this->create_send_data_(0x07, 0x6001, payload);
  ESP_LOGI(TAG, "SENT: Humidity step: %d (payload=0x%08X)", v, payload);
}

void SAUNA360Component::set_humidity_percent_number(float value) {
  int v = static_cast<int>(std::lround(value));
  if (v < 0)
    v = 0;
  if (v > 63)
    v = 63;

  uint32_t payload = this->bath_type_priority_received_hex_;
  payload |= 0x10000000u; // ensure percent mode

  payload &= ~(0x3Fu << 7);
  payload |= (static_cast<uint32_t>(v) & 0x3F) << 7;

  this->create_send_data_(0x07, 0x6001, payload);
  ESP_LOGI(TAG, "SENT: Humidity target (%%): %d (payload=0x%08X)", v, payload);
}

void SAUNA360Component::create_send_data_(uint8_t type, uint16_t code,
                                          uint32_t data) {
  ESP_LOGD(TAG, "CREATING SEND DATA TYPE:%s CODE:%s DATA:%s",
           format_hex_pretty(type).c_str(), format_hex_pretty(code).c_str(),
           format_hex_pretty(data).c_str());

  std::vector<uint8_t> packet = {0x40, type};
  std::array<uint8_t, 2> code_array = decode_value(code);
  std::array<uint8_t, 4> data_array = decode_value(data);

  packet.insert(packet.end(), code_array.begin(), code_array.end());
  packet.insert(packet.end(), data_array.begin(), data_array.end());

  uint16_t crc_calculated =
      crc16be(packet.data(), packet.size(), 0xffff, 0x90d9, false, false);
  std::array<uint8_t, 2> crc_array = decode_value(crc_calculated);
  packet.insert(packet.end(), crc_array.begin(), crc_array.end());

  packet.insert(packet.begin(), 0x98); // SOF
  packet.push_back(0x9C);              // EOF

  this->tx_queue_.push(packet);
}

void SAUNA360Component::send_data_() {
  const std::vector<uint8_t> &packet = this->tx_queue_.front();
  static constexpr uart_port_t PORT = UART_NUM_0;
  uart_write_bytes(PORT, (const char *)packet.data(), packet.size());
  this->tx_queue_.pop();
}

void SAUNA360Component::publish_session_() {
  const uint32_t s = this->session_active_
                       ? ((millis() - this->session_start_ms_) / 1000u)
                       : this->session_frozen_s_;
  const uint32_t m = s / 60u;

  static uint32_t last_sent_min = 0xFFFFFFFFu;
  if (this->session_active_) {
    if (m == last_sent_min) return;
  }
  last_sent_min = m;

  for (auto &l : this->listeners_) {
    l->on_session_uptime(m);
  }
}

void SAUNA360Component::initialize_defaults() {
  ESP_LOGI(TAG, "=========== Queueing default values ===========");
  if (!std::isnan(this->max_bath_temperature_default_))
    ESP_LOGI(TAG, "Max bath temperature: %.0f°C",
             this->max_bath_temperature_default_);
  if (!std::isnan(this->bath_time_default_))
    ESP_LOGI(TAG, "Bath time: %.0f minutes", this->bath_time_default_);
  if (!std::isnan(this->bath_temperature_default_))
    ESP_LOGI(TAG, "Bath temperature: %.0f°C", this->bath_temperature_default_);
  if (!std::isnan(this->humidity_step_default_))
    ESP_LOGI(TAG, "Humidity step: %.0f", this->humidity_step_default_);
#ifdef USE_NUMBER
  if (!std::isnan(this->humidity_percent_default_))
    ESP_LOGI(TAG, "Humidity percent: %.0f%%", this->humidity_percent_default_);
#endif
  ESP_LOGI(TAG, "================================================");

  if (!std::isnan(this->bath_time_default_))
    this->set_bath_time_number(this->bath_time_default_);

  if (!std::isnan(this->max_bath_temperature_default_))
    this->set_max_bath_temperature_number(this->max_bath_temperature_default_);

  if (!std::isnan(this->bath_temperature_default_))
    this->set_bath_temperature_number(this->bath_temperature_default_);

  if (this->humidity_step_number_ != nullptr &&
      !std::isnan(this->humidity_step_default_))
    this->set_humidity_step_number(this->humidity_step_default_);

#ifdef USE_NUMBER
  if (this->humidity_percent_number_ != nullptr &&
      !std::isnan(this->humidity_percent_default_))
    this->set_humidity_percent_number(this->humidity_percent_default_);
#endif
}

void SAUNA360Component::dump_config() {
  ESP_LOGCONFIG(TAG, "UART component");
  ESP_LOGCONFIG(TAG, "Session timer: enabled");
}

} // namespace sauna360
} // namespace esphome
