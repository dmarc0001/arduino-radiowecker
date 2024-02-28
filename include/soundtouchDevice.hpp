#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <vector>
#include <memory>
#include <HTTPClient.h>
#include <ArduinoWebsockets.h>
#include "common.hpp"
#include "appStructs.hpp"
#include "soundTouchXMLParser.hpp"

namespace soundtouch
{
  using namespace websockets;

  enum SoundTouchDeviceRunningMode : uint8_t
  {
    ST_STATE_GET_INFOS,
    ST_STATE_INIT_ALERT,
    ST_STATE_WAIT_FOR_INIT_COMLETE,
    ST_STATE_RUNNING_ALERT,
    ST_STATE_UNKNOWN
  };

  class SoundTouchDevice;

  using InstancePtr = std::pair< uint32_t, SoundTouchDevice * >;
  using InstancesList = std::vector< InstancePtr >;
  using SoundTouchXMLParserPtr = std::unique_ptr< SoundTouchXMLParser >;

  class SoundTouchDevice
  {
    private:
    static const char *tag;
    static TaskHandle_t wsTaskHandle;
    static TaskHandle_t decTaskHandle;
    static uint32_t instances;
    uint32_t instance;
    static InstancesList instList;
    SoundTouchDeviceRunningMode runMode;
    WebsocketsClient wsClient;
    alarmclock::DeviceEntry device;
    SoundTouchDeviceState currentState;
    XmlMessageList xmlList;
    DecodetMessageList msgList;
    SoundTouchXMLParserPtr parser;
    HTTPClient http;
    SoundTouchDevice();  //! no default constructor!

    public:
    explicit SoundTouchDevice( alarmclock::DeviceEntry & );
    ~SoundTouchDevice();
    bool getDeviceInfos();  //! get device Infos for current State
    inline SoundTouchDeviceRunningMode getDeviceRunningState()
    {
      return runMode;
    }

    private:
    bool askForZones();                                                        //! ast via webinterface for zones on the device
    bool askForNowPlaying();                                                   //! ask for playing now
    bool askForVolume();                                                       //! ask via webinterface for current volume
    void onMessageCallback( WebsocketsMessage );                               //! callback for device messages
    String getUrlString( const char * );                                       //! get url for an command
    void onEventCallback( WebsocketsEvent, String );                           //! callback for device events
    void onDecodetMessage( SoundTouchUpdateTmplPtr );                          //! if an message was decodet
    static void start();                                                       //! start task
    static void wsTask( void * );                                              //! the task fuer websocket
    static void decTask( void * );                                             //! xml decoder task
    static void onMessageCallbackWrapper( WebsocketsMessage, uint32_t );       //! wraps callback for members of an object
    static void onEventsCallbackWrapper( WebsocketsEvent, String, uint32_t );  //! wraps callback for members of an object
    static SoundTouchDevice *getInstancePtr( uint32_t );                       //! get instance pointer from the global array
  };
}  // namespace soundtouch
