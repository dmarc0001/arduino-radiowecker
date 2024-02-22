#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <vector>
#include <memory>
#include <ArduinoWebsockets.h>
#include "common.hpp"
#include "appStructs.hpp"
#include "soundTouchXMLParser.hpp"

namespace soundtouch
{
  using namespace websockets;

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
    WebsocketsClient wsClient;
    alarmclock::DeviceEntry device;
    XmlMessageList xmlList;
    DecodetMessageList msgList;
    SoundTouchXMLParserPtr parser;
    SoundTouchDevice();  //! no default constructor!

    public:
    explicit SoundTouchDevice( alarmclock::DeviceEntry & );
    ~SoundTouchDevice();

    private:
    void onMessageCallback( WebsocketsMessage );
    void onEventCallback( WebsocketsEvent, String );
    void onDecodetMessage( SoundTouchUpdateTmplPtr );
    static void start();            //! start task
    static void wsTask( void * );   //! the task fuer websocket
    static void decTask( void * );  //! xml decoder task
    static void onMessageCallbackWrapper( WebsocketsMessage, uint32_t );
    static void onEventsCallbackWrapper( WebsocketsEvent, String, uint32_t );
    static SoundTouchDevice *getInstancePtr( uint32_t );
  };
}  // namespace soundtouch
