#include "alertConfigFileObj.hpp"
#include "statusObject.hpp"
#include "statics.hpp"

namespace alertclock
{
  using namespace logger;

  // AlertConfObj
  const char *AlertConfObj::tag{ "alertconfig" };

  bool AlertConfObj::readConfig()
  {
    if ( AlertConfObj::readDeviceConfig( appprefs::LocalPrefs::devicesConfigfile ) )
      if ( AlertConfObj::readAlertConfig( appprefs::LocalPrefs::alertConfigFile ) )
        return true;
    return false;
  }

  /**
   * read alert config from file to the alert list
   */
  bool AlertConfObj::readAlertConfig( const String &fileName )
  {
    elog.log( DEBUG, "%s: alert config, get filesystem infos...", AlertConfObj::tag );
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    uint32_t fileSize{ 0U };
    bool retVal{ false };
    //
    // delete current list
    //
    StatusObject::alertList.clear();
    elog.log( DEBUG, "%s: infos total: %d, used: %d free: %d ...", AlertConfObj::tag, total, used, total - used );
    delay( 50 );
    cJSON *jsonObject = AlertConfObj::readAndParseFile( fileName );
    if ( jsonObject )
    {
      cJSON *elem;
      cJSON_ArrayForEach( elem, jsonObject )
      {
        //
        // every element
        //
        AlertEntryPtr entry = std::make_shared< AlertEntry >();
        entry->name = AlertConfObj::getValueFromJsonObj( "name", elem );
        elog.log( DEBUG, "%s: alert <%s>...", AlertConfObj::tag, entry->name.c_str() );
        entry->volume = static_cast< uint8_t >( AlertConfObj::getValueFromJsonObj( "volume", elem ).toInt() );
        entry->location = AlertConfObj::getValueFromJsonObj( "location", elem );
        entry->source = AlertConfObj::getValueFromJsonObj( "source", elem );
        entry->raiseVol = ( ( AlertConfObj::getValueFromJsonObj( "raise", elem ) ).compareTo( "true" ) == 0 ) ? true : false;
        entry->duration = static_cast< uint16_t >( AlertConfObj::getValueFromJsonObj( "duration", elem ).toInt() );
        entry->sourceAccount = AlertConfObj::getValueFromJsonObj( "account", elem );
        entry->type = AlertConfObj::getValueFromJsonObj( "type", elem );
        entry->enable = ( ( AlertConfObj::getValueFromJsonObj( "enable", elem ) ).compareTo( "true" ) == 0 ) ? true : false;
        entry->note = AlertConfObj::getValueFromJsonObj( "note", elem );
        entry->alertHour = static_cast< uint8_t >( AlertConfObj::getValueFromJsonObj( "hour", elem ).toInt() );
        entry->alertMinute = static_cast< uint8_t >( AlertConfObj::getValueFromJsonObj( "minute", elem ).toInt() );
        //
        // day/month 255 or value from entry
        //
        if ( AlertConfObj::getValueFromJsonObj( "dateday", elem ).isEmpty() )
        {
          entry->day = 255;
        }
        else
        {
          entry->day = static_cast< uint8_t >( AlertConfObj::getValueFromJsonObj( "dateday", elem ).toInt() );
        }
        if ( AlertConfObj::getValueFromJsonObj( "datemonth", elem ).isEmpty() )
        {
          entry->month = 255;
        }
        else
        {
          entry->month = static_cast< uint8_t >( AlertConfObj::getValueFromJsonObj( "datemonth", elem ).toInt() );
        }
        //
        // a little special
        // string with comma to vactor of enum
        //
        String alertDaysListStr = AlertConfObj::getValueFromJsonObj( "days", elem );
        entry->days = AlertConfObj::getAlertDaysList( alertDaysListStr );
        //
        // also special
        // string with comma to vector of devices
        // not validation of the devices yet
        //
        String alertDevicesListStr = AlertConfObj::getValueFromJsonObj( "devices", elem );
        entry->devices = AlertConfObj::getDevicesListForAlert( alertDevicesListStr );
        StatusObject::alertList.push_back( entry );
        delay( 60 );
      }
      // delete the JSON object
      cJSON_Delete( jsonObject );
      retVal = true;
    }
    return retVal;
  }  // namespace AlarmClockSrv

  /**
   * save curent alerts to a json file
   * TODO: implement
   */
  bool AlertConfObj::saveAlertConfig( const String & )
  {
    if ( xSemaphoreTake( StatusObject::configFileSem, pdMS_TO_TICKS( 6000 ) ) == pdTRUE )
    {
      xSemaphoreGive( StatusObject::configFileSem );
      return true;
    }
    return false;
  }

