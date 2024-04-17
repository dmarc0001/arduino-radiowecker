/*
  main, hier startet es
*/
#include <memory>
#include "eLog.hpp"
#include "appPreferences.hpp"
#include "appStructs.hpp"
#include "common.hpp"
#include "main.hpp"
#include "statusObject.hpp"
#include "wifiConfig.hpp"
#include "alertTask.hpp"
#include "ledStripe.hpp"
#include "alertConfigFileObj.hpp"
#include "deviceDiscover.hpp"
#include "soundTouchAlert.hpp"
#include "webServer.hpp"

void setup()
{
  using namespace alertclock;
  using namespace soundtouch;
  using namespace logger;

  const char *tag{ "main" };

  //
  // init the random generator
  //
  adcAttachPin( appprefs::RANDOM_INIT_PORT );
  randomSeed( analogRead( appprefs::RANDOM_INIT_PORT ) );
  //
  // Debug Ausgabe initialisieren
  //
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  // read persistent config
  Serial.println( "main: init local preferences..." );
  appprefs::LocalPrefs::init();
  // correct loglevel
  Loglevel level = static_cast< Loglevel >( appprefs::LocalPrefs::getLogLevel() );
  elog.addSerialLogging( Serial, "MAIN", level );  // Enable serial logging. We want only INFO or lower logleve.
  elog.setSyslogOnline( false );
  elog.addSyslogLogging( level );
  elog.log( INFO, "main: start with logging..." );
  // BUG: do not work by esp32-s2
  // set my timezone, i deal with timestamps
  // elog.log( DEBUG, "main: set timezone (%s)...", appprefs::LocalPrefs::getTimeZone().c_str() );
  // set my timezone, i deal with timestamps
  // setenv( "TZ", appprefs::LocalPrefs::getTimeZone().c_str(), 1 );
  // tzset();
  static String hName( appprefs::LocalPrefs::getHostName() );
  elog.log( INFO, "%s: hostname: <%s>...", tag, hName.c_str() );
  //
  // check if syslog and/or datalog present
  //
  IPAddress addr( appprefs::LocalPrefs::getSyslogServer() );
  uint16_t port( appprefs::LocalPrefs::getSyslogPort() );
  elog.log( INFO, "%s: syslog %s:%d", tag, addr.toString().c_str(), port );
  if ( ( addr > 0 ) && ( port > 0 ) )
  {
    elog.log( INFO, "%s: init syslog protocol to %s:%d...", tag, addr.toString().c_str(), port );
    elog.setUdpClient( udpClient, addr, port, hName.c_str(), appprefs::APPNAME, appprefs::SYSLOG_PRIO, appprefs::SYSLOG_PROTO );
  }
  elog.log( DEBUG, "%s: init StatusObject...", tag );
  StatusObject::init();
  elog.log( DEBUG, "%s: init LED Stripe...", tag );
  LEDStripe::init();
  elog.log( DEBUG, "%s: start wifi...", tag );
  WifiConfig::init();
  elog.log( INFO, "%s: read startup config...", tag );
  AlertConfObj::readConfig();
  elog.log( DEBUG, "%s: start alert task...", tag );
  AlertTask::start();
  elog.log( DEBUG, "%s: start device discover task...", tag );
  DeviceDiscover::init();
  elog.log( DEBUG, "%s: init webserver...", tag );
  webserver::AlWebServer::init();
}

void loop()
{
  using namespace alertclock;
  using namespace logger;

  // next time logger time sync
  static int64_t setNextTimeCorrect{ ( esp_timer_get_time() + getMicrosForSec( 21600 ) ) };
  static auto connected = WlanState::DISCONNECTED;
  //
  // for webserver
  //
  if ( setNextTimeCorrect < esp_timer_get_time() )
  {
    //
    // somtimes correct elog time
    //
    elog.log( DEBUG, "main: logger time correction..." );
    setNextTimeCorrect = esp_timer_get_time() + getMicrosForSec( 21600 );
    setLoggerTime();
    yield();
  }
  //
  // for config saving
  //
  if ( StatusObject::getWasConfigChanged() )
  {
    if ( StatusObject::getWasConfigChanged() < esp_timer_get_time() )
    {
      //
      // check if the config was changed
      //
      if ( StatusObject::getWasConfigChanged() )
      {
        StatusObject::setWasConfigChanged( false );
        yield();
        AlertConfObj::saveConfig();
        yield();
      }
    }
  }
  //
  // check if the state changed
  //
  if ( connected != StatusObject::getWlanState() )
  {
    // connection state has changed
    auto new_connected = StatusObject::getWlanState();
    switch ( new_connected )
    {
      case WlanState::DISCONNECTED:
      case WlanState::FAILED:
      case WlanState::SEARCHING:
        // it was with connection, now without
        elog.log( WARNING, "main: ip connectivity lost, stop webserver." );
        webserver::AlWebServer::stop();
        break;
      case WlanState::CONNECTED:
        elog.log( INFO, "main: ip connectivity found, start webserver." );
        webserver::AlWebServer::start();
        break;
      case WlanState::TIMESYNCED:
        webserver::AlWebServer::start();
        elog.log( INFO, "main: timesynced. enable alerts..." );
        //
        // DEBUG: testalert add
        //
        addTestAlert();
        break;
    }
    // mark new value
    connected = new_connected;
  }
  yield();
}

