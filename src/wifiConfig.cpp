#include <Esp.h>
#include "wifiConfig.hpp"
#include "statusObject.hpp"
#include "appPreferences.hpp"

namespace alertclock
{
  using namespace logger;

  const char *WifiConfig::tag{ "wificonf" };
  bool WifiConfig::is_sntp_init{ false };
  WiFiManager WifiConfig::wm;
  WiFiManagerParameter WifiConfig::custom_field;

  void WifiConfig::init()
  {
    char hostname[ 32 ];
    elog.log( INFO, "%s: initialize wifi...", WifiConfig::tag );
    uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
    snprintf( hostname, 32, "%s-%08X", appprefs::DEFAULT_HOSTNAME, chip );
    WiFi.setHostname( hostname );
    WiFi.mode( WIFI_STA );
    WiFi.onEvent( WifiConfig::wifiEventCallback );
    // reset settings - wipe credentials for testing
    // wm.resetSettings();
    // WifiConfig::wm.setConfigPortalBlocking( false );
    WifiConfig::wm.setConfigPortalBlocking( true );
    WifiConfig::wm.setConnectTimeout( 20 );
    //
    // esp32 time config
    // BUG: timezone not work, using gmt offset
    configTime( appprefs::LocalPrefs::getTimezoneOffset(), 0, appprefs::NTP_POOL_01, appprefs::NTP_POOL_02 );
    // the old way...
    // sntp_set_sync_mode( SNTP_SYNC_MODE_IMMED );
    // sntp_setoperatingmode( SNTP_OPMODE_POLL );
    // sntp_setservername( 1, "pool.ntp.org" );
    //
    // set an callback for my reasons
    //
    sntp_set_time_sync_notification_cb( WifiConfig::timeSyncNotificationCallback );
    if ( WifiConfig::wm.autoConnect( "EnvServerConfigAP" ) )
    {
      elog.log( INFO, "%s: wifi connected...", WifiConfig::tag );
      StatusObject::setWlanState( WlanState::CONNECTED );
      elog.log( DEBUG, "%s: try to sync time...", WifiConfig::tag );
      sntp_init();
      WifiConfig::is_sntp_init = true;
      WifiConfig::wm.stopWebPortal();
    }
    else
    {
      elog.log( WARNING, "%s: wifi not connected, access point running...", WifiConfig::tag );
      StatusObject::setWlanState( WlanState::DISCONNECTED );
      // WifiConfig::wm.setAPCallback( WifiConfig::configModeCallback );
      // set dark mode
      WifiConfig::wm.setClass( "invert" );
    }
    elog.log( INFO, "%s: initialize wifi...OK", WifiConfig::tag );
  }

  void WifiConfig::wifiEventCallback( arduino_event_t *event )
  {
    using namespace logger;

    switch ( event->event_id )
    {
      case SYSTEM_EVENT_STA_CONNECTED:
        elog.log( INFO, "%s: device connected to accesspoint...", WifiConfig::tag );
        // if ( StatusObject::getWlanState() == WlanState::DISCONNECTED )
        //   StatusObject::setWlanState( WlanState::CONNECTED );
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        elog.log( INFO, "%s: device disconnected from accesspoint...", WifiConfig::tag );
        StatusObject::setWlanState( WlanState::DISCONNECTED );
        elog.setSyslogOnline( false );
        break;
      case SYSTEM_EVENT_AP_STADISCONNECTED:
        elog.log( INFO, "%s: WIFI client disconnected...", WifiConfig::tag );
        sntp_stop();
        WifiConfig::is_sntp_init = false;
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        elog.log( INFO, "%s: device got ip <%s>...", WifiConfig::tag, WiFi.localIP().toString().c_str() );
        if ( StatusObject::getWlanState() == WlanState::DISCONNECTED )
          StatusObject::setWlanState( WlanState::CONNECTED );
        // sntp_init();
        if ( WifiConfig::is_sntp_init )
          sntp_restart();
        else
        {
          sntp_init();
          WifiConfig::is_sntp_init = true;
        }
        elog.setSyslogOnline( true );
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        elog.log( INFO, "%s: device lost ip...", WifiConfig::tag );
        StatusObject::setWlanState( WlanState::DISCONNECTED );
        sntp_stop();
        WifiConfig::is_sntp_init = false;
        elog.setSyslogOnline( false );
        break;
      default:
        break;
    }
  }

  void WifiConfig::timeSyncNotificationCallback( struct timeval * )
  {
    using namespace logger;

    sntp_sync_status_t state = sntp_get_sync_status();
    switch ( state )
    {
      case SNTP_SYNC_STATUS_COMPLETED:
        elog.log( INFO, "%s: notification: time status sync completed!", WifiConfig::tag );
        if ( StatusObject::getWlanState() == WlanState::CONNECTED )
        {
          StatusObject::setWlanState( WlanState::TIMESYNCED );
        }
        struct tm ti;
        if ( !getLocalTime( &ti ) )
        {
          elog.log( WARNING, "%s: failed to obtain systemtime!", WifiConfig::tag );
        }
        else
        {
          elog.log( DEBUG, "%s: gotten system time!", WifiConfig::tag );
          Elog::provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
        }
        break;
      default:
        elog.log( INFO, "%s: notification: time status NOT sync completed!", WifiConfig::tag );
        if ( StatusObject::getWlanState() == WlanState::TIMESYNCED )
        {
          StatusObject::setWlanState( WlanState::CONNECTED );
          if ( WifiConfig::is_sntp_init )
            sntp_restart();
          else
          {
            sntp_init();
            WifiConfig::is_sntp_init = true;
          }
        }
    }
  }

  void WifiConfig::configModeCallback( WiFiManager *myWiFiManager )
  {
    using namespace logger;

    elog.log( INFO, "%s: config callback, enter config mode...", WifiConfig::tag );
    IPAddress apAddr = WiFi.softAPIP();
    elog.log( INFO, "%s: config callback: Access Point IP: <%s>...", WifiConfig::tag, apAddr.toString() );
    auto pSSID = myWiFiManager->getConfigPortalSSID();
    elog.log( DEBUG, "%s: config callback: AP SSID: <%s>...", pSSID.c_str() );
    elog.log( INFO, "%s: config callback...OK", WifiConfig::tag );
  }

}  // namespace alertclock
