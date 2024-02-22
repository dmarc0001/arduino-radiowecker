#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "appPreferences.hpp"
#include "appStructs.hpp"

namespace alarmclock
{
  class AlertTask
  {
    private:
    static const char *tag;
    static bool isRunning;

    public:
    static void start();

    private:
    static void alTask( void * );
  };
}  // namespace alarmclock
