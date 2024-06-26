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

namespace alertclock
{
  /**
   * class handle alert config file, this was a json config file
   */
  class AlertConfObj
  {
    private:
    static const char *tag;

    public:
    static bool readConfig();  //! read whole config from flash
    static bool saveConfig();  //! write config to file

    private:
    static bool readAlertConfig( const String & );                                //! read config from json file
    static bool readDeviceConfig( const String & );                               // read device config to json file
    static cJSON *readAndParseFile( const String & );                             //! read and parse an json file
    static String getValueFromJsonObj( const char *name, const cJSON *jObject );  //! extract an value
  };
}  // namespace alertclock