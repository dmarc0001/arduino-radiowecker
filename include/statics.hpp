#pragma once
#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

namespace alarmclock
{
  extern logger::Elog elog;
  extern WiFiUDP udpClient;
}  // namespace alarmclock
