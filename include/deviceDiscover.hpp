#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>
#include <mdns.h>
#include <WiFi.h>
#include "appPreferences.hpp"
#include "common.hpp"
#include "appStructs.hpp"

namespace soundtouch
{
  using DevListPtr = std::shared_ptr< AlarmClockSrv::DevList >;

  class DeviceDiscover
  {
    private:
    static const char *tag;
    static bool isInit;
    static bool mdnsIsRunning;
    static TaskHandle_t taskHandle;

    public:
    static void init();
    static void start();

    private:
    static void discoverTask( void * );
    static bool startMDNS();
    static bool stopMDNS();
    static DevListPtr discoverSoundTouchDevices();
    static mdns_result_t *getResult( mdns_result_t *, int );
    static IPAddress getIP( mdns_result_t * );
  };
}  // namespace soundtouch