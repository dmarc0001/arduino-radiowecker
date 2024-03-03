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
    static const char *tag;               //! tag for messages
    bool isInit;                          //! was device initialized
    uint8_t oldVolume;                    //! save old volume
    SoundTouchDevicePtr sdDevice;         //! sd device object
    alarmclock::AlertEntryPtr alertEntr;  //! alert data obkject
    SoundTouchAlert();                    //! private constructor, not able to instance

    public:
    explicit SoundTouchAlert( SoundTouchDevicePtr , alarmclock::AlertEntryPtr );
    ~SoundTouchAlert();                                  //! destructor
    bool init();                                         //! init device
    SoundTouchDeviceRunningMode getDeviceRunningMode();  //! runmode from device
    bool prepareAlertDevivce();                          //! init device for the alert
    bool checkRunningAlert();                            //! check in a loop for alert's work
  };
}  // namespace soundtouch