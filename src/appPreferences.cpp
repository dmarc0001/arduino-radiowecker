#include "appPreferences.hpp"
#include "common.hpp"

namespace appprefs
{
  constexpr const char *CHECKVAL{ "confInit" };
  constexpr const char *INITVAL{ "wasInit" };
  constexpr const char *SYSLOGSRV{ "syslogServer" };
  constexpr const char *SYSLOGPORT{ "syslogPort" };
  constexpr const char *LOCAL_TIMEZONE_OFFSET{ "timezone-offset" };
  constexpr const char *LOCAL_TIMEZONE{ "locTimeZone" };
  constexpr const char *LOCAL_HOSTNAME{ "hostName" };
  constexpr const char *DEBUGSETTING{ "debugging" };

  Preferences LocalPrefs::lPref;
  bool LocalPrefs::wasInit{ false };
  const char *LocalPrefs::tag{ "LocalPrefs" };
  const String LocalPrefs::alertConfigFile( ALERT_CONFIG );
  const String LocalPrefs::devicesConfigfile( DEVICES_CONFIG );

  /**
   * init the static preferences objectg
   */
  void LocalPrefs::init()
  {
    using namespace logger;

    LocalPrefs::lPref.begin( APPNAME, false );
    if ( !LocalPrefs::getIfPrefsInit() )
    {
      Serial.println( "first-time-init preferences..." );
      char hostname[ 32 ];
      uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
      snprintf( hostname, 32, "%s-%08X", appprefs::DEFAULT_HOSTNAME, chip );
      String hn( &hostname[ 0 ] );
      LocalPrefs::lPref.putUInt( SYSLOGSRV, 0U );
      LocalPrefs::lPref.putUShort( SYSLOGPORT, ( uint16_t ) 514 );
      LocalPrefs::lPref.putLong( LOCAL_TIMEZONE_OFFSET, appprefs::BUGFIX_TIMEZONE_OFFSET );
      LocalPrefs::lPref.putString( LOCAL_TIMEZONE, "GMT" );
      LocalPrefs::lPref.putString( LOCAL_HOSTNAME, hn );
      LocalPrefs::lPref.putUChar( DEBUGSETTING, logger::DEBUG );
      LocalPrefs::setIfPrefsInit( true );
      Serial.println( "first-time-init preferences...DONE" );
    }
    LocalPrefs::wasInit = true;
  }

  /**
   * return ip add from syslog server, if 0 0> not valid
   */
  IPAddress LocalPrefs::getSyslogServer()
  {
    return ( IPAddress( LocalPrefs::lPref.getUInt( SYSLOGSRV ) ) );
  }

  /**
   * get the syslog Port
   */
  uint16_t LocalPrefs::getSyslogPort()
  {
    return ( LocalPrefs::lPref.getUShort( SYSLOGPORT ) );
  }

  /**
   * set syslog server ip addr
   */
  bool LocalPrefs::setSyslogServer( IPAddress &_addr )
  {
    return ( LocalPrefs::lPref.putUInt( SYSLOGSRV, _addr ) == 4 );
  }

  /**
   * set the syslog port, return true if okay
   */
  bool LocalPrefs::setSyslogPort( uint16_t _port )
  {
    return ( LocalPrefs::lPref.putUShort( SYSLOGPORT, _port ) == 2 );
  }

  /**
   * get the offset to GMT
   */
  long LocalPrefs::getTimezoneOffset()
  {
    return ( LocalPrefs::lPref.getLong( LOCAL_TIMEZONE_OFFSET, appprefs::BUGFIX_TIMEZONE_OFFSET ) );
  }

  /**
   * set the offset to GMT
   */
  bool LocalPrefs::setTimezoneOffset( long _offset )
  {
    configTime( _offset, 0, NTP_POOL_01, NTP_POOL_02 );
    return ( LocalPrefs::lPref.putLong( LOCAL_TIMEZONE_OFFSET, _offset ) );
  }

  /**
   * get local timezone
   */
  // String LocalPrefs::getTimeZone()
  // {
  //   return ( LocalPrefs::lPref.getString( LOCAL_TIMEZONE, "GMT" ) );
  // }

  /**
   * set local timezone
   */
  // bool LocalPrefs::setTimeZone( String &_zone )
  // {
  //   return ( LocalPrefs::lPref.putString( LOCAL_TIMEZONE, _zone ) > 0 );
  // }

  /**
   * get the local hostname
   */
  String LocalPrefs::getHostName()
  {
    return ( LocalPrefs::lPref.getString( LOCAL_HOSTNAME, appprefs::DEFAULT_HOSTNAME ) );
  }

  /**
   * set the local hostname
   */
  bool LocalPrefs::setHostName( String &_hostname )
  {
    return ( LocalPrefs::lPref.putString( LOCAL_HOSTNAME, _hostname ) > 0 );
  }

  uint8_t LocalPrefs::getLogLevel()
  {
#ifdef BUILD_DEBUG
    return ( LocalPrefs::lPref.getUChar( DEBUGSETTING, logger::DEBUG ) );
#else
    return ( LocalPrefs::lPref.getUChar( DEBUGSETTING, logger::INFO ) );
#endif
  }

  bool LocalPrefs::setLogLevel( uint8_t _set )
  {
    return ( LocalPrefs::lPref.putUChar( DEBUGSETTING, _set ) == 1 );
  }

  /**
   * Check if preferences was initialized once
   */
  bool LocalPrefs::getIfPrefsInit()
  {
    using namespace logger;

    String defaultVal( "-" );
    String correctVal( INITVAL );
    return ( correctVal == LocalPrefs::lPref.getString( CHECKVAL, defaultVal ) );
  }

  /**
   * set if prefs was initialized
   */
  bool LocalPrefs::setIfPrefsInit( bool _set )
  {
    // using namespace EnvServer;
    // using namespace logger;

    if ( _set )
    {
      // elog.log( INFO, "%s: setIfPrefsInit set <%s> to <%s> (%s)...", LocalPrefs::tag, CHECKVAL, INITVAL );
      // if set => set the correct value
      return ( LocalPrefs::lPref.putString( CHECKVAL, INITVAL ) > 0 );
    }
    // else remove the key, set the properties not valid
    // elog.log( INFO, "%s: setIfPrefsInit remove  <%s>...", LocalPrefs::tag, CHECKVAL );
    return LocalPrefs::lPref.remove( CHECKVAL );
  }

}  // namespace appprefs