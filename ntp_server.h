#pragma once

#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"
#include <WiFiUdp.h>

namespace esphome {
namespace ntp_server {

class NTPServerComponent : public Component {
 public:
  NTPServerComponent(uint16_t port) : port_(port) {}

  void set_time_id(time::RealTimeClock *time_id) { this->time_id_ = time_id; }

  float get_setup_priority() const override;
  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  uint16_t port_;
  WiFiUDP udp_;
  time::RealTimeClock *time_id_{nullptr};
};

}  // namespace ntp_server
}  // namespace esphome
