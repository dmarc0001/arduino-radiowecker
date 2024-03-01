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

namespace alarmclock
{
  class AlertTask
  {
    private:
    static const char *tag;
    static bool isRunning;
    static soundtouch::SoundTouchAlertListPtr acticeAlertList;

    public:
    static void start();

    private:
    static void alTask( void * );
  };
}  // namespace alarmclock
