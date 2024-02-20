#pragma once
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <SPIFFS.h>
#include <cJSON.h>
#include "common.hpp"
#include "appPreferences.hpp"
#include "appStructs.hpp"

namespace AlarmClockSrv
{
  /**
   * class handle alert config file, this was a json config file
   */
  class AlertConfObj
  {
    private:
    static const char *tag;

    public:
    static bool readConfig();                         //! read whole config from flash
    static bool readAlertConfig( const String & );    //! read config from json file
    static bool saveAlertConfig( const String & );    //! write config to json file
    static bool readDeviceConfig( const String & );   // read device config to json file
    static bool writeDeviceConfig( const String & );  //! write device config to json file

    private:
    static cJSON *readAndParseFile( const String & );                             //! read and parse an json file
    static String getValueFromJsonObj( const char *name, const cJSON *jObject );  //! extract an value
    static AlertDayList getAlertDaysList( const String & );                       //! convert to alert days
    static AlertDeviceIdList getDevicesListForAlert( const String & );            //! List of device-ID's for an alert
  };
}  // namespace AlarmClockSrv