#include <utility>
#include "appPreferences.hpp"
#include "soundtouchDevice.hpp"
#include "statics.hpp"

namespace soundtouch
{

  using namespace websockets;
  using namespace alertclock;
  using namespace logger;

  const char *SoundTouchDevice::tag{ "soundtouchdev" };
  TaskHandle_t SoundTouchDevice::wsTaskHandle{ nullptr };
  TaskHandle_t SoundTouchDevice::decTaskHandle{ nullptr };
  uint32_t SoundTouchDevice::instances{ 0 };
  InstancesList SoundTouchDevice::instancesList;

  /**
   * constructor, must habe an reference to an soundtouch device
   */
  SoundTouchDevice::SoundTouchDevice( alertclock::DeviceEntryPtr _device ) : device( _device )
  {
    //
    // create an instance and  a instance id
    //
    ++SoundTouchDevice::instances;
    this->instance = SoundTouchDevice::instances;
    InstancePtr myInstance = std::make_pair( this->instance, this );
    SoundTouchDevice::instancesList.push_back( myInstance );
    elog.log( INFO, "%s: create Instance: %d", SoundTouchDevice::tag, this->instance );
    http.setReuse( true );  /// keep-alive
    runMode = ST_STATE_UNINITIALIZED;
    //
    // make callbacks
    // run callback when messages are received
    //
    wsClient.onMessage( SoundTouchDevice::onMessageCallbackWrapper, this->instance );
    //
    // run callback when events are occuring
    //
    wsClient.onEvent( SoundTouchDevice::onEventsCallbackWrapper, this->instance );
    //
    // make a decoder object (unique deletes if object will delete)
    //
    parser = SoundTouchXMLParserPtr( new SoundTouchXMLParser( xmlList, msgList ) );
    //
    // Connect to server
    //
    char buffer[ 49 ];
    snprintf( buffer, 48, "ws://%s:%d\0", device->addr.toString().c_str(), device->wsPort );
    String interfaceString( buffer );
    elog.log( INFO, "%s: connect websocket to <%s>", SoundTouchDevice::tag, interfaceString.c_str() );
    //
    // create subprotocol "gabbo"
    // BOSE API description
    //
    String key1( "Sec-WebSocket-Protocol" );
    String value( "gabbo" );
    wsClient.WebsocketsClient::addHeader( key1, value );
    // make a connection to bose soundtouch
    wsClient.connect( interfaceString );
    //
    // start ws socket task
    //
    start();
  }

  /**
   * destructor, should erase instance from global list
   */
  SoundTouchDevice::~SoundTouchDevice()
  {
    wsClient.close();
    if ( SoundTouchDevice::wsTaskHandle )
    {
      elog.log( INFO, "%s: kill ws-tread instance: %d", SoundTouchDevice::tag, this->instance );
      vTaskDelete( SoundTouchDevice::wsTaskHandle );
      SoundTouchDevice::wsTaskHandle = nullptr;
    }
    if ( SoundTouchDevice::decTaskHandle )
    {
      elog.log( INFO, "%s: kill dec-tread instance: %d", SoundTouchDevice::tag, this->instance );
      vTaskDelete( SoundTouchDevice::decTaskHandle );
      SoundTouchDevice::decTaskHandle = nullptr;
    }
    for ( auto it = SoundTouchDevice::instancesList.begin(); it < SoundTouchDevice::instancesList.end(); it++ )
    {
      if ( it->first == this->instance )
      {
        SoundTouchDevice::instancesList.erase( it );
        break;
      }
    }
  }

  /**
   * send questions to devive for current status
   * (power state, playstate, volume)
   */
  bool SoundTouchDevice::getDeviceInfos()
  {
    runMode = ST_STATE_GET_INFOS;
    if ( askForNowPlaying() )
    {
      delay( 25 );
      if ( askForZones() )
      {
        delay( 25 );
        if ( askForVolume() )
        {
          //
          // done all questions!
          //
          return true;
        }
      }
    }
    return false;
  }

