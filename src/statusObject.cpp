#include <time.h>
#include <memory>
#include <cstring>
#include <algorithm>
#include <esp_log.h>
#include <stdio.h>
#include <cJSON.h>
#include "statics.hpp"
#include "statusObject.hpp"

namespace alertclock
{
  const char *StatusObject::tag{ "statusobj" };
  bool StatusObject::is_init{ false };
  bool StatusObject::is_running{ false };
  bool StatusObject::is_spiffs{ false };
  volatile WlanState StatusObject::wlanState{ WlanState::DISCONNECTED };
  volatile AlertState StatusObject::alertState{ AlertState::ALERT_NONE };
  volatile bool StatusObject::http_active{ false };
  AlRecordList StatusObject::alertList;
  DeviceEntrPtrList StatusObject::devList;

  SemaphoreHandle_t StatusObject::configFileSem{ nullptr };

  /**
   * init object and start file writer task
   */
  void StatusObject::init()
  {
    using namespace logger;

    if ( StatusObject::is_init )
    {
      elog.log( DEBUG, "%s: always initialized... Igpore call", StatusObject::tag );
      return;
    }
    elog.log( INFO, "%s: init status object...", StatusObject::tag );
    //
    // init the filesystem for log and web
    //
    elog.log( DEBUG, "%s: init filesystem...", StatusObject::tag );
    if ( !SPIFFS.begin( false, appprefs::WEB_PATH, 8, appprefs::WEB_PARTITION_LABEL ) )
    {
      elog.log( INFO, "%s: init failed, FORMAT filesystem...", StatusObject::tag );
      if ( !SPIFFS.format() )
      {
        // there is an error BAD!
        elog.log( ERROR, "%s: An Error has occurred while mounting SPIFFS!", StatusObject::tag );
        delay( 5000 );
      }
      else
      {
        elog.log( INFO, "%s: FORMAT filesystem successful...", StatusObject::tag );
        // is okay
        StatusObject::is_spiffs = true;
      }
      ESP.restart();
    }
    else
    {
      // is okay
      StatusObject::is_spiffs = true;
      elog.log( DEBUG, "%s: init filesystem...OK", StatusObject::tag );
    }
    vSemaphoreCreateBinary( StatusObject::configFileSem );
    xSemaphoreGive( StatusObject::configFileSem );
    StatusObject::is_init = true;
    elog.log( DEBUG, "%s: init status object...OK", StatusObject::tag );
  }

  /**
   * set wlan state into object
   */
  void StatusObject::setWlanState( WlanState _state )
  {
    StatusObject::wlanState = _state;
  }

  /**
   * get wlan state from object
   */
  WlanState StatusObject::getWlanState()
  {
    return StatusObject::wlanState;
  }

  /**
   * set alert state, if an new alert will preparing and another is running kepp stsate running
   */
  void StatusObject::setAlertState( AlertState _state )
  {
    switch ( _state )
    {
      case ALERT_NONE:
        alertState = _state;
        break;

      case ALERT_PREPARING:
        if ( alertState == ALERT_RUNNING )
          break;
        alertState = _state;
        break;

      default:
        alertState = _state;
        break;
    }
  }

  /**
   * get alert state
   */
  AlertState StatusObject::getAlertState()
  {
    return alertState;
  }

  /**
   * set is httpd active (active request of a file/data)
   */
  void StatusObject::setHttpActive( bool _http )
  {
    StatusObject::http_active = _http;
  }

  /**
   * get the active state of http daemon
   */
  bool StatusObject::getHttpActive()
  {
    return StatusObject::http_active;
  }

}  // namespace alertclock
