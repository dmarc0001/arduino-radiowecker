/*
  main, hier startet es
*/
#include "elog/eLog.hpp"
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

void setup()
{
  using namespace AlarmClockSrv;
  using namespace soundtouch;

  using namespace logger;

  const char *tag{ "main" };
  //
  // Debug Ausgabe initialisieren
  //
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  // read persistent config
  Serial.println( "main: init local preferences..." );
  Prefs::LocalPrefs::init();
  // correct loglevel
  Loglevel level = static_cast< Loglevel >( Prefs::LocalPrefs::getLogLevel() );
  elog.addSerialLogging( Serial, "MAIN", level );  // Enable serial logging. We want only INFO or lower logleve.
  elog.setSyslogOnline( false );
  elog.addSyslogLogging( level );
  elog.log( INFO, "main: start with logging..." );
  // set my timezone, i deal with timestamps
  elog.log( INFO, "%s: set timezone (%s)...", tag, Prefs::LocalPrefs::getTimeZone().c_str() );
  setenv( "TZ", Prefs::LocalPrefs::getTimeZone().c_str(), 1 );
  tzset();
  static String hName( Prefs::LocalPrefs::getHostName() );
  elog.log( INFO, "%s: hostname: <%s>...", tag, hName.c_str() );
  //
  // check if syslog and/or datalog present
  //
  IPAddress addr( Prefs::LocalPrefs::getSyslogServer() );
  uint16_t port( Prefs::LocalPrefs::getSyslogPort() );
  elog.log( INFO, "%s: syslog %s:%d", tag, addr.toString().c_str(), port );
  if ( ( addr > 0 ) && ( port > 0 ) )
  {
    elog.log( INFO, "%s: init syslog protocol to %s:%d...", tag, addr.toString().c_str(), port );
    elog.setUdpClient( udpClient, addr, port, hName.c_str(), Prefs::APPNAME, Prefs::SYSLOG_PRIO, Prefs::SYSLOG_PROTO );
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
  DeviceDiscover::init();
}

void loop()
{
  using namespace AlarmClockSrv;
  using namespace logger;

  // next time logger time sync
  static unsigned long setNextTimeCorrect{ ( millis() + ( 1000UL * 21600UL ) ) };
  static auto connected = WlanState::DISCONNECTED;
  //
  // for webserver
  //
  // EnvServer::WifiConfig::wm.process();
  if ( setNextTimeCorrect < millis() )
  {
    //
    // somtimes correct elog time
    //
    elog.log( DEBUG, "main: logger time correction..." );
    setNextTimeCorrect = ( millis() * 1000UL * 21600UL );
    struct tm ti;
    if ( !getLocalTime( &ti ) )
    {
      elog.log( WARNING, "main: failed to obtain system time!" );
    }
    else
    {
      elog.log( DEBUG, "main: gotten system time!" );
      Elog::provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
      elog.log( DEBUG, "main: logger time correction...OK" );
    }
  }
  delay( 600 );
  if ( connected != StatusObject::getWlanState() )
  {
    auto new_connected = StatusObject::getWlanState();
    if ( connected == WlanState::DISCONNECTED || connected == WlanState::FAILED || connected == WlanState::SEARCHING )
    {
      //
      // was not functional for webservice
      //
      if ( new_connected == WlanState::CONNECTED || new_connected == WlanState::TIMESYNCED )
      {
        //
        // new connection, start webservice
        //
        elog.log( INFO, "main: ip connectivity found, start webserver." );
        // TODO: EnvWebServer::start();
      }
      else
      {
        elog.log( WARNING, "main: ip connectivity lost, stop webserver." );
        // TODO: EnvWebServer::stop();
      }
    }
    else
    {
      //
      // was functional for webservice
      //
      if ( !( new_connected == WlanState::CONNECTED || new_connected == WlanState::TIMESYNCED ) )
      {
        //
        // not longer functional
        //
        elog.log( WARNING, "main: ip connectivity lost, stop webserver." );
        // TODO: EnvWebServer::stop();
      }
    }
    // mark new value
    connected = new_connected;
  }
}
