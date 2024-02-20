#pragma once
#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

namespace AlarmClockSrv
{
  extern logger::Elog elog;
  extern WiFiUDP udpClient;
}  // namespace AlarmClockSrv
