#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <vector>
#include <memory>
#include "common.hpp"
#include "appPreferences.hpp"
#include "appStructs.hpp"
#include "soundTouchDefs.hpp"
#include "soundtouchDevice.hpp"

namespace soundtouch
{
  class SoundTouchAlert;

  using SoundTouchAlertPtr = std::shared_ptr< SoundTouchAlert >;
  using SoundTouchAlertPtrList = std::vector< SoundTouchAlertPtr >;

  class SoundTouchAlert
  {
    private:
    static const char *tag;                  //! tag for messages
    bool isInit;                             //! was device initialized
    uint8_t oldVolume;                       //! save old volume
    SoundTouchDevicePtr sdDevice;            //! sd device object
    alertclock::AlertEntryPtr alertEntr;     //! alert data obkject
    bool alertIsStart{ false };              //! runningAlertVar
    bool runningAlertWasOkay{ false };       //! runningAlertVar
    int64_t runningAlertNextVolStep{ 0LL };  //! runningAlertVar
    int64_t runningAlertVolEndTime{ 0LL };   //! runningAlertVar
    uint8_t runningAlertNextDestVol{ 0 };    //! runningAlertVar

    SoundTouchAlert();  //! private constructor, not able to instance

    public:
    explicit SoundTouchAlert( SoundTouchDevicePtr, alertclock::AlertEntryPtr );
    ~SoundTouchAlert();                                  //! destructor
    bool init();                                         //! init device
    SoundTouchDeviceRunningMode getDeviceRunningMode();  //! runmode from device
    bool prepareAlertDevivce();                          //! init device for the alert
    bool checkRunningAlert();                            //! check in a loop for alert's work
    WsPlayStatus getPlayState()                          //! which playstate has the device
    {
      return sdDevice->getPlayState();
    }
    String getAlertName()  //! which alert is this (get name from config)
    {
      return alertEntr->name;
    }
  };
}  // namespace soundtouch