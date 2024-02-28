#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "common.hpp"
#include "appStructs.hpp"

namespace soundtouch
{
  struct SoundTouchXmlLoopParams
  {
    bool isError{ false };
    WsMsgType type{ WS_UNKNOWN };
    String deviceID;
    uint8_t depth{ 0 };
    String elemName;
    String piVal;
    String attrName;
    String attrVal;
    SoundTouchUpdateTmpl *updatePtr{ nullptr };
    SoundTouchZoneMember *zoneMember{ nullptr };
  };

  class SoundTouchXMLParser
  {
    private:
    static const char *tag;
    XmlMessageList &xmlList;
    DecodetMessageList &msgList;
    SemaphoreHandle_t parseSem;

    public:
    explicit SoundTouchXMLParser( XmlMessageList &, DecodetMessageList & );
    ~SoundTouchXMLParser();
    bool decodeMessage();

    private:
    void computeElemStart( SoundTouchXmlLoopParams & );
    void computeElemEnd( SoundTouchXmlLoopParams & );
    void computeAttrEnd( SoundTouchXmlLoopParams & );
    WsMsgType getMessageType( String & );
    WsMsgUpdateType getUpdateType( String & );
    WsPlayStatus getPlayingType( String & );
    WsStreamingTypes getStreamingType( String & );
    bool setVolumeMessageSubPropertys( SoundTouchUpdateTmpl *, String &, String & );
    bool setNowPlayingMessageSubPropertys( SoundTouchUpdateTmpl *, String &, String & );
  };
}  // namespace soundtouch