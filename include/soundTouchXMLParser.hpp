#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "common.hpp"
#include "appStructs.hpp"

namespace soundtouch
{
  /**
   * parameters for subroutines
   * subroutines for make ist sipler to read
   */
  struct SoundTouchXmlLoopParams
  {
    bool isError{ false };         //! is an erroer occured
    WsMsgType type{ WS_UNKNOWN };  //! the current message type
    String deviceID;               //! what is the current device id
    uint8_t depth{ 0 };  //! depth (level) in XML Objectl    String elemName; //! name of the current element in current depth/Level
    String piVal;        //! value of process instruction
    String attrName;     //! name of the attribute inner of element
    String attrVal;      //! value of the attribute inner of the element
    SoundTouchUpdateTmpl *updatePtr{ nullptr };   //! pointer to the current update object
    SoundTouchZoneMember *zoneMember{ nullptr };  //! if zoen update => pointzrer to zone members
  };

  class SoundTouchXMLParser
  {
    private:
    static const char *tag;       //! tag for debugging
    XmlMessageList &xmlList;      //! list of raw xml messages
    DecodetMessageList &msgList;  //! list of decodet messages (sourced from xml message)
    SemaphoreHandle_t parseSem;   //! semaphore for decoding process

    public:
    explicit SoundTouchXMLParser( XmlMessageList &, DecodetMessageList & );  // the constructor
    ~SoundTouchXMLParser();                                                  //! destructor
    bool decodeMessage();                                                    //! decode the message

    private:
    void computeElemStart( SoundTouchXmlLoopParams & );
    void computeElemEnd( SoundTouchXmlLoopParams & );
    void computeAttrEnd( SoundTouchXmlLoopParams & );
    WsMsgType getMessageType( String & );           //! get Messagetype from String
    WsMsgUpdateType getUpdateType( String & );      //! get update type from string
    WsPlayStatus getPlayingType( String & );        //! get type of play from string
    WsStreamingTypes getStreamingType( String & );  //! get type of streaming from string
    bool setVolumeMessageSubPropertys( SoundTouchUpdateTmpl *, String &, String & );
    bool setNowPlayingMessageSubPropertys( SoundTouchUpdateTmpl *, String &, String & );
  };
}  // namespace soundtouch