#include "deviceDiscover.hpp"
#include "statusObject.hpp"
#include "statics.hpp"

namespace soundtouch
{
  using namespace logger;
  using namespace AlarmClockSrv;

  constexpr uint32_t NEXT_TIME_MDNS_SHORT = 4000UL;
  constexpr uint32_t NEXT_TIME_MDNS = 17000UL;
  constexpr uint32_t NEXT_TIME_DISCOVER_SHORT = 2500UL;
  constexpr uint32_t NEXT_TIME_DISCOVER = 350000UL;

  const char *DeviceDiscover::tag{ "DeviceDiscover" };
  bool DeviceDiscover::isInit{ false };
  bool DeviceDiscover::mdnsIsRunning{ false };
  TaskHandle_t DeviceDiscover::taskHandle{ nullptr };

  /**
   * init some things, start discover thread
   */
  void DeviceDiscover::init()
  {
    if ( !DeviceDiscover::isInit )
    {
      DeviceDiscover::isInit = true;
      // make some preparations
      StatusObject::init();
      DeviceDiscover::start();
      randomSeed( analogRead( 0 ) );
    }
  }

  /**
   * discover thread, sleeps the most time :-)
   */
  void DeviceDiscover::discoverTask( void * )
  {
    static unsigned long nextTimeDiscover{ ( millis() + NEXT_TIME_DISCOVER_SHORT ) };
    static unsigned long nextTimeMDNSCheck{ ( millis() + NEXT_TIME_MDNS_SHORT ) };
    static WlanState oldConnectionState{ WlanState::DISCONNECTED };
    //
    // forever
    //
    elog.log( INFO, "%s: discoverTask running...", DeviceDiscover::tag );
    while ( true )
    {
      //
      // mDNS state checking
      //
      if ( millis() > nextTimeMDNSCheck )
      {
        //
        // at first, has state changed?
        //
        if ( StatusObject::getWlanState() != oldConnectionState )
        {
          WlanState newConnectionState = StatusObject::getWlanState();
          //
          // state has changed
          //
          if ( newConnectionState == WlanState::TIMESYNCED || newConnectionState == WlanState::CONNECTED )
          {
            // if has timesynced => start mdns
            if ( !DeviceDiscover::mdnsIsRunning )
            {
              DeviceDiscover::startMDNS();
              nextTimeDiscover = millis() + NEXT_TIME_DISCOVER_SHORT;
            }
          }
          else if ( newConnectionState == WlanState::DISCONNECTED || newConnectionState == WlanState::FAILED )
          {
            // state was disconnected
            DeviceDiscover::stopMDNS();
          }
          oldConnectionState = newConnectionState;
        }
        nextTimeMDNSCheck = millis() + NEXT_TIME_MDNS;
      }

      //
      // checking if device discovering is current
      //
      if ( millis() < nextTimeDiscover )
      {
        // wait loop for next discovering or mDNS check
        delay( 103 );
        continue;
      }
      //
      // ok discover the devices
      // a little entrophy please
      //
      nextTimeDiscover = millis() + static_cast< unsigned long >( random( NEXT_TIME_DISCOVER, NEXT_TIME_DISCOVER + 1500UL ) );
      if ( DeviceDiscover::mdnsIsRunning )
      {
        elog.log( DEBUG, "%s: start devices search...", DeviceDiscover::tag );
        DevListPtr services = DeviceDiscover::discoverSoundTouchDevices();
        if ( services->size() == 0 )
        {
          elog.log( DEBUG, "%s: start devices search, nothing found.", DeviceDiscover::tag );
        }
        else
        {
          elog.log( DEBUG, "%s: start devices search, found <%d> services.", DeviceDiscover::tag, services->size() );
          // update current list
          DeviceDiscover::updateCurrentDeviceList( services );
        }
        //_soundtouch._tcp.local.
      }
    }  // end while forever
  }

  void DeviceDiscover::updateCurrentDeviceList( DevListPtr devList )
  {
    //
    // copy all devices, there are in
    //
    StatusObject::devList.clear();
    for ( const auto &entry : *( devList.get() ) )
    {
      StatusObject::devList.push_back( entry );
    }
  }