  /**
   * ask the device for actual playing
   */
  bool SoundTouchDevice::askForNowPlaying()
  {
    bool wasOk{ false };
    String questionString( std::move( getUrlString( WEB_GET_NOW_PLAYINGZONE ) ) );

    elog.log( DEBUG, "%s: ask for nowPlaying...(%s)", SoundTouchDevice::tag, questionString.c_str() );
    // prepare http
    // http.begin( tcpClient, questionString );
    http.begin( questionString );
    // make question
    int httpResponseCode = http.GET();
    delay( 50 );
    if ( httpResponseCode > 0 )
    {
      wasOk = true;
      // elog.log( DEBUG, "%s: HTTP Response code: %d", SoundTouchDevice::tag, httpResponseCode );
      String payload = http.getString();
      // elog.log( DEBUG, "%s: payload <%s>", SoundTouchDevice::tag, payload.c_str() );
      int idx = payload.indexOf( "?>" );
      if ( idx > 0 )
      {
        //
        // add header for zone update, so i can using the present code
        // and than put it in the decoder queue
        //
        String msg = "<updates deviceID=\"";
        msg += device->id;
        msg += "\">";
        msg += "<nowPlayingUpdated>";
        msg += payload.substring( idx + 2 );
        msg += "</nowPlayingUpdated>";
        msg += "</updates>";
        // elog.log( DEBUG, "%s: payload <%s>", SoundTouchDevice::tag, msg.c_str() );
        xmlList.push_back( msg );
      }
      // <?xml version="1.0" encoding="UTF-8" ?>
      //   <nowPlaying deviceID="689E19653E96" source="AUX">
      //     <ContentItem source="AUX" sourceAccount="AUX" isPresetable="true">
      //       <itemName>AUX IN</itemName>
      //     </ContentItem>
      //     <playStatus>PLAY_STATE</playStatus>
      //   </nowPlaying>
    }
    else
    {
      elog.log( ERROR, "%s: HTTP Response code: %d", SoundTouchDevice::tag, httpResponseCode );
    }
    // Free resources
    http.end();
    return ( wasOk );
  }

  /**
   * ask device for zones
   */
  bool SoundTouchDevice::askForZones()
  {
    bool wasOk{ false };
    String questionString( std::move( getUrlString( WEB_GET_ZONE ) ) );

    elog.log( DEBUG, "%s: ask for zones...(%s)", SoundTouchDevice::tag, questionString.c_str() );
    // prepare http
    // http.begin( tcpClient, questionString );
    http.begin( questionString );
    //  make question
    int httpResponseCode = http.GET();
    delay( 50 );
    if ( httpResponseCode > 0 )
    {
      wasOk = true;
      // elog.log( DEBUG, "%s: HTTP Response code: %d", SoundTouchDevice::tag, httpResponseCode );
      String payload = http.getString();
      // elog.log( DEBUG, "%s: payload <%s>", SoundTouchDevice::tag, payload.c_str() );
      int idx = payload.indexOf( "?>" );
      if ( idx > 0 )
      {
        //
        // add header for zone update, so i can using the present code
        // and than put it in the decoder queue
        //
        String msg = "<updates deviceID=\"";
        msg += device->id;
        msg += "\"><zoneUpdated>";
        msg += payload.substring( idx + 2 );
        msg += "</zoneUpdated></updates>";
        // elog.log( DEBUG, "%s: payload <%s>", SoundTouchDevice::tag, msg.c_str() );
        xmlList.push_back( msg );
      }
      // <?xml version="1.0" encoding="UTF-8" ?>
      // <zone master="689E19653E96">
      //   <member ipaddress="192.168.1.68">
      //     689E19653E96
      //   </member>
      //   <member ipaddress="192.168.1.39">
      //     38D2697C128E
      //   </member>
      // </zone>
    }
    else
    {
      elog.log( ERROR, "%s: HTTP Response code: %d", SoundTouchDevice::tag, httpResponseCode );
    }
    // Free resources
    http.end();
    return ( wasOk );
  }

