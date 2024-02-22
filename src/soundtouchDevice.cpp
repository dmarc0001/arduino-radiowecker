#include <utility>
#include "soundtouchDevice.hpp"
#include "statics.hpp"

namespace soundtouch
{

  using namespace websockets;
  using namespace alarmclock;
  using namespace logger;

  const char *SoundTouchDevice::tag{ "soundtouch" };
  TaskHandle_t SoundTouchDevice::wsTaskHandle{ nullptr };
  TaskHandle_t SoundTouchDevice::decTaskHandle{ nullptr };
  uint32_t SoundTouchDevice::instances{ 0 };
  InstancesList SoundTouchDevice::instList;

  SoundTouchDevice::SoundTouchDevice( alarmclock::DeviceEntry &_device ) : device( _device )
  {
    //
    // create an instance and  a instance id
    //
    ++SoundTouchDevice::instances;
    this->instance = SoundTouchDevice::instances;
    InstancePtr myInstance = std::make_pair( this->instance, this );
    SoundTouchDevice::instList.push_back( myInstance );
    elog.log( INFO, "%s: create Instance: %d", SoundTouchDevice::tag, this->instance );
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
    snprintf( buffer, 48, "ws://%s:%d\0", device.addr.toString().c_str(), device.wsPort );
    String interfaceString( buffer );
    elog.log( INFO, "%s: connect websocket to <%s>", SoundTouchDevice::tag, interfaceString.c_str() );
    //
    // create subprotocol "gabbo"
    // BOSE API description
    //
    String key1( "Sec-WebSocket-Protocol" );
    String value( "gabbo" );
    wsClient.WebsocketsClient::addHeader( key1, value );
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
    for ( auto it = SoundTouchDevice::instList.begin(); it < SoundTouchDevice::instList.end(); it++ )
    {
      if ( it->first == this->instance )
      {
        SoundTouchDevice::instList.erase( it );
        break;
      }
    }
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
    for ( auto elem : SoundTouchDevice::instList )
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
   * task for processing websocket connections in SoundtouchDevice
   */
  void SoundTouchDevice::wsTask( void * )
  {
    static unsigned long nextPing{ millis() + 8000UL };
    bool timeToPing{ false };

    nextPing = millis() + 8000UL;
    while ( true )
    {
      timeToPing = ( millis() > nextPing );
      for ( auto elem : SoundTouchDevice::instList )
      {
        if ( timeToPing )
        {
          // Send a ping
          elem.second->wsClient.ping();
        }
        elem.second->wsClient.poll();
      }
      if ( timeToPing )
        nextPing = millis() + 17000UL + static_cast< unsigned long >( random( 1000 ) );
      delay( 10 );
    }
  }

  /**
   * task for decoding incomming xml messages
   */
  void SoundTouchDevice::decTask( void * )
  {
    while ( true )
    {
      bool wasEntry{ false };
      for ( auto elem : SoundTouchDevice::instList )
      {
        if ( !elem.second->xmlList.empty() )
        {
          // message in the list => decode
          elem.second->parser->decodeMessage();
          wasEntry = true;
        }
      }
      if ( !wasEntry )
        delay( 150 );
    }
  }

  /**
   * start or restart the SoundtouchDevice's task
   */
  void SoundTouchDevice::start()
  {
    elog.log( logger::INFO, "%s: soundtouch websocket task start...", SoundTouchDevice::tag );

    if ( SoundTouchDevice::wsTaskHandle )
    {
      vTaskDelete( SoundTouchDevice::wsTaskHandle );
      SoundTouchDevice::wsTaskHandle = nullptr;
    }
    else
    {
      xTaskCreate( SoundTouchDevice::wsTask, "ws-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY,
                   &SoundTouchDevice::wsTaskHandle );
    }

    elog.log( logger::INFO, "%s: soundtouch xml decoder task start...", SoundTouchDevice::tag );

    if ( SoundTouchDevice::decTaskHandle )
    {
      vTaskDelete( SoundTouchDevice::decTaskHandle );
      SoundTouchDevice::decTaskHandle = nullptr;
    }
    else
    {
      xTaskCreate( SoundTouchDevice::decTask, "xml-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY,
                   &SoundTouchDevice::decTaskHandle );
    }
  }

}  // namespace soundtouch