  /**
   * start mdns service with my own params
   */
  bool DeviceDiscover::startMDNS()
  {
    //
    // start the daemon with the own hostname
    //
    String myInstanceName( Prefs::LocalPrefs::getHostName() );
    myInstanceName.toLowerCase();

    if ( !DeviceDiscover::mdnsIsRunning )
    {
      if ( mdns_init() != ESP_OK )
      {
        elog.log( ERROR, "%s: mDNS failed to start", DeviceDiscover::tag );
        return false;
      }
      DeviceDiscover::mdnsIsRunning = true;
      if ( mdns_hostname_set( myInstanceName.c_str() ) )
      {
        elog.log( ERROR, "%s: mDNS failed to set own hostname", DeviceDiscover::tag );
        return false;
      }
      if ( mdns_instance_name_set( myInstanceName.c_str() ) )
      {
        elog.log( ERROR, "%s: mDNS failed to set own hostname", DeviceDiscover::tag );
      }
      if ( mdns_service_add( myInstanceName.c_str(), "_http", "_tcp", 80, nullptr, 0 ) )
      {
        elog.log( ERROR, "%s: mDNS failed to announce own service", DeviceDiscover::tag );
      }
      return true;
    }
    return false;
  }

  /**
   * stop the mdns service
   */
  bool DeviceDiscover::stopMDNS()
  {
    if ( DeviceDiscover::mdnsIsRunning )
    {
      mdns_free();
    }
    return true;
  }

  /**
   * find soundtouch devices and create a fresh config
   */
  DevListPtr DeviceDiscover::discoverSoundTouchDevices()
  {
    mdns_result_t *results;
    DevListPtr devListPtr = std::make_shared< DevList >();

    esp_err_t err = mdns_query_ptr( "_soundtouch", "_tcp", 3000, 20, &results );
    if ( err )
    {
      elog.log( ERROR, "%s: mDNS failed to discover soundtouch services", DeviceDiscover::tag );
      return 0;
    }
    if ( !results )
    {
      elog.log( WARNING, "%s: mDNS none soundtouch services discovered", DeviceDiscover::tag );
      return 0;
    }
    mdns_result_t *r = results;
    int resultIdx = 0;
    while ( r )
    {
      resultIdx++;
      r = r->next;
    }
    //
    // create an device list
    //
    for ( int idx = 0; idx < resultIdx; ++idx )
    {
      DeviceEntry devEntry;
      mdns_result_t *currResult = DeviceDiscover::getResult( results, idx );
      if ( !currResult )
      {
        elog.log( ERROR, "%s: mDNS not found a result on index <%d>!", DeviceDiscover::tag, idx );
        continue;
      }
      devEntry.addr = DeviceDiscover::getIP( currResult );
      devEntry.webPort = currResult->port;
      devEntry.wsPort = 8080;
      devEntry.name = String( currResult->instance_name );
      elog.log( DEBUG, "%s: device #%d has IPv4 <%s:%d> (instance: %s)", DeviceDiscover::tag, idx, devEntry.addr.toString(),
                devEntry.webPort, devEntry.name.c_str() );
      // how many text entrys are there
      int textNum = static_cast< int >( currResult->txt_count );
      for ( int txIdx = 0; txIdx < textNum; ++txIdx )
      {
        //
        // find and filter text entrys
        //
        mdns_txt_item_t textPair = currResult->txt[ txIdx ];
        String key( textPair.key );
        String val( textPair.value );
        if ( key.equals( "DESCRIPTION" ) )
          devEntry.type = val;
        else if ( key.equals( "MAC" ) )
          devEntry.id = val;
        delay( 1 );
      }
      devListPtr->push_back( devEntry );
      delay( 5 );
    }
    //
    // free the results
    //
    mdns_query_results_free( results );
    return devListPtr;
  }

  /**
   * get ipv4 adress for an entry as IPAddress
   */
  IPAddress DeviceDiscover::getIP( mdns_result_t *result )
  {
    mdns_ip_addr_t *addr = result->addr;
    while ( addr )
    {
      if ( addr->addr.type == MDNS_IP_PROTOCOL_V4 )
      {
        return IPAddress( addr->addr.u_addr.ip4.addr );
      }
      addr = addr->next;
    }
    return IPAddress();
  }

  /**
   * get an result for an device via index
   */
  mdns_result_t *DeviceDiscover::getResult( mdns_result_t *results, int idx )
  {
    mdns_result_t *result = results;
    int i = 0;
    while ( result )
    {
      if ( i == idx )
      {
        break;
      }
      i++;
      result = result->next;
    }
    return result;
  }

  /**
   * start the device discovering esp32/RTOS task
   */
  void DeviceDiscover::start()
  {
    elog.log( logger::INFO, "%s: discoverTask start...", DeviceDiscover::tag );

    if ( DeviceDiscover::taskHandle )
    {
      vTaskDelete( DeviceDiscover::taskHandle );
      DeviceDiscover::taskHandle = nullptr;
    }
    else
    {
      xTaskCreate( DeviceDiscover::discoverTask, "led-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY,
                   &DeviceDiscover::taskHandle );
    }
  }

}  // namespace soundtouch