  /**
   * ask the device for volume
   */
  bool SoundTouchDevice::askForVolume()
  {
    bool wasOk{ false };
    String questionString( std::move( getUrlString( WEB_CMD_VOLUME ) ) );

    elog.log( DEBUG, "%s: ask for volume...(%s)", SoundTouchDevice::tag, questionString.c_str() );
    // prepare http
    // http.begin( tcpClient, questionString );
    http.begin( questionString );
    //  make question
    int httpResponseCode = http.GET();
    delay( 50 );
    if ( httpResponseCode > 0 )
    {
      wasOk = true;
      // elog.log( DEBUG, "%s: HTTP Response code: %d", SoundTouchDevice::tag, httpResponseCode );
      String payload = http.getString();
      // elog.log( DEBUG, "%s: payload <%s>", SoundTouchDevice::tag, payload.c_str() );
      int idx = payload.indexOf( "?>" );
      if ( idx > 0 )
      {
        //
        // add header for zone update, so i can using the present code
        // and than put it in the decoder queue
        //
        String msg = "<updates deviceID=\"";
        msg += device->id;
        msg += "\"><volumeUpdated>";
        msg += payload.substring( idx + 2 );
        msg += "</volumeUpdated></updates>";
        xmlList.push_back( msg );
      }
    }
    // <?xml version="1.0" encoding="UTF-8" ?>
    // <volume deviceID="689E19653E96">
    //   <targetvolume>44</targetvolume>
    //   <actualvolume>44</actualvolume>
    //   <muteenabled>false</muteenabled>
    // </volume>
    http.end();
    return ( wasOk );
  }

  /**
   * simulate press power button if needet
   */
  bool SoundTouchDevice::setPower( bool _pwstate )
  {
    if ( ( currentState.playStatus == STANDBY_STATE ) != _pwstate )
      return true;
    //
    return clickSoundTouchVirtualButton( KEY_POWER );
  }

  /**
   * simulate press mute key if needet
   */
  bool SoundTouchDevice::setMute( bool _mute )
  {
    if ( currentState.currVolume.mute == _mute )
      return true;
    //
    return clickSoundTouchVirtualButton( KEY_MUTE );
  }

  /**
   * touch an virtual soundtouch button
   */
  bool SoundTouchDevice::touchBtn( String &_btn )
  {
    return clickSoundTouchVirtualButton( _btn );
  }

  /**
   * touch an virtual soundtouch button
   */
  bool SoundTouchDevice::touchBtn( const char *_btn )
  {
    return clickSoundTouchVirtualButton( _btn );
  }

  /**
   * set current volume on device
   */
  bool SoundTouchDevice::setCurrentVolume( uint8_t _vol, bool _mute )
  {
    bool wasOk{ false };
    String questionString( std::move( getUrlString( WEB_CMD_VOLUME ) ) );
    char buffer[ 32 ];
    char *bufferPtr = &buffer[ 0 ];
    // faster, less memory as String
    snprintf( bufferPtr, 32, "<volume>%d</volume>", static_cast< int >( _vol ) );
    String payload( bufferPtr );
    elog.log( DEBUG, "%s: set volume to %s", SoundTouchDevice::tag, payload.c_str() );
    http.begin( questionString );
    //  make question
    int httpResponseCode = http.POST( payload );
    delay( 50 );
    if ( httpResponseCode > 0 )
    {
      wasOk = true;
    }
    http.end();
    if ( currentState.currVolume.mute != _mute )
      return setMute( _mute );
    return ( wasOk );
  }

  /**
   * click virtual button on soundtouch device
   */
  bool SoundTouchDevice::clickSoundTouchVirtualButton( const String &_btn )
  {
    return clickSoundTouchVirtualButton( _btn.c_str() );
  }

