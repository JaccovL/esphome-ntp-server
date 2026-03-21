#include "ntp_server.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ntp_server {

static const char *const TAG = "ntp_server";

float NTPServerComponent::get_setup_priority() const {
  return setup_priority::AFTER_CONNECTION;
}

void NTPServerComponent::setup() {
  if (this->udp_.begin(this->port_)) {
    ESP_LOGI(TAG, "NTP server listening on UDP port %d", this->port_);
  } else {
    ESP_LOGE(TAG, "Failed to bind UDP port %d", this->port_);
    this->mark_failed();
  }
}

void NTPServerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "NTP Server:");
  ESP_LOGCONFIG(TAG, "  Port: %d", this->port_);
}

void NTPServerComponent::loop() {
  int packet_size = this->udp_.parsePacket();
  if (packet_size < 48)
    return;

  uint8_t request[48];
  this->udp_.read(request, 48);

  if (this->time_id_ == nullptr)
    return;

  auto now = this->time_id_->now();
  if (!now.is_valid()) {
    ESP_LOGD(TAG, "Ignoring NTP request - no valid GPS time yet");
    return;
  }

  // Build NTP response packet
  uint8_t response[48];
  memset(response, 0, 48);

  // Byte 0: LI=0 (no warning), VN=4 (NTPv4), Mode=4 (server)
  response[0] = 0b00100100;

  // Byte 1: Stratum 1 (primary reference - GPS)
  response[1] = 1;

  // Byte 2: Poll interval (6 = 64 seconds)
  response[2] = 6;

  // Byte 3: Precision (-20 ~ microsecond)
  response[3] = 0xEC;

  // Bytes 12-15: Reference ID - "GPS\0"
  response[12] = 'G';
  response[13] = 'P';
  response[14] = 'S';
  response[15] = 0;

  // NTP epoch offset: Jan 1 1900 -> Jan 1 1970 = 2208988800s
  const uint32_t NTP_UNIX_OFFSET = 2208988800UL;
  uint32_t ntp_seconds = (uint32_t) now.timestamp + NTP_UNIX_OFFSET;

  // Bytes 24-31: Origin Timestamp (copy client's transmit timestamp)
  response[24] = request[40];
  response[25] = request[41];
  response[26] = request[42];
  response[27] = request[43];
  response[28] = request[44];
  response[29] = request[45];
  response[30] = request[46];
  response[31] = request[47];

  // Bytes 16-23: Reference Timestamp
  response[16] = (ntp_seconds >> 24) & 0xFF;
  response[17] = (ntp_seconds >> 16) & 0xFF;
  response[18] = (ntp_seconds >> 8) & 0xFF;
  response[19] = (ntp_seconds) & 0xFF;

  // Bytes 32-39: Receive Timestamp
  response[32] = (ntp_seconds >> 24) & 0xFF;
  response[33] = (ntp_seconds >> 16) & 0xFF;
  response[34] = (ntp_seconds >> 8) & 0xFF;
  response[35] = (ntp_seconds) & 0xFF;

  // Bytes 40-47: Transmit Timestamp
  response[40] = (ntp_seconds >> 24) & 0xFF;
  response[41] = (ntp_seconds >> 16) & 0xFF;
  response[42] = (ntp_seconds >> 8) & 0xFF;
  response[43] = (ntp_seconds) & 0xFF;

  // Send response
  this->udp_.beginPacket(this->udp_.remoteIP(), this->udp_.remotePort());
  this->udp_.write(response, 48);
  this->udp_.endPacket();

  ESP_LOGI(TAG, "NTP response sent to %s:%d (time: %04d-%02d-%02d %02d:%02d:%02d)",
           this->udp_.remoteIP().toString().c_str(), this->udp_.remotePort(),
           now.year, now.month, now.day_of_month,
           now.hour, now.minute, now.second);
}

}  // namespace ntp_server
}  // namespace esphome