void addTestAlert()
{
  using namespace alertclock;
  using namespace logger;
  const char *tag{ "testalert" };

  //
  // step 1, create and init a soundTouch device
  //
  elog.log( DEBUG, "%s: create soundtouch testalert alert with device...", tag );
  using namespace soundtouch;
  using namespace alertclock;
  DeviceEntry device;
  IPAddress addr;
  addr.fromString( "192.168.1.68" );
  device.name = String( "Arbeitszimmer" );
  device.addr = addr;
  device.id = String( "689E19653E96" );
  device.webPort = 8090;
  device.wsPort = 8080;
  device.type = String( "Soundtouch" );
  device.note = String( "TESTALERTDEVICE" );

  //
  // i want to make the alert in two minutes after reset
  //
  time_t now;
  time( &now );
  tm *lt = localtime( &now );
  if ( lt->tm_min > 57 )
  {
    ++lt->tm_hour;
    lt->tm_min = 0;
  }
  else
  {
    lt->tm_min += 2;
  }
  //
  // create testalert
  //
  AlertEntryPtr alert = std::make_shared< AlertEntry >( "alert_98" );
  alert->volume = 21;                                         //! volume to weak up
  alert->location = "";                                       //! have to read in manual api
  alert->source = "PRESET_1";                                 //! preset or string to source
  alert->raiseVol = true;                                     //! should volume raisng on? down
  alert->duration = 90;                                       //! length in secounds
  alert->days = { mo, tu, we, tu, fr, sa, su };               //! if present, days to alert
  alert->devices.push_back( device.id );                      //! which devices?
  alert->enable = true;                                       //! alert enable?
  alert->note = "Test alert 001 with raise vol";              //! user note (cause etc)
  alert->alertHour = static_cast< uint8_t >( lt->tm_hour );   //! which hour wake up
  alert->alertMinute = static_cast< uint8_t >( lt->tm_min );  //! which minute wake up
                                                              // uint8_t day;                //! if on a day, day number (1-31)
                                                              // uint8_t month;              //! if on day, month number (1-12)
                                                              //

  //
  // put alert in current List
  //
  elog.log( INFO, "main: test alert 01 dayly at <%02d:%02d> at device <%s>", alert->alertHour, alert->alertMinute,
            alert->devices.front() );
  StatusObject::alertList.push_back( alert );

  //
  // i want to make the alert in two minutes after reset
  //
  time( &now );
  lt = localtime( &now );
  if ( lt->tm_min > 52 )
  {
    ++lt->tm_hour;
    lt->tm_min = 0;
  }
  else
  {
    lt->tm_min += 5;
  }
  //
  // create testalert
  //
  alert = std::make_shared< AlertEntry >( "alert_99" );
  alert->volume = 23;                                         //! volume to weak up
  alert->location = "";                                       //! have to read in manual api
  alert->source = "PRESET_4";                                 //! preset or string to source
  alert->raiseVol = false;                                    //! should volume raisng on? down
  alert->duration = 90;                                       //! length in secounds
  alert->days = { mo, tu, we, th, fr, sa, su };               //! if present, days to alert
  alert->devices.push_back( device.id );                      //! which devices?
  alert->enable = true;                                       //! alert enable?
  alert->note = "Test alert 002-without raise vol ";          //! user note (cause etc)
  alert->alertHour = static_cast< uint8_t >( lt->tm_hour );   //! which hour wake up
  alert->alertMinute = static_cast< uint8_t >( lt->tm_min );  //! which minute wake up
                                                              // uint8_t day;                //! if on a day, day number (1-31)
                                                              // uint8_t month;              //! if on day, month number (1-12)
                                                              //

  //
  // put alert in current List
  //
  elog.log( INFO, "main: test alert 02 dayly at <%02d:%02d> at device <%s>", alert->alertHour, alert->alertMinute,
            alert->devices.front() );
  StatusObject::alertList.push_back( alert );
}

void setLoggerTime()
{
  using namespace logger;
  //
  struct tm ti;
  if ( !getLocalTime( &ti ) )
  {
    elog.log( WARNING, "main: failed to obtain system time!" );
  }
  else
  {
    elog.log( DEBUG, "main: gotten system time!" );
    Elog::provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
  }
}
