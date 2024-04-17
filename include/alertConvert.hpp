#pragma once
#include "appStructs.hpp"

namespace alertclock
{
  class AlertConvert
  {
    private:
    static const char *tag;  //! for debugging

    public:
    static AlertDayList getAlertDaysList( const String & );                      //! get daylist from comma separated string
    static void makeDaysString( alertclock::AlertDayList &, String & );          //! make a string of weekdays, if there
    static void makeDevicesString( alertclock::AlertDeviceIdList &, String & );  //! make a string from devices list of strings
    static AlertDeviceIdList getDevicesListForAlert( const String & );            //! List of device-ID's for an alert
  };
}  // namespace alertclock