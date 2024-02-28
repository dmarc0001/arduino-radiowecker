#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <vector>
#include <memory>
#include "common.hpp"
#include "appStructs.hpp"
#include "soundTouchDefs.hpp"
#include "soundtouchDevice.hpp"

namespace soundtouch
{
  class SoundTouchAlert
  {
    private:
    static const char *tag;
    bool isInit;
    std::shared_ptr< SoundTouchDevice > sdDevice;
    SoundTouchAlert();

    public:
    explicit SoundTouchAlert( alarmclock::DeviceEntry & );
    ~SoundTouchAlert();
    bool init(unsigned long timeout = TIMEOUNT_WHILE_DEVICE_INIT);
  };
}  // namespace soundtouch