  /**
   * read devices list from file into devices list, initial load
   * if there is no file, no config read
   * devices will be discovered if device is online
   */
  bool AlertConfObj::readDeviceConfig( const String &fileName )
  {
    elog.log( DEBUG, "%s: device config, get filesystem infos...", AlertConfObj::tag );
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    uint32_t fileSize{ 0U };
    //
    elog.log( DEBUG, "%s: infos total: %d, used: %d free: %d ...", AlertConfObj::tag, total, used, total - used );
    cJSON *jsonObject = AlertConfObj::readAndParseFile( fileName );
    if ( jsonObject )
    {
      StatusObject::devList.clear();
      cJSON *elem;
      cJSON_ArrayForEach( elem, jsonObject )
      {
        DeviceEntryPtr devElem = std::make_shared< DeviceEntry >();
        //
        // every element
        //
        devElem->name = AlertConfObj::getValueFromJsonObj( "name", elem );
        elog.log( DEBUG, "%s: found device name <%s>...", AlertConfObj::tag, devElem->name.c_str() );
        devElem->addr.fromString( AlertConfObj::getValueFromJsonObj( "ip", elem ) );
        devElem->id = AlertConfObj::getValueFromJsonObj( "id", elem );
        devElem->webPort = static_cast< uint16_t >( AlertConfObj::getValueFromJsonObj( "webport", elem ).toInt() );
        devElem->wsPort = static_cast< uint16_t >( AlertConfObj::getValueFromJsonObj( "wsport", elem ).toInt() );
        devElem->type = AlertConfObj::getValueFromJsonObj( "type", elem );
        devElem->note = AlertConfObj::getValueFromJsonObj( "note", elem );
        StatusObject::devList.push_back( devElem );
        delay( 100 );
      }
    }
    // delete the JSON object
    cJSON_Delete( jsonObject );
    return true;
  }

  /**
   * save current devices to a json file
   */
  /**
   * read an json file and parse it
   * return pointer to parsed file
   */
  cJSON *AlertConfObj::readAndParseFile( const String &fileName )
  {
    elog.log( DEBUG, "%s: get filesystem infos...", AlertConfObj::tag );
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    uint32_t fileSize{ 0U };
    cJSON *jsonObject{ nullptr };
    char *myBuffer{ nullptr };

    bool isOkay{ true };
    //
    elog.log( DEBUG, "%s: infos total: %d, used: %d free: %d ...", AlertConfObj::tag, total, used, total - used );
    sleep( 1 );
    if ( xSemaphoreTake( StatusObject::configFileSem, pdMS_TO_TICKS( 6000 ) ) == pdTRUE )
    {
      //
      // taken the semaphore
      //
      File fd;
      elog.log( DEBUG, "%s: open %s...", AlertConfObj::tag, fileName.c_str() );
      fd = SPIFFS.open( fileName, "r", false );
      if ( !fd )
        isOkay = false;
      // checkpoint
      elog.log( DEBUG, "%s: open %s...OK", AlertConfObj::tag, fileName.c_str() );
      if ( isOkay )
      {
        // file is opened
        elog.log( DEBUG, "%s: file %s opened, check size...", AlertConfObj::tag, fileName.c_str() );
        fileSize = static_cast< uint32_t >( fd.size() );
        elog.log( DEBUG, "%s: file %s size is %07d...", AlertConfObj::tag, fileName.c_str(), fileSize );
        uint32_t freeRam = esp_get_minimum_free_heap_size();
        if ( fileSize + 200 > freeRam )
        {
          isOkay = false;
          fd.close();
          elog.log( CRITICAL, "%s: not enough free ram for reading %s. ABORT!", AlertConfObj::tag, fileName.c_str() );
        }
      }
      // checkpoint
      if ( isOkay )
      {
        //
        // enough ram, buffer the file
        //
        elog.log( DEBUG, "%s: alloc file (%d bytes)buffer...", AlertConfObj::tag, fileSize );
        myBuffer = static_cast< char * >( malloc( fileSize + 50 ) );
        char *buffptr = myBuffer;
        uint32_t count{ 0U };
        elog.log( DEBUG, "%s: read file into buffer...", AlertConfObj::tag );
        while ( fd.available() && count < fileSize + 4 )
        {
          *buffptr = static_cast< char >( fd.read() );
          ++buffptr;
          ++count;
        }
        fd.close();
        elog.log( DEBUG, "%s: file closed, parse json data...", AlertConfObj::tag );
        jsonObject = cJSON_Parse( myBuffer );
        //
        // worked?
        //
        if ( jsonObject == nullptr )
        {
          isOkay = false;
          //
          // not worked
          //
          const char *error_ptr = cJSON_GetErrorPtr();
          if ( error_ptr != NULL )
          {
            elog.log( ERROR, "%s: json file parsing failed (%s)... ABORT!", AlertConfObj::tag, error_ptr );
          }
        }
        //
        // free the buffer
        //
        if ( myBuffer )
          free( myBuffer );
      }
      else
      {
        elog.log( ERROR, "%s: file %s can't open!", AlertConfObj::tag, fileName.c_str() );
      }
      xSemaphoreGive( StatusObject::configFileSem );
    }
    return jsonObject;
  }

  /**
   * check presence and give the value of json entry (string)
   */
  String AlertConfObj::getValueFromJsonObj( const char *name, const cJSON *jObject )
  {
    cJSON *elem = cJSON_GetObjectItemCaseSensitive( jObject, name );
    if ( cJSON_IsString( elem ) && ( elem->valuestring != nullptr ) )
    {
      String value( elem->valuestring );
      return value;
    }
    return ( String() );
    ;
  }

  /**
   * mak a enum list fo weekdays for an alert
   */
  AlertDayList AlertConfObj::getAlertDaysList( const String &dayListStr )
  {
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
        elog.log( ERROR, "%s: not known weekday in config: <%s>!", AlertConfObj::tag, dayStr.c_str() );
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
  AlertDeviceIdList AlertConfObj::getDevicesListForAlert( const String &listStr )
  {
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
    elog.log( DEBUG, "%s: devices: <%s>...", AlertConfObj::tag, listStr.c_str() );
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
      elog.log( DEBUG, "%s: found device id: <%s>...", AlertConfObj::tag, devStr.c_str() );
      deviceList.push_back( devStr );
      strIndex += idx + 1;
      // if these the once or last pair of chars
      if ( idx < 0 )
        break;
    }
    return deviceList;
  }
}  // namespace alertclock