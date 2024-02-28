#pragma once
#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

extern logger::Elog elog;
namespace alarmclock
{
  extern WiFiUDP udpClient;
}  // namespace alarmclock
