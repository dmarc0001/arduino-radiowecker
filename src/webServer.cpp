#include <memory>
#include <cJSON.h>
#include <esp_chip_info.h>
#include "webServer.hpp"
#include "statics.hpp"
#include "appPreferences.hpp"
#include "statusObject.hpp"
#include "alertConvert.hpp"
#include "version.hpp"

namespace webserver
{
  using namespace alertclock;

  const char *AlWebServer::tag{ "webserver" };
  // instantiate a webserver
  AsyncWebServer AlWebServer::server( 80 );

  /**
   * ini a few things?
   */
  void AlWebServer::init()
  {
    //
    // maybe a few things to init?
    //
    StatusObject::init();
  }

  /**
   * start the webserver service
   */
  void AlWebServer::start()
  {
    using namespace logger;

    elog.log( INFO, "%s: start webserver...", AlWebServer::tag );
    // Cache responses for 1 minutes (60 seconds)
    AlWebServer::server.serveStatic( "/", SPIFFS, "/spiffs/" ).setCacheControl( "max-age=60" );
    //
    // response filters
    //
    AlWebServer::server.on( "/", HTTP_GET, AlWebServer::onIndex );
    AlWebServer::server.on( "/index\\.html", HTTP_GET, AlWebServer::onIndex );
    AlWebServer::server.on( "^\\/api\\/v1\\/set-(.*)\?(.*)$", HTTP_GET | HTTP_POST, AlWebServer::onApiV1Set );
    AlWebServer::server.on( "^\\/api\\/v1\\/(.*)$", HTTP_GET, AlWebServer::onApiV1 );
    AlWebServer::server.on( "^\\/.*$", HTTP_GET, AlWebServer::onFilesReq );
    AlWebServer::server.onNotFound( AlWebServer::onNotFound );
    AlWebServer::server.begin();
    elog.log( DEBUG, "%s: start webserver...OK", AlWebServer::tag );
  }

  /**
   * stop the webserver service comlete
   */
  void AlWebServer::stop()
  {
    AlWebServer::server.reset();
    AlWebServer::server.end();
  }

  /**
   * response for index request
   */
  void AlWebServer::onIndex( AsyncWebServerRequest *request )
  {
    String file( "/index.html" );
    StatusObject::setHttpActive( true );
    AlWebServer::deliverFileToHttpd( file, request );
    // request->send( 200, "text/plain", "Hello, INDEX" );
  }

  /**
   * response for request a file (for a directory my fail)
   */
  void AlWebServer::onFilesReq( AsyncWebServerRequest *request )
  {
    StatusObject::setHttpActive( true );
    String file( request->url() );
    AlWebServer::deliverFileToHttpd( file, request );
  }

  /**
   * response for an api request, Version 1
   */
  void AlWebServer::onApiV1( AsyncWebServerRequest *request )
  {
    StatusObject::setHttpActive( true );
    String parameter = request->pathArg( 0 );
    elog.log( logger::DEBUG, "%s: api version 1 call <%s>", AlWebServer::tag, parameter );
    if ( parameter.equals( "alerts" ) )
    {
      // get all alerts
      AlWebServer::apiAllAlertsGetHandler( request );
    }
    else if ( parameter.equals( "alert" ) )
    {
      // get one alert (GET + param alert=ALERTNAME )
      AlWebServer::apiOneAlertsGetHandler( request );
    }
    else if ( parameter.equals( "devices" ) )
    {
      // get a list of found devices
      AlWebServer::apiDevicesGetHandler( request );
    }
    else if ( parameter.equals( "version" ) )
    {
      AlWebServer::apiVersionInfoGetHandler( request );
    }
    else if ( parameter.equals( "info" ) )
    {
      AlWebServer::apiSystemInfoGetHandler( request );
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <" + parameter + ">" );
    }
    // TODO: implement loglevel change (Elog::globalSettings)
  }

