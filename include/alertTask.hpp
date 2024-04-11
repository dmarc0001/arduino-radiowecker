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
    static const char *tag;
    static bool isRunning;
    static soundtouch::SoundTouchAlertPtrList activeAlertList;

    public:
    static void start();

    private:
    static void alTask( void * );
    static bool startAlert( AlertEntryPtr );
    static soundtouch::SoundTouchDeviceRunningMode alertLoop( soundtouch::SoundTouchAlertPtr );
  };
}  // namespace alertclock
