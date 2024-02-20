#include "common.hpp"
#include <WiFi.h>
#include <WiFiUdp.h>

namespace AlarmClockSrv
{
  logger::Elog elog;
  WiFiUDP udpClient;
}  // namespace EnvServer
