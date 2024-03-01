#pragma once
#include <IPAddress.h>
#include <FastLED.h>
#include <esp_wifi.h>
#include <stdint.h>
#include <string>
#include "eLog.hpp"
#include <Preferences.h>

constexpr int64_t getMicrosForSec( int32_t _sec )
{
  return ( 1000000LL * static_cast< int64_t >( _sec ) );
}
constexpr int64_t getMicrosForMiliSec( int32_t _sec )
{
  return ( 1000LL * static_cast< int64_t >( _sec ) );
}

namespace appprefs
{
  //
  // some values are depend from compiling mode...
  //
  constexpr gpio_num_t RANDOM_INIT_PORT = GPIO_NUM_0;          //! port for init random
  constexpr const char *DEFAULT_HOSTNAME{ "alert" };           //! default hostname
  constexpr const char *APPNAME{ "alert-app" };                //! app name
  constexpr const char *ALERT_CONFIG{ "/alert.json" };         //! json file, includes alerts
  constexpr const char *DEVICES_CONFIG{ "/devices.json" };     // json file describes devices
  constexpr const uint16_t SYSLOG_PRIO{ 8 };                   //! standart syslog prio (user)
  constexpr const uint16_t SYSLOG_PROTO{ 0 };                  //! standart syslog protocol (IETF)
  constexpr const char *WEB_PATH{ "/spiffs" };                 //! virtual path wegserver
  constexpr const char *WEB_PARTITION_LABEL{ "mydata" };       //! label of the spiffs or null
  constexpr uint32_t WIFI_MAXIMUM_RETRY{ 5 };                  //! Max connection retries
  constexpr wifi_auth_mode_t WIFI_AUTH{ WIFI_AUTH_WPA2_PSK };  //! wifi auth method
  constexpr uint8_t LED_STRIPE_COUNT = 1;                      //! count of LED
  constexpr gpio_num_t LED_STRIPE_RMT_TX_GPIO = GPIO_NUM_18;   //! control pin GPIO für led control
  constexpr int LED_STRIP_BRIGHTNESS = 255;                    //! brightness led stripe
  constexpr EOrder LED_RGB_ORDER = GRB;                        //! what order is the red/green/blue byte
  constexpr uint32_t LED_STRIP_RESOLUTION_HZ = 10000000;       //! 10MHz resolution, 1 tick = 0.1us
  constexpr uint8_t LED_STATUS = 0;                            //! indicator WLAN
  constexpr uint8_t LED_ALL = 255;                             //! indicator means all led'S
  constexpr int32_t TASK_MARK_INTERVAL_MS = 43000;             //! interval between "mark" in tasks for debuging

  class LocalPrefs
  {
    private:
    static const char *tag;    //! logging tag
    static bool wasInit;       //! was the prefs object initialized?
    static Preferences lPref;  // static preferences object

    public:
    static const String alertConfigFile;
    static const String devicesConfigfile;

    public:
    static void init();                          //! init the preferences Object and Preferences
    static IPAddress getSyslogServer();          //! get syslog server ip
    static uint16_t getSyslogPort();             //! get syslog pornum
    static bool setSyslogServer( IPAddress & );  //! set syslog server ipo
    static bool setSyslogPort( uint16_t );       //! set syslog portnum
    static String getTimeZone();                 //! get my timezone
    static bool setTimeZone( String & );         //! set my timezone
    static String getHostName();                 //! get my own hostname
    static bool setHostName( String & );         //! set my Hostname
    static uint8_t getLogLevel();                //! get Logging
    static bool setLogLevel( uint8_t );          //! set Logging

    private:
    static bool getIfPrefsInit();        //! internal, is preferences initialized?
    static bool setIfPrefsInit( bool );  //! internal, set preferences initialized?
  };
}  // namespace appprefs
