#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

namespace alarmclock
{
  logger::Elog elog;
  WiFiUDP udpClient;
}  // namespace alarmclock