  /**
   * click virtual button on soundtouch device
   */
  bool SoundTouchDevice::clickSoundTouchVirtualButton( const char *_btn )
  {
    String questionString( std::move( getUrlString( WEB_CMD_KEY ) ) );
    int httpResponseCode;
    char buffer[ 64 ];
    char *bufferPtr = &buffer[ 0 ];
    elog.log( DEBUG, "%s: toggle button %s", SoundTouchDevice::tag, _btn );
    //
    // press and release key
    //
    elog.log( DEBUG, "%s: press virtual key (%s)...", SoundTouchDevice::tag, questionString.c_str() );
    for ( int i = 0; i < 2; ++i )
    {
      const char *keyState = ( i == 0 ) ? KEY_PRESS : KEY_RELEASE;
      snprintf( bufferPtr, 64, "<key state=\"%s\" sender=\"Gabbo\">%s</key>", keyState, _btn );
      String payload( bufferPtr );
      http.begin( questionString );
      //  make question
      httpResponseCode = http.POST( payload );
      if ( httpResponseCode < 0 )
      {
        elog.log( ERROR, "%s: error while send keystroke %d, errcode -%03d, ABORT", SoundTouchDevice::tag, _btn,
                  0 - httpResponseCode );
        http.end();
        return false;
      }
      else
      {
        http.end();
        delay( ( 2 - i ) * 10 );
      }
    }
    return ( true );
  }

  /**
   * generate an url vor GET operation and command name
   */
  String SoundTouchDevice::getUrlString( const char *_command )
  {
    String questionString;

    questionString = "http://";
    questionString += device->addr.toString();
    questionString += ":";
    questionString += device->webPort;
    questionString += _command;
    return questionString;
  }

  /**
   * callback function for websocket receiver
   */
  void SoundTouchDevice::onMessageCallback( WebsocketsMessage _message )
  {
    if ( _message.isEmpty() )
    {
      elog.log( DEBUG, "%s: message is empty", SoundTouchDevice::tag );
      return;
    }
    else if ( _message.isText() )
    {
      if ( _message.isComplete() )
      {
        elog.log( DEBUG, "%s: msg (complete) received.", SoundTouchDevice::tag );
        // move the message to queue for decoding
        xmlList.push_back( std::move( _message.data() ) );
        return;
      }
      else
      {
        elog.log( DEBUG, "%s: msg is NOT complete (not implemented yet)", SoundTouchDevice::tag );
      }
    }
    else
    {
      elog.log( ERROR, "%s: msg is NOT text, so it's not implemented yet", SoundTouchDevice::tag );
    }
  }

  /**
   * callback function for websocket receiver
   */
  void SoundTouchDevice::onEventCallback( WebsocketsEvent _event, String _data )
  {
    if ( _event == WebsocketsEvent::ConnectionOpened )
    {
      elog.log( DEBUG, "%s: websocket instance %d connection opened!", SoundTouchDevice::tag, this->instance );
    }
    else if ( _event == WebsocketsEvent::ConnectionClosed )
    {
      elog.log( DEBUG, "%s: websocket instance %d connection closed!", SoundTouchDevice::tag, this->instance );
    }
    else if ( _event == WebsocketsEvent::GotPing )
    {
      elog.log( DEBUG, "%s: websocket instance %d received ping!", SoundTouchDevice::tag, this->instance );
      wsClient.pong();
    }
    else if ( _event == WebsocketsEvent::GotPong )
    {
      elog.log( DEBUG, "%s: websocket instance %d received pong!", SoundTouchDevice::tag, this->instance );
    }
  }

  /**
   * on received a message (static function) then send message to the right instance
   */
  void SoundTouchDevice::onMessageCallbackWrapper( WebsocketsMessage _message, uint32_t _instance )
  {
    SoundTouchDevice *stInstance{ nullptr };

    //
    // find instance addr
    //
    stInstance = SoundTouchDevice::getInstancePtr( _instance );
    if ( !stInstance )
    {
      elog.log( ERROR, "%s: can't find active websocket instance (id: %d) for processing! ABORT", SoundTouchDevice::tag, _instance );
      return;
    }
    stInstance->onMessageCallback( _message );
  }

