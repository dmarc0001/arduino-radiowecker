#include "alertConfigFileObj.hpp"
#include "statusObject.hpp"
#include "statics.hpp"
#include "alertConvert.hpp"

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

  bool AlertConfObj::saveConfig()
  {
    bool isOkay{ true };

    elog.log( INFO, "%s: save alert config...", AlertConfObj::tag );
    if ( xSemaphoreTake( StatusObject::configFileSem, pdMS_TO_TICKS( 6000 ) ) == pdTRUE )
    {
      //
      // save data
      // first create an json Array for alerts
      //
      cJSON *root = cJSON_CreateArray();
      for ( auto alertIt = StatusObject::alertList.begin(); alertIt != StatusObject::alertList.end(); alertIt++ )
      {
        //
        // create json Object for root Array and every alert
        //
        cJSON *devObj = cJSON_CreateObject();
        //
        cJSON_AddStringToObject( devObj, "note", ( *alertIt )->note.c_str() );
        cJSON_AddStringToObject( devObj, "name", ( *alertIt )->name.c_str() );
        cJSON_AddBoolToObject( devObj, "enable", ( *alertIt )->enable );
        cJSON_AddStringToObject( devObj, "volume", String( ( *alertIt )->volume ).c_str() );
        cJSON_AddBoolToObject( devObj, "raise", ( *alertIt )->raiseVol );
        cJSON_AddStringToObject( devObj, "location", "" );
        cJSON_AddStringToObject( devObj, "source", ( *alertIt )->source.c_str() );
        cJSON_AddStringToObject( devObj, "duration", String( ( *alertIt )->duration ).c_str() );
        cJSON_AddStringToObject( devObj, "sourceAccount", "" );
        cJSON_AddStringToObject( devObj, "type", "" );
        cJSON_AddStringToObject( devObj, "alertHour", String( ( *alertIt )->alertHour ).c_str() );
        cJSON_AddStringToObject( devObj, "alertMinute", String( ( *alertIt )->alertMinute ).c_str() );
        if ( ( *alertIt )->day == 255 )
          cJSON_AddStringToObject( devObj, "day", "" );
        else
          cJSON_AddStringToObject( devObj, "day", String( ( *alertIt )->day ).c_str() );
        if ( ( *alertIt )->month == 255 )
          cJSON_AddStringToObject( devObj, "month", "" );
        else
          cJSON_AddStringToObject( devObj, "month", String( ( *alertIt )->month ).c_str() );
        // days enum-vector to string
        String listJoinString;
        AlertConvert::makeDaysString( ( *alertIt )->days, listJoinString );
        cJSON_AddStringToObject( devObj, "days", listJoinString.c_str() );
        listJoinString.clear();
        // devices string-vector to String
        AlertConvert::makeDevicesString( ( *alertIt )->devices, listJoinString );
        cJSON_AddStringToObject( devObj, "devices", listJoinString.c_str() );
      }
      const char *alerts = cJSON_Print( root );
      String alertsStr( alerts );
      //
      // TODO: write to file
      //
      File fd;
      elog.log( DEBUG, "%s: open %s...", AlertConfObj::tag, appprefs::LocalPrefs::alertConfigFile );
      fd = SPIFFS.open( appprefs::LocalPrefs::alertConfigFile, "w", true );
      if ( !fd )
        isOkay = false;
      // checkpoint
      elog.log( DEBUG, "%s: open %s...OK", AlertConfObj::tag, appprefs::LocalPrefs::alertConfigFile );
      if ( isOkay )
      {
        fd.print( alertsStr );
        fd.flush();
        fd.close();
      }
      else
      {
        elog.log( ERROR, "%s: save alert config tailed, can't open file <%s>...", AlertConfObj::tag,
                  appprefs::LocalPrefs::alertConfigFile );
      }
      free( ( void * ) alerts );
      cJSON_Delete( root );
      xSemaphoreGive( StatusObject::configFileSem );
      return isOkay;
    }
    elog.log( ERROR, "%s: save alert config tailed, can't obtain semaphore...", AlertConfObj::tag );
    return isOkay;
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
        String aName( AlertConfObj::getValueFromJsonObj( "name", elem ) );
        AlertEntryPtr entry = std::make_shared< AlertEntry >( aName );
        elog.log( DEBUG, "%s: alert <%s>...", AlertConfObj::tag, aName.c_str() );
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
        entry->days = AlertConvert::getAlertDaysList( alertDaysListStr );
        //
        // also special
        // string with comma to vector of devices
        // not validation of the devices yet
        //
        String alertDevicesListStr = AlertConfObj::getValueFromJsonObj( "devices", elem );
        entry->devices = AlertConvert::getDevicesListForAlert( alertDevicesListStr );
        entry->lastWriten = 0L;
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

}  // namespace alertclock