  /**
   * compute set commands via API
   */
  void AlWebServer::onApiV1Set( AsyncWebServerRequest *request )
  {
    StatusObject::setHttpActive( true );
    String verb = request->pathArg( 0 );

    // String server, port;

    elog.log( logger::DEBUG, "%s: api version 1 call set-%s", AlWebServer::tag, verb );
    if ( verb.equals( "al-enable" ) )
    {
      if ( request->getParam( "alert" ) && request->getParam( "enable" ) && request->method() == HTTP_GET )
      {
        String alertName = request->getParam( "alert" )->value();
        bool enabled = request->getParam( "enable" )->value().equals( "true" ) ? true : false;
        elog.log( logger::DEBUG, "%s: set-%s, param: %s to <%s>", AlWebServer::tag, verb.c_str(), alertName.c_str(),
                  enabled ? "true" : "false" );
        //
        // searching the right alert
        //
        auto alertPtr = StatusObject::getAlertWithName( alertName );
        if ( alertPtr )
        {
          if ( alertPtr->enable != enabled )
          {
            alertPtr->enable = enabled;
            StatusObject::setWasConfigChanged( true );
          }
          request->send( 200, "text/plain",
                         "OK api call v1 for <set-" + verb + "> -  alert <" + alertName + "> set to <" + enabled + ">." );
          return;
        }
        elog.log( logger::ERROR, "%s: set-%s, requested alert not found!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> requested alert not found!" );
        return;
      }
      else
      {
        elog.log( logger::ERROR, "%s: set-%s, without params!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    //
    // raise volume for this alert?
    //
    else if ( verb.equals( "al-raise" ) )
    {
      if ( request->getParam( "alert" ) && request->getParam( "enable" ) && request->method() == HTTP_GET )
      {
        String alertName = request->getParam( "alert" )->value();
        bool enabled = request->getParam( "enable" )->value().equals( "true" ) ? true : false;
        elog.log( logger::DEBUG, "%s: set-%s, param: %s to <%s>", AlWebServer::tag, verb.c_str(), alertName.c_str(),
                  enabled ? "true" : "false" );
        //
        // searching the right alert
        //
        auto alertPtr = StatusObject::getAlertWithName( alertName );
        if ( alertPtr )
        {
          if ( alertPtr->raiseVol != enabled )
          {
            alertPtr->raiseVol = enabled;
            StatusObject::setWasConfigChanged( true );
          }
          request->send( 200, "text/plain",
                         "OK api call v1 for <set-" + verb + "> -  alert <" + alertName + "> set to <" + enabled + ">." );
          return;
        }
        elog.log( logger::ERROR, "%s: set-%s, requested alert not found!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> requested alert not found!" );
        return;
      }
      else
      {
        elog.log( logger::ERROR, "%s: set-%s, without params!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    //
    // alert delete
    //
    else if ( verb.equals( "al-delete" ) )
    {
      if ( request->hasParam( "alert" ) && request->method() == HTTP_GET )
      {
        String alertName = request->getParam( "alert" )->value();
        //
        // searching the right alert
        //
        for ( auto alertIter = StatusObject::alertList.begin(); alertIter != StatusObject::alertList.end(); alertIter++ )
        {
          if ( ( *alertIter )->name.equals( alertName ) )
          {
            StatusObject::alertList.erase( alertIter );
            StatusObject::setWasConfigChanged( true );
            request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + "> -  alert <" + alertName + ">." );
            return;
          }
        }
        elog.log( logger::ERROR, "%s: set-%s, requested alert not found!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "OK api call v1 for <set-" + verb + "> -  alert <" + alertName + "> not found." );
        return;
      }
      else
      {
        elog.log( logger::ERROR, "%s: set-%s, without params!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    //
    // timezone set?
    //
    else if ( verb.equals( "timezone" ) )
    {
      // timezone parameter find
      if ( request->hasParam( "timezone" ) && request->method() == HTTP_GET )
      {
        String timezone = request->getParam( "timezone" )->value();
        elog.log( logger::DEBUG, "%s: set-%s, param: %s", AlWebServer::tag, verb.c_str(), timezone.c_str() );
        // appprefs::LocalPrefs::setTimeZone( timezone );
        request->send( 200, "text/plain",
                       "OK api call v1 for <set-" + verb + "> -  not implemented (BUG:) while lib error in esp32-s2" );
        // setenv( "TZ", timezone.c_str(), 1 );
        // tzset();
        // yield();
        // sleep( 1 );
        // ESP.restart();
        return;
      }
      // timezone offset parameter find
      else if ( request->hasParam( "timezone-offset" ) && request->method() == HTTP_GET )
      {
        String timezone = request->getParam( "timezone-offset" )->value();
        elog.log( logger::DEBUG, "%s: set-%s, param: %s", AlWebServer::tag, verb.c_str(), timezone.c_str() );
        appprefs::LocalPrefs::setTimezoneOffset( timezone.toInt() );
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
        // setenv( "TZ", timezone.c_str(), 1 );
        // tzset();
        yield();
        sleep( 1 );
        ESP.restart();
        return;
      }
      else
      {
        elog.log( logger::ERROR, "%s: set-%s, param not found!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    else if ( verb.equals( "loglevel" ) )
    {
      // loglevel parameter find
      if ( request->hasParam( "level" ) && request->method() == HTTP_GET )
      {
        String level = request->getParam( "level" )->value();
        elog.log( logger::DEBUG, "%s: set-%s, param: %s", AlWebServer::tag, verb.c_str(), level.c_str() );
        uint8_t numLevel = static_cast< uint8_t >( level.toInt() );
        appprefs::LocalPrefs::setLogLevel( numLevel );
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
        yield();
        sleep( 1 );
        ESP.restart();
        return;
      }
      else
      {
        elog.log( logger::ERROR, "%s: set-%s, param not found!", AlWebServer::tag, verb.c_str() );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    else if ( verb.equals( "syslog" ) )
    {
      //
      // server/port set?
      //
      if ( request->getParam( "server" ) && request->getParam( "port" ) && request->method() == HTTP_GET )
      {
        String server = request->getParam( "server" )->value();
        String port = request->getParam( "port" )->value();
        //
        // params to usable objects
        //
        IPAddress srv;
        srv.fromString( server );
        uint16_t srvport = static_cast< uint16_t >( port.toInt() );
        elog.log( logger::DEBUG, "%s: set-syslog, params: %s, %s", AlWebServer::tag, server.c_str(), port.c_str() );
        appprefs::LocalPrefs::setSyslogServer( srv );
        appprefs::LocalPrefs::setSyslogPort( srvport );
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + "> RESTART CONTROLLER" );
        yield();
        sleep( 2 );
        ESP.restart();
      }
      else
      {
        request->send( 300, "text/plain", "fail api call v1 for <set-" + verb + "> - no server/port params" );
        return;
      }
    }
    else
    {
      String meth( request->method() == HTTP_POST ? "POST" : "GET" );
      request->send( 300, "text/plain", "unknown api call v1 for <set-" + verb + "> method: " + meth );
      return;
    }
  }

  /**
   * get system info as json
   */
  void AlWebServer::apiSystemInfoGetHandler( AsyncWebServerRequest *request )
  {
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info( &chip_info );
    cJSON_AddStringToObject( root, "version", IDF_VER );
    cJSON_AddNumberToObject( root, "cores", chip_info.cores );
    const char *sys_info = cJSON_Print( root );
    String info( sys_info );
    request->send( 200, "application/json", info );
    free( ( void * ) sys_info );
    cJSON_Delete( root );
  }

  /**
   * ask for software vesion
   */
  void AlWebServer::apiVersionInfoGetHandler( AsyncWebServerRequest *request )
  {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject( root, "version", Prefs::VERSION );
    const char *sys_info = cJSON_Print( root );
    String info( sys_info );
    request->send( 200, "application/json", info );
    free( ( void * ) sys_info );
    cJSON_Delete( root );
  }

  /**
   * get all alerts
   */
  void AlWebServer::apiAllAlertsGetHandler( AsyncWebServerRequest *request )
  {
    //
    // create an json Array for alerts
    //
    cJSON *root = cJSON_CreateArray();
    //
    // for all alerts
    //
    for ( auto alert = StatusObject::alertList.begin(); alert != StatusObject::alertList.end(); alert++ )
    {
      //
      // create json Object for root Array and every alert
      //
      cJSON *devObj = cJSON_CreateObject();
      //
      cJSON_AddStringToObject( devObj, "note", ( *alert )->note.c_str() );
      cJSON_AddStringToObject( devObj, "name", ( *alert )->name.c_str() );
      cJSON_AddBoolToObject( devObj, "enable", ( *alert )->enable );
      cJSON_AddBoolToObject( devObj, "deleted", false );
      cJSON_AddStringToObject( devObj, "volume", String( ( *alert )->volume ).c_str() );
      cJSON_AddBoolToObject( devObj, "raise", ( *alert )->raiseVol );
      cJSON_AddStringToObject( devObj, "location", "" );
      cJSON_AddStringToObject( devObj, "source", ( *alert )->source.c_str() );
      cJSON_AddStringToObject( devObj, "duration", String( ( *alert )->duration ).c_str() );
      cJSON_AddStringToObject( devObj, "sourceAccount", "" );
      cJSON_AddStringToObject( devObj, "type", "" );
      cJSON_AddStringToObject( devObj, "alertHour", String( ( *alert )->alertHour ).c_str() );
      cJSON_AddStringToObject( devObj, "alertMinute", String( ( *alert )->alertMinute ).c_str() );
      if ( ( *alert )->day == 255 )
        cJSON_AddStringToObject( devObj, "day", "" );
      else
        cJSON_AddStringToObject( devObj, "day", String( ( *alert )->day ).c_str() );
      if ( ( *alert )->month == 255 )
        cJSON_AddStringToObject( devObj, "month", "" );
      else
        cJSON_AddStringToObject( devObj, "month", String( ( *alert )->month ).c_str() );
      // days enum-vector to string
      String listJoinString;
      AlertConvert::makeDaysString( ( *alert )->days, listJoinString );
      cJSON_AddStringToObject( devObj, "days", listJoinString.c_str() );
      listJoinString.clear();
      // devices string-vector to String
      AlertConvert::makeDevicesString( ( *alert )->devices, listJoinString );
      cJSON_AddStringToObject( devObj, "devices", listJoinString.c_str() );
      // make timestamp to ascci
      char buf[ 32 ];
      ltoa( ( *alert )->lastWriten, buf, 10 );
      cJSON_AddStringToObject( devObj, "lastWritten", buf );
      cJSON_AddItemToArray( root, devObj );
    }
    const char *alerts = cJSON_Print( root );
    String alertsStr( alerts );
    request->send( 200, "application/json", alertsStr );
    free( ( void * ) alerts );
    cJSON_Delete( root );
  }

  /**
   * get one alert
   */
  void AlWebServer::apiOneAlertsGetHandler( AsyncWebServerRequest *request )
  {
    String alertName{ "" };
    bool hasError{ false };
    if ( request->hasParam( "alert" ) )
    {
      alertName = request->getParam( "alert" )->value();
    }
    else
    {
      request->send( 500, "text/plain", "request one alert without alert name" );
      return;
    }
    if ( !hasError )
    {
      //
      // search for alert name
      //
      auto alertPtr = StatusObject::getAlertWithName( alertName );
      if ( alertPtr )
      {
        //
        // i want to receive this alert
        //
        cJSON *root = cJSON_CreateObject();
        //
        cJSON_AddStringToObject( root, "note", alertPtr->note.c_str() );
        cJSON_AddStringToObject( root, "name", alertPtr->name.c_str() );
        cJSON_AddBoolToObject( root, "enable", alertPtr->enable );
        cJSON_AddBoolToObject( root, "deleted", false );
        cJSON_AddStringToObject( root, "volume", String( alertPtr->volume ).c_str() );
        cJSON_AddBoolToObject( root, "raise", alertPtr->raiseVol );
        cJSON_AddStringToObject( root, "location", "" );
        cJSON_AddStringToObject( root, "source", alertPtr->source.c_str() );
        cJSON_AddStringToObject( root, "duration", String( alertPtr->duration ).c_str() );
        cJSON_AddStringToObject( root, "sourceAccount", "" );
        cJSON_AddStringToObject( root, "type", "" );
        cJSON_AddStringToObject( root, "alertHour", String( alertPtr->alertHour ).c_str() );
        cJSON_AddStringToObject( root, "alertMinute", String( alertPtr->alertMinute ).c_str() );
        if ( alertPtr->day == 255 )
          cJSON_AddStringToObject( root, "day", "" );
        else
          cJSON_AddStringToObject( root, "day", String( alertPtr->day ).c_str() );
        if ( alertPtr->month == 255 )
          cJSON_AddStringToObject( root, "month", "" );
        else
          cJSON_AddStringToObject( root, "month", String( alertPtr->month ).c_str() );
        // days enum-vector to string
        String listJoinString;
        AlertConvert::makeDaysString( alertPtr->days, listJoinString );
        cJSON_AddStringToObject( root, "days", listJoinString.c_str() );
        listJoinString.clear();
        // devices string-vector to String
        AlertConvert::makeDevicesString( alertPtr->devices, listJoinString );
        cJSON_AddStringToObject( root, "devices", listJoinString.c_str() );
        // make timestamp to ascci
        char buf[ 32 ];
        ltoa( alertPtr->lastWriten, buf, 10 );
        cJSON_AddStringToObject( root, "lastWritten", buf );
        const char *alert = cJSON_Print( root );
        String alertsStr( alert );
        request->send( 200, "application/json", alertsStr );
        free( ( void * ) alert );
        cJSON_Delete( root );
        return;
      }
      request->send( 500, "text/plain", "requested alert not found" );
    }
    request->send( 500, "text/plain", "error while request one alert" );
  }

  /**
   * ask for devices, found from mdns
   */
  void AlWebServer::apiDevicesGetHandler( AsyncWebServerRequest *request )
  {
    //
    // create an json Array for devices
    //
    cJSON *root = cJSON_CreateArray();
    //
    // for all devices
    //
    for ( auto device = StatusObject::devList.begin(); device != StatusObject::devList.end(); device++ )
    {
      //
      // create json Object for root Array and every device
      //
      cJSON *devObj = cJSON_CreateObject();
      //
      // create items in device object
      //
      cJSON_AddStringToObject( devObj, "name", ( *device )->name.c_str() );
      cJSON_AddStringToObject( devObj, "id", ( *device )->id.c_str() );
      cJSON_AddStringToObject( devObj, "type", ( *device )->type.c_str() );
      cJSON_AddStringToObject( devObj, "note", ( *device )->note.c_str() );
      // cJSON_AddStringToObject( devObj, "ip", (*device)->note.c_str() );
      // cJSON_AddStringToObject( devObj, "webport", (*device)->note.c_str() );
      // cJSON_AddStringToObject( devObj, "wsport", (*device)->note.c_str() );
      cJSON_AddItemToArray( root, devObj );
    }
    const char *devices = cJSON_Print( root );
    String devicesStr( devices );
    request->send( 200, "application/json", devicesStr );
    free( ( void * ) devices );
    cJSON_Delete( root );
  }

  /**
   * deliver a file (physic pathname on the controller) via http
   */
  void AlWebServer::deliverFileToHttpd( String &filePath, AsyncWebServerRequest *request )
  {
    using namespace logger;

    String contentType( "text/plain" );
    String contentTypeMarker{ 0 };

    if ( !StatusObject::getIsSpiffsOkay() )
    {
      elog.log( WARNING, "%s: SPIFFS not initialized, send file ABORT!", AlWebServer::tag );
      request->send( 500, "text/plain", "SPIFFS not initialized" );
      return;
    }
    //
    // next check if filename not exits
    // do this after file check, so i can overwrite this
    // behavior if an file is exist
    //
    if ( !SPIFFS.exists( filePath ) )
    {
      return AlWebServer::handleNotPhysicFileSources( filePath, request );
    }
    //
    // set content type of file
    //
    contentTypeMarker = AlWebServer::setContentTypeFromFile( contentType, filePath );
    elog.log( DEBUG, "%s: file <%s>: type: <%s>...", AlWebServer::tag, filePath.c_str(), contentTypeMarker.c_str() );
    //
    // send via http server, he mak this chunked if need
    //
    AsyncWebServerResponse *response = request->beginResponse( SPIFFS, filePath, contentType, false );
    response->addHeader( "Server", "ESP Environment Server" );
    if ( contentTypeMarker.equals( "js.gz" ) )
    {
      response->addHeader( "Content-Encoding", "gzip" );
    }
    request->send( response );
  }

  /**
   * handle non-physical files
   */
  void AlWebServer::handleNotPhysicFileSources( String &filePath, AsyncWebServerRequest *request )
  {
    // TODO: implemtieren von virtuellen datenpdaden
    AlWebServer::onNotFound( request );
  }

  /**
   * if there is an server error
   */
  void AlWebServer::onServerError( AsyncWebServerRequest *request, int errNo, const String &msg )
  {
    StatusObject::setHttpActive( true );
    String myUrl( request->url() );
    elog.log( logger::ERROR, "%s: Server ERROR: %03d - %s", AlWebServer::tag, errNo, msg.c_str() );
    request->send( errNo, "text/plain", msg );
  }

  /**
   * File-Not-Fopuind Errormessage
   */
  void AlWebServer::onNotFound( AsyncWebServerRequest *request )
  {
    StatusObject::setHttpActive( true );
    String myUrl( request->url() );
    elog.log( logger::WARNING, "%s: url not found <%s>", AlWebServer::tag, myUrl.c_str() );
    request->send( 404, "text/plain", "URL not found: <" + myUrl + ">" );
  }

  /**
   * find out what is the content type fron the file extension
   */
  String AlWebServer::setContentTypeFromFile( String &contentType, const String &filename )
  {
    String type = "text";

    if ( filename.endsWith( ".pdf" ) )
    {
      type = "pdf";
      contentType = "application/pdf";
      return type;
    }
    if ( filename.endsWith( ".html" ) )
    {
      type = "html";
      contentType = "text/html";
      return type;
    }
    if ( filename.endsWith( ".jpeg" ) )
    {
      type = "jpeg";
      contentType = "image/jpeg";
      return type;
    }
    if ( filename.endsWith( ".ico" ) )
    {
      type = "icon";
      contentType = "image/x-icon";
      return type;
    }
    if ( filename.endsWith( ".json" ) )
    {
      type = "json";
      contentType = "application/json";
      return type;
    }
    if ( filename.endsWith( ".jdata" ) )
    {
      // my own marker for my "raw" fileformat
      type = "jdata";
      contentType = "application/json";
      return type;
    }
    if ( filename.endsWith( "js.gz" ) )
    {
      type = "js.gz";
      contentType = "text/javascript";
      return type;
    }
    if ( filename.endsWith( ".js" ) )
    {
      type = "js";
      contentType = "text/javascript";
      return type;
    }
    if ( filename.endsWith( ".css" ) )
    {
      type = "css";
      contentType = "text/css";
      return type;
    }
    //
    // This is a limited set only
    // For any other type always set as plain text
    //
    contentType = "text/plain";
    return type;
  }

}  // namespace webserver