  /**
   * in received a event, (static function) route the event to the right instance
   */
  void SoundTouchDevice::onEventsCallbackWrapper( WebsocketsEvent _event, String _data, uint32_t _instance )
  {
    SoundTouchDevice *stInstance = SoundTouchDevice::getInstancePtr( _instance );
    if ( !stInstance )
    {
      elog.log( ERROR, "%s: can't find active websocket instance (id: %d) for processing! ABORT", SoundTouchDevice::tag, _instance );
      return;
    }
    stInstance->onEventCallback( _event, _data );
  }

  /**
   * find a pointer to the instfance of SoundtouchDevice with an insctance number
   */
  SoundTouchDevice *SoundTouchDevice::getInstancePtr( uint32_t _instance )
  {
    for ( auto elem : SoundTouchDevice::instancesList )
    {
      if ( elem.first == _instance )
      {
        // instance found!
        SoundTouchDevice *stInstance = elem.second;
        return stInstance;
      }
    }
    return nullptr;
  }

  /**
   * callback (member of object) which compute the messages in order to runMode
   */
  void SoundTouchDevice::onDecodetMessage( SoundTouchUpdateTmplPtr ptr )
  {
    elog.log( INFO, "%s: rec decodet msg, type %d", SoundTouchDevice::tag, ptr->getUpdateType() );
    //
    // decode enytime
    //
    switch ( ptr->getUpdateType() )
    {
      case MSG_UPDATE_VOLUME:
        //
        // save the current state
        //
        currentState.currVolume.currVol = static_cast< SoundTouchVolumeUpdate * >( ptr.get() )->currVol;
        currentState.currVolume.targetVol = static_cast< SoundTouchVolumeUpdate * >( ptr.get() )->targetVol;
        currentState.currVolume.mute = static_cast< SoundTouchVolumeUpdate * >( ptr.get() )->mute;
        currentState.stateChecked.isVolume = true;
        // elog.log( INFO, "%s: rec decodet msg, state: get infos, type: volume (%d/%d)", SoundTouchDevice::tag,
        //           currentState.currVolume.currVol, currentState.currVolume.targetVol );
        break;
      case MSG_UPDATE_NOW_PLAYING_CHANGED:
        currentState.playItem = static_cast< SoundTouchNowPlayingUpdate * >( ptr.get() )->contenItem;
        currentState.playStatus = static_cast< SoundTouchNowPlayingUpdate * >( ptr.get() )->playStatus;
        // elog.log( INFO, "%s: rec decodet msg, state: get infos, type: playstatus (%s)", SoundTouchDevice::tag,
        //           currentState.playStatus == PLAY_STATE ? "playing" : "not playing" );
        currentState.stateChecked.isPlaying = true;
        break;
      case MSG_UPDATE_ZONE:
        currentState.masterID = static_cast< SoundTouchZoneUpdate * >( ptr.get() )->masterID;
        currentState.members = static_cast< SoundTouchZoneUpdate * >( ptr.get() )->members;
        // elog.log(
        //     INFO, "%s: rec decodet msg, state: get infos, type: zones, members: %d (%s)", SoundTouchDevice::tag,
        //     currentState.members.size(),
        //     ( ( currentState.masterID == currentState.deviceID ) && ( currentState.members.size() > 0 ) ) ? "master" : "not master"
        //     );
        currentState.stateChecked.isZone = true;
        break;
      default:
        elog.log( WARNING, "%s: rec decodet msg, state: get infos, type: unkown (%d)", SoundTouchDevice::tag,
                  static_cast< int >( ptr->getUpdateType() ) );
    }
    //
    // special on decode time
    //
    switch ( runMode )
    {
      case ST_STATE_UNINITIALIZED:
      case ST_STATE_GET_INFOS:
        //
        // ckeck if state change is needet
        //
        if ( currentState.stateChecked.isPlaying && currentState.stateChecked.isVolume && currentState.stateChecked.isZone )
        {
          //
          // set mode to alert initiate
          //
          elog.log( INFO, "%s: now in state: alert init...", SoundTouchDevice::tag );
          runMode = ST_STATE_INIT_ALERT;
          //
          // next config Device for alert
          // if no zone and no playing, config device(s)
          //
        }
        break;

      case ST_STATE_INIT_ALERT:
        //
        // check, if the device standby?
        //
        break;

      case ST_STATE_WAIT_FOR_INIT_COMLETE:
        break;
        
      case ST_STATE_ERROR:
      default:
        break;
    }  // end switch runMode
  }

