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
  class SoundTouchXmlLoopParams
  {
    public:
    bool isError{ false };                        //! is an erroer occured
    WsMsgType type{ WS_UNKNOWN };                 //! the current message type
    String deviceID;                              //! what is the current device id
    uint8_t depth{ 0 };                           //! depth (level) in XML Objectl
    String piVal;                                 //! value of process instruction
    String currElemName;                          //! name of the attribute inner of element
    String currAttrName;                          //! name of the attribute inner of element
    String value;                                 //! value of the attribute inner of the element
    String piName;                                //! name of the processing instruction
    SoundTouchUpdateTmpl *updatePtr{ nullptr };   //! pointer to the current update object
    SoundTouchZoneMember *zoneMember{ nullptr };  //! if zoen update => pointzrer to zone members
    void init();                                  //! init values
    SoundTouchXmlLoopParams()                     //! constructor
    {
      init();
    }
  };
  using SoundTouchXmlLoopParamsPtr = std::shared_ptr< SoundTouchXmlLoopParams >;

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
    void createSoundTouchObject( SoundTouchXmlLoopParamsPtr );  //! create soundTouchObjects for Type (XML Level 1)
    void computeElemStart( SoundTouchXmlLoopParamsPtr );        //! compute xml tag start
    void computeElemEnd( SoundTouchXmlLoopParamsPtr );          //! compute XML Tag end
    void computeAttrEnd( SoundTouchXmlLoopParamsPtr );          //! compute Attribute's end  in an element
    WsMsgType getSoundTouchMessageType( String & );             //! get Messagetype from String
    WsMsgUpdateType getSoundTouchUpdateType( String & );        //! get update type from string
    WsPlayStatus getPlayingStateType( String & );               //! get type of play from string
    WsStreamingTypes getPlayingStreamingType( String & );       //! get type of streaming from string
    bool getVolumeMessagePropertys( SoundTouchUpdateTmpl *, String &, String & );
    bool getNowPlayingMessagePropertys( SoundTouchUpdateTmpl *, String &, String & );
  };
}  // namespace soundtouch