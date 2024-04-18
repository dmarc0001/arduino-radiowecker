#pragma once
#include <memory>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "appPreferences.hpp"
#include "appStructs.hpp"
#include "soundTouchDefs.hpp"
#include "soundTouchAlert.hpp"

namespace alertclock
{
  class AlertTask
  {
    private:
    static const char *tag;                                     //! tag for debug
    static bool isRunning;                                      //! tsk is running
    static soundtouch::SoundTouchAlertPtrList activeAlertList;  //! list with active tasks

    public:
    static void start();

    private:
    static void alTask( void * );  //! task for aleerts
    static bool startAlert( AlertEntryPtr );
    static soundtouch::SoundTouchDeviceRunningMode alertLoop( soundtouch::SoundTouchAlertPtr );
  };
}  // namespace alertclock