  /**
   * task for processing websocket connections in SoundtouchDevice
   */
  void SoundTouchDevice::wsTask( void * )
  {
    static const char *tag{ "wstask" };

    int64_t nextPing{ esp_timer_get_time() + getMicrosForMiliSec( 8000L ) };
    int64_t nextMark =
        esp_timer_get_time() + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
    bool timeToPing{ false };

    elog.log( logger::INFO, "%s: soundtouch websocket task start...", tag );

    while ( true )
    {
      timeToPing = ( esp_timer_get_time() > nextPing );
      for ( auto elem : SoundTouchDevice::instancesList )
      {
        if ( timeToPing )
        {
          // Send a ping
          elem.second->wsClient.ping();
        }
        elem.second->wsClient.poll();
      }
      if ( timeToPing )
      {
        nextPing = esp_timer_get_time() + getMicrosForMiliSec( 8000 + static_cast< int32_t >( random( 1000 ) ) );
      }
      if ( nextMark < esp_timer_get_time() )
      {
        elog.log( DEBUG, "%s: ==== MARK ====", tag );
        nextMark =
            esp_timer_get_time() + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
      }
      yield();
    }
  }

  /**
   * task for decoding incomming xml messages
   */
  void SoundTouchDevice::decTask( void * )
  {
    static const char *tag{ "decodertask" };
    bool wasEntry{ false };
    int64_t nextMark = esp_timer_get_time() + getMicrosForMiliSec( 20000L );
    elog.log( logger::INFO, "%s: soundtouch xml decoder task start...", tag );
    while ( true )
    {
      wasEntry = false;
      for ( auto elem : SoundTouchDevice::instancesList )
      {
        if ( !elem.second->xmlList.empty() )
        {
          // message in the list => decode
          elem.second->parser->decodeMessage();
          wasEntry = true;
          break;
        }
      }
      for ( auto elem : SoundTouchDevice::instancesList )
      {
        if ( !elem.second->msgList.empty() )
        {
          // decodet message in the list => make something
          SoundTouchUpdateTmplPtr ptr = elem.second->msgList.front();
          elem.second->msgList.erase( elem.second->msgList.begin() );
          elem.second->onDecodetMessage( ptr );
          wasEntry = true;
          break;
        }
      }
      //
      // if there was no entry in a list here
      //
      if ( !wasEntry )
      {
        if ( nextMark < esp_timer_get_time() )
        {
          elog.log( DEBUG, "%s: ==== MARK ====", tag );
          nextMark = esp_timer_get_time() + getMicrosForMiliSec( 21003L );
        }
      }
      yield();
    }
  }

  /**
   * start or restart the SoundtouchDevice's task
   */
  void SoundTouchDevice::start()
  {
    // elog.log( logger::INFO, "%s: soundtouch websocket task start...", SoundTouchDevice::tag );

    if ( SoundTouchDevice::wsTaskHandle )
    {
      vTaskDelete( SoundTouchDevice::wsTaskHandle );
      SoundTouchDevice::wsTaskHandle = nullptr;
    }
    else
    {
      xTaskCreate( SoundTouchDevice::wsTask, "ws-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY + 1,
                   &SoundTouchDevice::wsTaskHandle );
    }

    // elog.log( logger::INFO, "%s: soundtouch xml decoder task start...", SoundTouchDevice::tag );

    if ( SoundTouchDevice::decTaskHandle )
    {
      vTaskDelete( SoundTouchDevice::decTaskHandle );
      SoundTouchDevice::decTaskHandle = nullptr;
    }
    else
    {
      xTaskCreate( SoundTouchDevice::decTask, "xml-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY + 1,
                   &SoundTouchDevice::decTaskHandle );
    }
  }

}  // namespace soundtouch
