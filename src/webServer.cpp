#include <memory>
#include <cJSON.h>
#include <esp_chip_info.h>
#include "webServer.hpp"
#include "statics.hpp"
#include "appPreferences.hpp"
#include "statusObject.hpp"
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
    AlWebServer::server.on( "^\\/api\\/v1\\/set-(.*)\?(.*)$", HTTP_GET, AlWebServer::onApiV1Set );
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
    if ( parameter.equals( "version" ) )
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
    String server, port;

    elog.log( logger::DEBUG, "%s: api version 1 call set-%s", AlWebServer::tag, verb );
    //
    // timezone set?
    //
    if ( verb.equals( "timezone" ) )
    {
      // timezone parameter find
      if ( request->hasParam( "timezone" ) )
      {
        String timezone = request->getParam( "timezone" )->value();
        elog.log( logger::DEBUG, "%s: set-timezone, param: %s", AlWebServer::tag, timezone.c_str() );
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
      else
      {
        elog.log( logger::ERROR, "%s: set-timezone, param not found!", AlWebServer::tag );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    if ( verb.equals( "time-offset" ) )
    {
      // timezone parameter find
      if ( request->hasParam( "timezone" ) )
      {
        String timezone = request->getParam( "timezone-offset" )->value();
        elog.log( logger::DEBUG, "%s: set-timezone-offset, param: %s", AlWebServer::tag, timezone.c_str() );
        appprefs::LocalPrefs::setTimezoneOffset( timezone.toInt() );

        // appprefs::LocalPrefs::setTimeZone( timezone );
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
        elog.log( logger::ERROR, "%s: set-timezone, param not found!", AlWebServer::tag );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    else if ( verb.equals( "loglevel" ) )
    {
      // loglevel parameter find
      if ( request->hasParam( "level" ) )
      {
        String level = request->getParam( "level" )->value();
        elog.log( logger::DEBUG, "%s: set-loglevel, param: %s", AlWebServer::tag, level.c_str() );
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
        elog.log( logger::ERROR, "%s: set-loglevel, param not found!", AlWebServer::tag );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    //
    // server/port set?
    //
    if ( request->hasParam( "server" ) )
      server = request->getParam( "server" )->value();
    if ( request->hasParam( "port" ) )
      port = request->getParam( "port" )->value();
    //
    if ( server.isEmpty() || port.isEmpty() )
    {
      //
      // not set, i can't compute this
      //
      request->send( 300, "text/plain", "fail api call v1 for <set-" + verb + "> - no server/port params" );
      return;
    }
    //
    // params to usable objects
    //
    IPAddress srv;
    srv.fromString( server );
    uint16_t srvport = static_cast< uint16_t >( port.toInt() );
    //
    // which command?
    //
    if ( verb.equals( "syslog" ) )
    {
      elog.log( logger::DEBUG, "%s: set-syslog, params: %s, %s", AlWebServer::tag, server.c_str(), port.c_str() );
      appprefs::LocalPrefs::setSyslogServer( srv );
      appprefs::LocalPrefs::setSyslogPort( srvport );
      // reboot then
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <set-" + verb + ">" );
      return;
    }
    request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + "> RESTART CONTROLLER" );
    yield();
    sleep( 2 );
    ESP.restart();
  }

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
