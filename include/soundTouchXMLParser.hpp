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

    private:
    WsMsgType getMessageType( String & );
    WsMsgUpdateType getUpdateType( String & );
    WsPlayStatus getPlayingType( String & );
    WsStreamingTypes getStreamingType( String & );
    bool setVolumeMessageSubPropertys( SoundTouchUpdateTmpl *, String &, String & );
    bool setNowPlayingMessageSubPropertys( SoundTouchUpdateTmpl *, String &, String & );
  };
}  // namespace soundtouch