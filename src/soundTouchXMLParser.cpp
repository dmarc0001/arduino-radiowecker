#include <utility>
#include <yxml.h>
#include "soundTouchXMLParser.hpp"
#include "statics.hpp"

namespace soundtouch
{
  using namespace alarmclock;
  using namespace logger;

  const char *SoundTouchXMLParser::tag{ "xmlparser" };

  SoundTouchXMLParser::SoundTouchXMLParser( XmlMessageList &_xmlList, DecodetMessageList &_msgList )
      : xmlList( _xmlList ), msgList( _msgList )
  {
    elog.log( logger::INFO, "%s: soundtouch xml parser create...", SoundTouchXMLParser::tag );
  }

  SoundTouchXMLParser::~SoundTouchXMLParser()
  {
    elog.log( logger::DEBUG, "%s: soundtouch xml parser delete...", SoundTouchXMLParser::tag );
  }

  bool SoundTouchXMLParser::decodeMessage()
  {
    bool isError{ false };
    if ( xmlList.empty() )
      return true;
    //
    // get the message string
    //
    String msg = std::move( xmlList.front() );
    xmlList.erase( xmlList.begin() );
    // #ifdef BUILD_DEBUG
    //     delay( 50 );
    //     elog.log( DEBUG, "%s: decode xml message \"%s\"...", SoundTouchXMLParser::tag, msg.c_str() );
    //     delay( 250 );
    // #endif
    //
    // first init xml resources
    // 1+10*(stringlen+1)+(50+1)
    //
    size_t msglen = msg.length();
    size_t bufflen = 10 * ( msglen + 1 ) + 52;
    yxml_t *x = static_cast< yxml_t * >( malloc( sizeof( yxml_t ) + bufflen ) );
    if ( !x )
    {
      elog.log( ERROR, "%s: can't allocate %d bytes für xml processing...", SoundTouchXMLParser::tag, bufflen );
      return false;
    }
    yxml_init( x, x + 1, bufflen );
    //
    // parsing message
    //
    elog.log( DEBUG, "%s: parse xml message...", SoundTouchXMLParser::tag );
    ///* The XML document as a zero-terminated string
    WsMsgType type{ WS_UNKNOWN };
    uint8_t depth{ 0 };
    String attrName;
    String elemName;
    String attrVal;
    SoundTouchUpdateTmpl *updatePtr{ nullptr };
    const char *doc = msg.c_str();
    for ( ; *doc; doc++ )
    {
      yxml_ret_t r = yxml_parse( x, *doc );
      // result is.....
      switch ( r )
      {
        case YXML_EEOF:    // Unexpected EOF
        case YXML_EREF:    // Invalid character or entity reference (&whatever;)
        case YXML_ECLOSE:  // Close tag does not match open tag (<Tag> .. </OtherTag>)
        case YXML_ESTACK:  // Stack overflow (too deeply nested tags or too long element/attribute name)
        case YXML_ESYN:    // Syntax error (unexpected byte)
          // an error
          isError = true;
          elog.log( ERROR, "%s: can't parse xml..", SoundTouchXMLParser::tag );
          break;
        case YXML_OK:  // Character consumed, no new token present
          break;
        case YXML_ELEMSTART:
          // element start
          elemName = std::move( String( x->elem ) );
          switch ( depth )
          {
            case 0:
              elog.log( DEBUG, "%s: root element is <%s>", SoundTouchXMLParser::tag, elemName.c_str() );
              type = getMessageType( elemName );
              // not interesting for me
              isError = ( type == WS_UNKNOWN );
              if ( isError )
              {
                elog.log( ERROR, "%s: root element is unknown type, abort parsing!", SoundTouchXMLParser::tag );
              }
              break;
            case 1:
              elog.log( DEBUG, "%s: update type is <%s>...", SoundTouchXMLParser::tag, elemName.c_str() );
              if ( type == WS_UPDATES )
              {
                switch ( getUpdateType( elemName ) )
                {
                  case MSG_UPDATE_VOLUME:
                    // make an object for volume
                    updatePtr = new SoundTouchVolume();
                    break;
                  case MSG_UPDATE_NOW_PLAYING_CHANGED:
                    // make an object for nowPlayingUpdate
                    updatePtr = new SoundTouchNowPlayingUpdate();
                    break;
                  default:
                    if ( updatePtr )
                    {
                      free( updatePtr );
                      updatePtr = nullptr;
                    }
                    elog.log( INFO, "%s: update type <%s> not implemented (yet?)...", SoundTouchXMLParser::tag, elemName.c_str() );
                    break;
                }
              }
              break;
            case 2:
              elog.log( DEBUG, "%s: update property <%s>...", SoundTouchXMLParser::tag, elemName.c_str() );
              break;
            case 3:
              elog.log( DEBUG, "%s: property.sub <%s>...", SoundTouchXMLParser::tag, elemName.c_str() );
              if ( updatePtr )
              {
                switch ( updatePtr->getUpdateType() )
                {
                  case MSG_UPDATE_VOLUME:
                    setVolumeMessageSubPropertys( updatePtr, elemName, attrVal );
                    break;
                  case MSG_UPDATE_NOW_PLAYING_CHANGED:
                    setNowPlayingMessageSubPropertys( updatePtr, elemName, attrVal );
                    break;
                  default:
                    if ( updatePtr )
                    {
                      free( updatePtr );
                      updatePtr = nullptr;
                    }
                    elog.log( INFO, "%s: property.sub <%s> not implemented (yet?)...", SoundTouchXMLParser::tag, elemName.c_str() );
                    break;
                }
              }
              break;
          }
          ++depth;
          break;
        case YXML_CONTENT:
          // content add
          attrVal += std::move( String( x->data ) );
          break;
        case YXML_ELEMEND:
          // element ends here
          --depth;
          elemName.clear();
          attrVal.clear();
          // element endet
          break;
        case YXML_ATTRSTART:
          // init attribute string
          attrName.clear();
          break;
        case YXML_ATTRVAL:
          // append to attribute string
          attrVal += std::move( String( x->data ) );
          break;
        case YXML_ATTREND:
          attrName = std::move( String( x->attr ) );
          //
          // Now we have a full attribute. Its name is in x->attr, and its value is
          // in the string 'attrVal'
          //
          if ( depth == 1 )
          {
            elog.log( DEBUG, "%s: root elem attrib \"%s\" = \"%s\"", SoundTouchXMLParser::tag, attrName.c_str(), attrVal.c_str() );
          }
          attrVal.clear();
          break;
        case YXML_PISTART:   /* Start of a processing instruction          */
        case YXML_PICONTENT: /* Content of a PI                            */
        case YXML_PIEND:     /* End of a processing instruction            */
        default:
          break;
      }
      if ( isError )
        break;

      /* Handle any tokens we are interested in */
    }
    if ( !isError )
      elog.log( DEBUG, "%s: parse xml message...DONE", SoundTouchXMLParser::tag );

    // DO NOT FORGET!!!!!!
    free( x );
    if ( updatePtr )
      if ( updatePtr->isValid )
      {
        SoundTouchUpdateTmplPtr savePtr( std::move( updatePtr ) );
        updatePtr = nullptr;
        msgList.push_back( savePtr );
      }
    return !isError;
  }

