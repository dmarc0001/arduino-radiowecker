#include "statics.hpp"
#include "alertConvert.hpp"

namespace alertclock
{
  const char *AlertConvert::tag{ "AlertConvert" };

  AlertDayList AlertConvert::getAlertDaysList( const String &dayListStr )
  {
    using namespace logger;
    AlertDayList days;
    const int strLen = dayListStr.length();

    //
    // is zero len, do nor work, be lazy
    //
    if ( strLen == 0 )
      return days;
    //
    // start at idx 0
    //
    int strIndex = 0;
    // elog.log( DEBUG, "%s: days: <%s>, len <%d>...", AlertConfObj::tag, dayListStr.c_str(), strLen );
    while ( strIndex < strLen )
    {
      String dayStr;
      int idx = dayListStr.indexOf( ',', strIndex );
      if ( idx < 0 )
      {
        // maybe the once or last pair of chars
        // than i'll take all or the last part
        dayStr = dayListStr.substring( strIndex /*, 2*/ );
      }
      else
      {
        // comma found at index greater then 0
        // extract found
        dayStr = dayListStr.substring( strIndex, idx );
      }
      // trim string (spaces remove)
      dayStr.trim();
      if ( dayStr.equals( "mo" ) )
      {
        days.push_back( AlertDays::mo );
      }
      else if ( dayStr.equals( "tu" ) )
      {
        days.push_back( AlertDays::tu );
      }
      else if ( dayStr.equals( "we" ) )
      {
        days.push_back( AlertDays::we );
      }
      else if ( dayStr.equals( "th" ) )
      {
        days.push_back( AlertDays::th );
      }
      else if ( dayStr.equals( "fr" ) )
      {
        days.push_back( AlertDays::fr );
      }
      else if ( dayStr.equals( "sa" ) )
      {
        days.push_back( AlertDays::sa );
      }
      else if ( dayStr.equals( "su" ) )
      {
        days.push_back( AlertDays::su );
      }
      else
      {
        elog.log( ERROR, "%s: not known weekday in config: <%s>!", AlertConvert::tag, dayStr.c_str() );
      }
      strIndex = abs( idx ) + 1;
      // if these the once or last pair of chars
      if ( idx < 0 )
        break;
    }
    // elog.log( DEBUG, "%s: days vector is: <%d>...", AlertConfObj::tag, days.size() );
    return days;
  }

  /**
   * make a list of decice-id's for an alert
   */
  AlertDeviceIdList AlertConvert::getDevicesListForAlert( const String &listStr )
  {
    using namespace logger;

    AlertDeviceIdList deviceList;
    int strLen = listStr.length();

    //
    // is zero len, do nor work, be lazy
    //
    if ( strLen == 0 )
      return deviceList;
    //
    // start at idx 0
    //
    int strIndex = 0;
    elog.log( DEBUG, "%s: devices: <%s>...", AlertConvert::tag, listStr.c_str() );
    while ( strIndex < strLen )
    {
      String devStr;
      int idx = listStr.indexOf( ',', strIndex );
      if ( idx < 0 )
      {
        // maybe the once or last chargroup
        devStr = listStr.substring( strIndex );
      }
      if ( idx > 0 )
      {
        // comma found at index idx
        // extract found
        devStr = listStr.substring( strIndex, idx );
      }
      // trim string (spaces remove)
      devStr.trim();
      elog.log( DEBUG, "%s: found device id: <%s>...", AlertConvert::tag, devStr.c_str() );
      deviceList.push_back( devStr );
      strIndex += idx + 1;
      // if these the once or last pair of chars
      if ( idx < 0 )
        break;
    }
    return deviceList;
  }
  /**
   * make day string from enum
   */
  void AlertConvert::makeDaysString( AlertDayList &list, String &retString )
  {
    for ( auto day = list.begin(); day != list.end(); day++ )
    {
      String loc_day;
      switch ( *day )
      {
        case AlertDays::mo:
          loc_day = "mo";
          break;
        case AlertDays::tu:
          loc_day = "tu";
          break;
        case AlertDays::we:
          loc_day = "we";
          break;
        case AlertDays::th:
          loc_day = "th";
          break;
        case AlertDays::fr:
          loc_day = "fr";
          break;
        case AlertDays::sa:
          loc_day = "sa";
          break;
        case AlertDays::su:
          loc_day = "su";
          break;
      }
      if ( !retString.isEmpty() )
        retString += ",";
      retString += loc_day;
    }
  }

  /**
   * make device string from list
   */
  void AlertConvert::makeDevicesString( AlertDeviceIdList &list, String &retString )
  {
    for ( auto dev = list.begin(); dev != list.end(); dev++ )
    {
      if ( retString.isEmpty() )
        retString = *dev;
      else
        retString += "," + *dev;
    }
  }

}  // namespace alertclock