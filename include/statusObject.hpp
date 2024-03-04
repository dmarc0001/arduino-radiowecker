#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <SPIFFS.h>
#include "appPreferences.hpp"
#include "appStructs.hpp"

namespace alertclock
{
  class StatusObject
  {
    private:
    static const char *tag;               //! TAG for esp log
    static bool is_init;                  //! was object initialized
    static bool is_running;               //! is save Task running?
    static bool is_spiffs;                //! is fikesystem okay?
    static volatile WlanState wlanState;  //! is wlan disconnected, connected etc....
    static volatile bool http_active;     //! was an acces via http?

    public:
    static AlRecordList alertList;           //! list of alerts (read from file)
    static DeviceEntrPtrList devList;        //! list from devices, read from nDNS
    static SemaphoreHandle_t configFileSem;  // is access to files busy

    public:
    static void init();
    static void setWlanState( WlanState );
    static WlanState getWlanState();
    static void setHttpActive( bool );
    static bool getHttpActive();
    static bool getIsSpiffsOkay()
    {
      return ( StatusObject::is_spiffs );
    }

    private:
  };
}  // namespace alertclock
