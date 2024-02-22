#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <TinyXML.h>
#include "common.hpp"
#include "appStructs.hpp"

namespace soundtouch
{
  class SoundTouchXMLParser
  {
    SoundTouchXMLParser();
    ~SoundTouchXMLParser();
  };
}  // namespace soundtouch