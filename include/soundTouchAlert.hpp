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

  using SoundTouchAlertList = std::vector< SoundTouchAlert >;
  using SoundTouchAlertListPtr = std::shared_ptr< SoundTouchAlertList >;

  class SoundTouchAlert
  {
    private:
    static const char *tag;                               //! tag for messages
    bool isInit;                                          //! was device initialized
    uint8_t oldVolume;                                    //! save old volume
    std::shared_ptr< SoundTouchDevice > sdDevice;         //! device object
    std::shared_ptr< alarmclock::AlertEntry > alertEntr;  //! alert data obkject
    SoundTouchAlert();                                    //! private constructor, not able to instance

    public:
    explicit SoundTouchAlert( alarmclock::DeviceEntry &, alarmclock::AlertEntry & );
    ~SoundTouchAlert();                                  //! destructor
    bool init();                                         //! init device
    SoundTouchDeviceRunningMode getDeviceRunningMode();  //! runmode from device
    bool prepareAlertDevivce();                          //! init device for the alert
    bool checkRunningAlert();                            //! check in a loop for alert's work
  };
}  // namespace soundtouch