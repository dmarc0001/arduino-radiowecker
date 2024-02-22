#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "common.hpp"
#include "appStructs.hpp"

namespace soundtouch
{
  class SoundTouchXMLParser
  {
    private:
    static const char *tag;
    XmlMessageList &xmlList;
    DecodetMessageList &msgList;

    public:
    explicit SoundTouchXMLParser( XmlMessageList &, DecodetMessageList & );
    ~SoundTouchXMLParser();
    bool decodeMessage();
  };
}  // namespace soundtouch