#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <vector>
#include <utility>
#include <ArduinoWebsockets.h>
#include "common.hpp"
#include "appStructs.hpp"

namespace soundtouch
{
  using namespace websockets;

  class SoundtouchDevice;

  using InstancePtr = std::pair< uint32_t, SoundtouchDevice * >;
  using InstancesList = std::vector< InstancePtr >;

  class SoundtouchDevice
  {
    private:
    static const char *tag;
    static TaskHandle_t taskHandle;
    static uint32_t instances;
    uint32_t instance;
    static InstancesList instList;
    WebsocketsClient wsClient;
    AlarmClockSrv::DeviceEntry device;
    SoundtouchDevice();  //! no default constructor!

    public:
    explicit SoundtouchDevice( AlarmClockSrv::DeviceEntry & );
    ~SoundtouchDevice();

    private:
    void onMessageCallback( WebsocketsMessage );
    void onEventCallback( WebsocketsEvent, String );
    static void start();           //! start task
    static void wsTask( void * );  //! the task fuer websocket
    static void onMessageCallbackWrapper( WebsocketsMessage, uint32_t );
    static void onEventsCallbackWrapper( WebsocketsEvent, String, uint32_t );
    static SoundtouchDevice *getInstancePtr( uint32_t );
  };
}  // namespace soundtouch
