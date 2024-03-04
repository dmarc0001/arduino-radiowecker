#pragma once
#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

extern logger::Elog elog;
namespace alertclock
{
  extern WiFiUDP udpClient;
}  // namespace alertclock
