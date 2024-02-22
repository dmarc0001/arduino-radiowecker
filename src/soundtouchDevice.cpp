#include "soundtouchDevice.hpp"
#include "statics.hpp"

namespace soundtouch
{

  using namespace websockets;
  using namespace AlarmClockSrv;
  using namespace logger;

  const char *SoundtouchDevice::tag{ "soundtouch" };
  TaskHandle_t SoundtouchDevice::taskHandle{ nullptr };
  uint32_t SoundtouchDevice::instances{ 0 };
  InstancesList SoundtouchDevice::instList;

  SoundtouchDevice::SoundtouchDevice( AlarmClockSrv::DeviceEntry &_device ) : device( _device )
  {
    //
    // create an instance id
    //
    ++SoundtouchDevice::instances;
    this->instance = SoundtouchDevice::instances;
    InstancePtr myInstance = std::make_pair( this->instance, this );
    SoundtouchDevice::instList.push_back( myInstance );
    elog.log( INFO, "%s: create Instance: %d", SoundtouchDevice::tag, this->instance );

    // run callback when messages are received
    wsClient.onMessage( SoundtouchDevice::onMessageCallbackWrapper, this->instance );

    // // run callback when events are occuring
    wsClient.onEvent( SoundtouchDevice::onEventsCallbackWrapper, this->instance );

    // // Connect to server
    char buffer[ 49 ];
    snprintf( buffer, 48, "ws://%s:%d\0", device.addr.toString().c_str(), device.wsPort );
    String interfaceString( buffer );
    elog.log( INFO, "%s: connect websocket to <%s>", SoundtouchDevice::tag, interfaceString.c_str() );
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
  SoundtouchDevice::~SoundtouchDevice()
  {
    if ( SoundtouchDevice::taskHandle )
    {
      elog.log( INFO, "%s: kill ws-tread instance: %d", SoundtouchDevice::tag, this->instance );
      vTaskDelete( SoundtouchDevice::taskHandle );
      SoundtouchDevice::taskHandle = nullptr;
    }
    for ( auto it = SoundtouchDevice::instList.begin(); it < SoundtouchDevice::instList.end(); it++ )
    {
      if ( it->first == this->instance )
      {
        SoundtouchDevice::instList.erase( it );
        break;
      }
    }
  }

  /**
   * callback function for websocket receiver
   */
  void SoundtouchDevice::onMessageCallback( WebsocketsMessage _message )
  {
    if ( _message.isEmpty() )
    {
      elog.log( DEBUG, "%s: message is empty", SoundtouchDevice::tag );
      return;
    }
    else if ( _message.isText() )
    {
      elog.log( DEBUG, "%s: msg <%s>", SoundtouchDevice::tag, _message.data().c_str() );
      if ( _message.isComplete() )
      {
        elog.log( DEBUG, "%s: msg is complete", SoundtouchDevice::tag );
        // TODO: XML Parsing
      }
      else
      {
        elog.log( DEBUG, "%s: msg is NOT complete (not implemented yet)", SoundtouchDevice::tag );
      }
    }
    else
    {
      elog.log( ERROR, "%s: msg is NOT text, so it's not implemented yet", SoundtouchDevice::tag );
    }
  }

  /**
   * callback function for websocket receiver
   */
  void SoundtouchDevice::onEventCallback( WebsocketsEvent _event, String _data )
  {
    if ( _event == WebsocketsEvent::ConnectionOpened )
    {
      elog.log( DEBUG, "%s: websocket instance %d connection opened!", SoundtouchDevice::tag, this->instance );
    }
    else if ( _event == WebsocketsEvent::ConnectionClosed )
    {
      elog.log( DEBUG, "%s: websocket instance %d connection closed!", SoundtouchDevice::tag, this->instance );
    }
    else if ( _event == WebsocketsEvent::GotPing )
    {
      elog.log( DEBUG, "%s: websocket instance %d received ping!", SoundtouchDevice::tag, this->instance );
      wsClient.pong();
    }
    else if ( _event == WebsocketsEvent::GotPong )
    {
      elog.log( DEBUG, "%s: websocket instance %d received pong!", SoundtouchDevice::tag, this->instance );
    }
  }

  /**
   * on received a message (static function) then send message to the right instance
   */
  void SoundtouchDevice::onMessageCallbackWrapper( WebsocketsMessage _message, uint32_t _instance )
  {
    SoundtouchDevice *stInstance{ nullptr };

    //
    // find instance addr
    //
    stInstance = SoundtouchDevice::getInstancePtr( _instance );
    if ( !stInstance )
    {
      elog.log( ERROR, "%s: can't find active websocket instance (id: %d) for processing! ABORT", SoundtouchDevice::tag, _instance );
      return;
    }
    stInstance->onMessageCallback( _message );
  }

  /**
   * in received a event, (static function) route the event to the right instance
   */
  void SoundtouchDevice::onEventsCallbackWrapper( WebsocketsEvent _event, String _data, uint32_t _instance )
  {
    SoundtouchDevice *stInstance = SoundtouchDevice::getInstancePtr( _instance );
    if ( !stInstance )
    {
      elog.log( ERROR, "%s: can't find active websocket instance (id: %d) for processing! ABORT", SoundtouchDevice::tag, _instance );
      return;
    }
    stInstance->onEventCallback( _event, _data );
  }

  SoundtouchDevice *SoundtouchDevice::getInstancePtr( uint32_t _instance )
  {
    for ( auto elem : SoundtouchDevice::instList )
    {
      if ( elem.first == _instance )
      {
        // instance found!
        SoundtouchDevice *stInstance = elem.second;
        return stInstance;
      }
    }
    return nullptr;
  }

  void SoundtouchDevice::wsTask( void * )
  {
    static unsigned long nextPing{ millis() + 8000UL };
    bool timeToPing{ false };

    nextPing = millis() + 8000UL;
    while ( true )
    {
      timeToPing = ( millis() > nextPing );
      for ( auto elem : SoundtouchDevice::instList )
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

  void SoundtouchDevice::start()
  {
    elog.log( logger::INFO, "%s: soundtouch websocket Task start...", SoundtouchDevice::tag );

    if ( SoundtouchDevice::taskHandle )
    {
      vTaskDelete( SoundtouchDevice::taskHandle );
      SoundtouchDevice::taskHandle = nullptr;
    }
    else
    {
      xTaskCreate( SoundtouchDevice::wsTask, "ws-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY,
                   &SoundtouchDevice::taskHandle );
    }
  }

}  // namespace soundtouch
