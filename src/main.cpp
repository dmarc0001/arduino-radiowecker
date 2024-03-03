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
// #include "soundtouchDevice.hpp"
#include "soundTouchAlert.hpp"

void setup()
{
  using namespace alarmclock;
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
  // set my timezone, i deal with timestamps
  elog.log( INFO, "%s: set timezone (%s)...", tag, appprefs::LocalPrefs::getTimeZone().c_str() );
  setenv( "TZ", appprefs::LocalPrefs::getTimeZone().c_str(), 1 );
  tzset();
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
  //
  // DEBUG: testalert add
  //
  addTestAlert();
  //
  // DEBUG: terstalert add
  //
  elog.log( DEBUG, "%s: start alert task...", tag );
  AlertTask::start();
  DeviceDiscover::init();
}

void addTestAlert()
{
  using namespace alarmclock;
  using namespace logger;
  const char *tag{ "testalert" };

  //
  // step 1, create and init a soundTouch device
  //
  elog.log( DEBUG, "%s: create soundtouch testalert alert with device...", tag );
  using namespace soundtouch;
  using namespace alarmclock;
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
  AlertEntryPtr alert = std::make_shared< AlertEntry >();
  alert->name = "alert_00";                                   //! name of the alert
  alert->volume = 22;                                         //! volume to weak up
  alert->location = "";                                       //! have to read in manual api
  alert->source = "PRESET_1";                                 //! preset or string to source
  alert->raiseVol = true;                                     //! should volume raisng on? down
  alert->duration = 120;                                      //! length in secounds
  alert->days = { mo, tu, we, tu, fr, sa, su };               //! if present, days to alert
  alert->devices.push_back( device.id );                      //! which devices?
  alert->enable = true;                                       //! alert enable?
  alert->note = "Test alert 001";                             //! user note (cause etc)
  alert->alertHour = static_cast< uint8_t >( lt->tm_min );    //! which hour wake up
  alert->alertMinute = static_cast< uint8_t >( lt->tm_min );  //! which minute wake up
                                                              // uint8_t day;                //! if on a day, day number (1-31)
                                                              // uint8_t month;              //! if on day, month number (1-12)
                                                              //

  //
  // put alert in current List
  //
  StatusObject::alertList.push_back( alert );
}

void testLoop( std::shared_ptr< soundtouch::SoundTouchAlert > testAlert )
{
  using namespace soundtouch;
  using namespace logger;
  //
  static int64_t timeoutTime{ ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) ) };
  static SoundTouchDeviceRunningMode oldMode{ ST_STATE_UNKNOWN };

  if ( !testAlert )
    return;
  switch ( testAlert->getDeviceRunningMode() )
  {
    case ST_STATE_UNINITIALIZED:
      if ( oldMode != ST_STATE_UNINITIALIZED )
      {
        // first time, timeout activating
        timeoutTime = ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) );
        oldMode != ST_STATE_UNINITIALIZED;
      }
      break;
    case ST_STATE_GET_INFOS:
      if ( oldMode != ST_STATE_GET_INFOS )
      {
        // first time, timeout activating
        timeoutTime = ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) );
        oldMode != ST_STATE_GET_INFOS;
      }
      // wait if timeout or next level
      if ( timeoutTime < esp_timer_get_time() )
      {
        // timeout!
        elog.log( ERROR, "main: device init TIMEOUT, Abort alert!" );
        testAlert.reset();
        return;
      }
      break;
    case ST_STATE_INIT_ALERT:
      oldMode = ST_STATE_INIT_ALERT;
      testAlert->prepareAlertDevivce();
      timeoutTime = esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT );
      break;
    case ST_STATE_WAIT_FOR_INIT_COMLETE:
      oldMode = ST_STATE_WAIT_FOR_INIT_COMLETE;
      // timeout
      if ( timeoutTime < esp_timer_get_time() )
      {
        // timeout!
        elog.log( ERROR, "main: device prepare TIMEOUT, Abort alert!" );
        delay( 2000 );  //! DEBUG: debug
        testAlert.reset();
        return;
      }
      break;
    case ST_STATE_RUNNING_ALERT:
      if ( oldMode != ST_STATE_RUNNING_ALERT )
      {
        // first time, timeout activating
        timeoutTime = ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) );
        oldMode != ST_STATE_RUNNING_ALERT;
      }
      // test if checkRunning Alert is false and timeout is over
      if ( !testAlert->checkRunningAlert() && ( timeoutTime < esp_timer_get_time() ) )
      {
        // timeout!
        elog.log( ERROR, "main: device running TIMEOUT or Alert aborted" );
        delay( 20 );  //! DEBUG: debug
        testAlert.reset();
        return;
      }
      break;
    default:
      // testAlert->checkRunningAlert();
      break;
  }
}

void loop()
{
  using namespace alarmclock;
  using namespace logger;

  // next time logger time sync
  static int64_t setNextTimeCorrect{ ( esp_timer_get_time() + getMicrosForSec( 21600 ) ) };
  static auto connected = WlanState::DISCONNECTED;
  //
  // for webserver
  //
  // EnvServer::WifiConfig::wm.process();
  if ( setNextTimeCorrect < esp_timer_get_time() )
  {
    //
    // somtimes correct elog time
    //
    elog.log( DEBUG, "main: logger time correction..." );
    setNextTimeCorrect = esp_timer_get_time() + getMicrosForSec( 21600 );
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
    yield();
  }
  //
  // check if the state chenged
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
        // TODO: EnvWebServer::stop();
        break;
      case WlanState::CONNECTED:
        elog.log( INFO, "main: ip connectivity found, start webserver." );
        // TODO: EnvWebServer::start();
        break;
      case WlanState::TIMESYNCED:
        elog.log( INFO, "main: timesynced. enable alerts..." );
        break;
    }
    // mark new value
    connected = new_connected;
  }
  yield();
}