  bool SoundTouchXMLParser::setNowPlayingMessageSubPropertys( SoundTouchUpdateTmpl *ptr, String &elemName, String &attrVal )
  {
    //
    // polymorph class, convert pointer
    //
    SoundTouchNowPlayingUpdate *dev = static_cast< SoundTouchNowPlayingUpdate * >( ptr );
    if ( elemName.equals( UPDATE_PROPERTY_NPLAY_CONTENT ) )
    {
      // subentry contenItem found
      // not nessesary here
    }
    else if ( elemName.equals( UPDATE_PROPERTY_NPLAY_TRACK ) )
    {
      // which track is playing
      dev->track = attrVal;
    }
    else if ( elemName.equals( UPDATE_PROPERTY_NPLAY_ALBUM ) )
    {
      // which album is playing
      dev->album = attrVal;
    }
    else if ( elemName.equals( UPDATE_PROPERTY_NPLAY_STATIONNAME ) )
    {
      // which album is playing
      dev->album = attrVal;
    }
    else if ( elemName.equals( UPDATE_PROPERTY_NPLAY_ART ) )
    {
      // which album is playing
      dev->art = attrVal;
    }
    else if ( elemName.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATUS ) )
    {
      // which album is playing
      dev->playStatus = getPlayingType( attrVal );
      dev->isValid = true;
    }
    else if ( elemName.equals( UPDATE_PROPERTY_NPLAY_STREAMTYPE ) )
    {
      // which album is playing
      dev->streamingType = getStreamingType( attrVal );
    }
    return dev->isValid;
    // <nowPlaying deviceID="689E19653E96" source="TUNEIN" sourceAccount="">
    //   <ContentItem source="TUNEIN" type="stationurl" location="/v1/playback/station/s24950" sourceAccount="" isPresetable="true">
    //     <itemName>91.4 Berliner Rundfunk</itemName>
    //     <containerArt>http://cdn-profiles.tunein.com/s24950/images/logoq.jpg?t=160315</containerArt>
    //   </ContentItem>
    //   <track>Berliner Rundfunk</track>
    //   <artist>Berliner Rundfunk 91.4 - Die besten Hits aller Zeiten</artist>
    //   <album></album>
    //   <stationName>Berliner Rundfunk</stationName>
    //   <art artImageStatus="IMAGE_PRESENT">http://cdn-profiles.tunein.com/s24950/images/logog.jpg?t=637387494910000000</art>
    //   <favoriteEnabled />
    //   <playStatus>BUFFERING_STATE</playStatus>
    //   <streamType>RADIO_STREAMING</streamType>
    // </nowPlaying>
  }

  /**
   * set sub property in volume setting
   */
  bool SoundTouchXMLParser::setVolumeMessageSubPropertys( SoundTouchUpdateTmpl *ptr, String &elemName, String &attrVal )
  {
    //
    // polymorph class, convert pointer
    //
    SoundTouchVolume *dev = static_cast< SoundTouchVolume * >( ptr );
    //
    if ( elemName.equals( UPDATE_PROPERTY_VOL_TARGET ) )
    {
      dev->targetVol = static_cast< uint8_t >( attrVal.toInt() & 0xff );
    }
    else if ( elemName.equals( UPDATE_PROPERTY_VOL_CURR ) )
    {
      dev->currVol = static_cast< uint8_t >( attrVal.toInt() & 0xff );
    }
    else if ( elemName.equals( UPDATE_PROPERTY_VOL_MUTE ) )
    {
      dev->currVol = attrVal.equals( "true" ) ? true : false;
    }
    if ( dev->currVol != 255 && dev->targetVol != 255 )
      dev->isValid = true;
    return dev->isValid;
  }

  /**
   * get type of update message
   */
  WsMsgUpdateType SoundTouchXMLParser::getUpdateType( String &_update )
  {
    if ( _update.equals( UPDATE_VOLUME ) )
      return MSG_UPDATE_VOLUME;
    else if ( _update.equals( UPDATE_NOW_PLAYING ) )
      return MSG_UPDATE_NOW_PLAYING_CHANGED;
    else if ( _update.equals( UPDATE_PRESETS ) )
      return MSG_UPDATE_PRESETS;
    else if ( _update.equals( UPDATE_ZONE ) )
      return MSG_UPDATE_ZONE;
    return MSG_UNKNOWN;

    // UPDATE_BASS{ "bassUpdated" }
    // UPDATE_RECENT{ "recentsUpdated" }
    // UPDATE_ACC_MODE{ "acctModeUpdated" }
    // UPDATE_SOURCES{ "sourcesUpdated" }
    // UPDATE_NOW_SELECT{ "nowSelectionUpdated" }
    // UPDATE_CONNECTION_STATE{ "connectionStateUpdated" }
    // UPDATE_INFO{ "infoUpdated" }
    // UPDATE_SOFTWARE_STATE{ "swUpdateStatusUpdated" }
    // UPDATE_SITE_SURVEY_RES{ "siteSurveyResultsUpdated" }

    // MSG_UPDATE_BASS,                 // bassUpdated
    // MSG_UPDATE_RECENTS,              // recentsUpdated
    // MSG_UPDATE_ACCMODE,              // acctModeUpdated
    // MSG_UPDATE_SOURCES,              // sourcesUpdated
    // MSG_UPDATE_NOW_SELECTION,        // nowSelectionUpdated
    // MSG_UPDATE_CONNECTION_STATE,     // connectionStateUpdated
    // MSG_UPDATE_INFOS,                // infoUpdated
    // MSG_UPDATE_SOFTWARE,             // swUpdateStatusUpdated
    // MSG_UPDATE_SITE_THINGS,          // siteSurveyResultsUpdated
    // MSG_UNKNOWN
  }

  /**
   * get type of message
   */
  WsMsgType SoundTouchXMLParser::getMessageType( String &_type )
  {
    if ( _type.equals( MSG_TYPE_UPDATES ) )
    {
      return WS_UPDATES;
    }
    else if ( _type.equals( MSG_TYPE_USER_UPDATES ) )
    {
      return WS_USER_ACTIVITY_UPDATES;
    }
    return WS_UNKNOWN;
  }

  /**
   * get status of buffering
   */
  WsPlayStatus SoundTouchXMLParser::getPlayingType( String &_state )
  {
    if ( _state.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATE_PLAY ) )
    {
      return PLAY_STATE;
    }
    else if ( _state.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATE_PAUSE ) )
    {
      return PAUSE_STATE;
    }
    else if ( _state.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATE_STOP ) )
    {
      return STOP_STATE;
    }
    else if ( _state.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATE_BUFFER ) )
    {
      return BUFFERING_STATE;
    }
    return INVALID_PLAY_STATUS;
  }

  /**
   * get Stream type (not compete yet)
   * TODO: all the stream types
   */
  WsStreamingTypes SoundTouchXMLParser::getStreamingType( String &_type )
  {
    if ( _type.equals( UPDATE_PROPERTY_NPLAY_STREAMTYPE_RADIO ) )
    {
      return STREAM_RADIO_STREAMING;
    }
    else if ( _type.equals( UPDATE_PROPERTY_NPLAY_STREAMTYPE_ONDEMAND ) )
    {
      return STREAM_TRACK_ONDEMAND;
    }
    return STREAM_UNKNOWN;
  }
}  // namespace soundtouch