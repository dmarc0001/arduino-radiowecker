#include <utility>
#include <yxml.h>
#include "soundTouchXMLParser.hpp"
#include "statics.hpp"

namespace soundtouch
{
  using namespace alertclock;
  using namespace logger;

  const char *SoundTouchXMLParser::tag{ "xmlparser" };

  /**
   * constructor, set some propertys
   */
  SoundTouchXMLParser::SoundTouchXMLParser( XmlMessageList &_xmlList, DecodetMessageList &_msgList )
      : xmlList( _xmlList ), msgList( _msgList )
  {
    elog.log( logger::INFO, "%s: soundtouch xml parser create...", SoundTouchXMLParser::tag );
    // init semaphore
    vSemaphoreCreateBinary( parseSem );
    // free semaphore
    xSemaphoreGive( parseSem );
  }

  /**
   * destructor, clean up
   */
  SoundTouchXMLParser::~SoundTouchXMLParser()
  {
    elog.log( logger::DEBUG, "%s: soundtouch xml parser delete...", SoundTouchXMLParser::tag );
  }

  /**
   * decode message, runs from a setparate task, be arare then this runs single
   */
  bool SoundTouchXMLParser::decodeMessage()
  {
    SoundTouchXmlLoopParams params;
    if ( xmlList.empty() )
      return true;
    //
    // if semaphore free, process, if not try it at neyt round
    if ( xSemaphoreTake( parseSem, pdMS_TO_TICKS( 10 ) ) == pdTRUE )
    {
      //
      // get the message string
      //
      String msg = std::move( xmlList.front() );
      xmlList.erase( xmlList.begin() );
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
        xSemaphoreGive( parseSem );
        return false;
      }
      // init the structures
      yxml_init( x, x + 1, bufflen );
      //
      // parsing message
      //
      elog.log( DEBUG, "%s: parse xml message...", SoundTouchXMLParser::tag );
      ///* The XML document as a zero-terminated string
      const char *doc = msg.c_str();
      for ( ; *doc; doc++ )
      {
        yield();
        yxml_ret_t r = yxml_parse( x, *doc );
        // result is type.....
        switch ( r )
        {
          case YXML_EEOF:    // Unexpected EOF
          case YXML_EREF:    // Invalid character or entity reference (&whatever;)
          case YXML_ECLOSE:  // Close tag does not match open tag (<Tag> .. </OtherTag>)
          case YXML_ESTACK:  // Stack overflow (too deeply nested tags or too long element/attribute name)
          case YXML_ESYN:    // Syntax error (unexpected byte)
            // an error
            params.isError = true;
            elog.log( ERROR, "%s: can't parse xml..", SoundTouchXMLParser::tag );
            break;
          case YXML_OK:  // Character consumed, no new token present
            break;
          case YXML_ELEMSTART:
            // element started his name is....
            params.attrName = std::move( String( x->elem ) );
            if ( params.depth == 0 )
            {
              //
              // first lebel (rootlevel)
              //
              // elog.log( DEBUG, "%s: <root> element is <%s> level <%d>", SoundTouchXMLParser::tag, params.attrName.c_str(),
              //           params.depth );
              params.type = getMessageType( params.attrName );
              // not interesting for me
              params.isError = ( params.type == WS_UNKNOWN );
              if ( params.isError )
              {
                elog.log( ERROR, "%s: <root> element <%s> level <%d> is unknown type, abort parsing!", SoundTouchXMLParser::tag,
                          params.attrName.c_str(), params.depth );
              }
            }
            else
            {
              // compute level 1-x
              computeElemStart( params );
              yield();
            }
            ++params.depth;
            break;
          case YXML_CONTENT:
            // xml element content add to value
            params.attrVal += std::move( String( x->data ) );
            break;
          case YXML_ELEMEND:
            // element ends here
            --params.depth;
            computeElemEnd( params );
            yield();
            params.attrName.clear();
            params.attrVal.clear();
            // element endet
            break;
          case YXML_ATTRSTART:
            // init attribute string
            params.attrName.clear();
            break;
          case YXML_ATTRVAL:
            // append to attribute string
            params.attrVal += std::move( String( x->data ) );
            break;
          case YXML_ATTREND:
            params.attrName = std::move( String( x->attr ) );
            //
            // Now we have a full attribute. Its name is in x->attr, and its value is
            // in the string 'attrVal'
            //
            computeAttrEnd( params );
            params.attrVal.clear();
            break;
          case YXML_PISTART: /* Start of a processing instruction          */
            params.piVal.clear();
          case YXML_PICONTENT: /* Content of a PI                            */
            params.piVal += String( x->pi );
          case YXML_PIEND: /* End of a processing instruction            */
            elog.log( DEBUG, "%s: <level %d> pi \"%s\" = \"%s\"", SoundTouchXMLParser::tag, params.depth, params.attrName.c_str(),
                      params.attrVal.c_str() );
          default:
            break;
        }
        if ( params.isError )
          break;

        /* Handle any tokens we are interested in */
        yield();
      }
      if ( !params.isError )
      {
        elog.log( DEBUG, "%s: parse xml message...DONE", SoundTouchXMLParser::tag );
      }
      else
      {
        elog.log( ERROR, "%s: parse xml message...UNDONE", SoundTouchXMLParser::tag );
      }
      // DO NOT FORGET!!!!!!
      free( x );
      if ( params.updatePtr )
      {
        if ( params.updatePtr->isValid )
        {
          elog.log( DEBUG, "%s: decoded message to queue!", SoundTouchXMLParser::tag );
          if ( params.updatePtr->deviceID.isEmpty() )
          {
            params.updatePtr->deviceID = params.deviceID;
          }
          SoundTouchUpdateTmplPtr savePtr( std::move( params.updatePtr ) );
          msgList.push_back( savePtr );
          params.updatePtr = nullptr;
        }
        else
        {
          elog.log( DEBUG, "%s: decoded message not valid!", SoundTouchXMLParser::tag );
        }
      }
      else
      {
        elog.log( ERROR, "%s: no decoded object created!", SoundTouchXMLParser::tag );
      }
    }
    xSemaphoreGive( parseSem );
    return !params.isError;
  }

  /**
   * workload for start of an element
   */
  void SoundTouchXMLParser::computeElemStart( SoundTouchXmlLoopParams &p )
  {
    switch ( p.depth )
    {
      case 0:
        // handled in main loop
        break;
      case 1:
        // elog.log( DEBUG, "%s: <level 1> element update type is <%s>...", SoundTouchXMLParser::tag, p.elemName.c_str() );
        if ( p.type == WS_UPDATES )
        {
          switch ( getUpdateType( p.attrName ) )
          {
            case MSG_UPDATE_VOLUME:
              // make an object for volume
              if ( p.updatePtr )
                free( p.updatePtr );
              p.updatePtr = new SoundTouchVolumeUpdate();
              break;
            case MSG_UPDATE_NOW_PLAYING_CHANGED:
              // make an object for nowPlayingUpdate
              if ( p.updatePtr )
                free( p.updatePtr );
              p.updatePtr = new SoundTouchNowPlayingUpdate();
              break;
            case MSG_UPDATE_ZONE:
              // make an object for zone updates
              if ( p.updatePtr )
                free( p.updatePtr );
              p.updatePtr = new SoundTouchZoneUpdate();
              break;
            case MSG_UPDATE_RECENTS:
              // elog.log( DEBUG, "%s: ignore recents update!", SoundTouchXMLParser::tag );
            default:
              if ( p.updatePtr )
              {
                free( p.updatePtr );
                p.updatePtr = nullptr;
              }
              // elog.log( INFO, "%s: <level 1> element update type <%s> not implemented (yet?)...", SoundTouchXMLParser::tag,
              //           p.elemName.c_str() );
              break;
          }
        }
        break;
      case 2:
        // elog.log( DEBUG, "%s: <level 2> element update property <%s>...", SoundTouchXMLParser::tag, p.elemName.c_str() );
        if ( p.updatePtr )
        {
          switch ( p.updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_ZONE:
              // elog.log( DEBUG, "%s: <level 2> await attribute for zone master...", SoundTouchXMLParser::tag );
              p.updatePtr->isValid = true;
              // await attribute ipaddr
              break;
            default:
              break;
          }
        }
        break;
      case 3:
        // elog.log( DEBUG, "%s: <level 3> element property.sub <%s>...", SoundTouchXMLParser::tag, p.elemName.c_str() );
        if ( p.updatePtr )
        {
          switch ( p.updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_VOLUME:
              // values at level 4 while elem end
              break;
            case MSG_UPDATE_NOW_PLAYING_CHANGED:
              // setNowPlayingMessageSubPropertys( p.updatePtr, p.elemName, p.attrVal );
              break;
            case MSG_UPDATE_ZONE:
              // elog.log( DEBUG, "%s: <level 2> await attribute for member...", SoundTouchXMLParser::tag );
              if ( p.zoneMember )
                free( p.zoneMember );
              p.zoneMember = new SoundTouchZoneMember;
              p.updatePtr->isValid = true;
              break;
            default:
              if ( p.updatePtr )
              {
                free( p.updatePtr );
                p.updatePtr = nullptr;
              }
              // elog.log( INFO, "%s: <level 3> element property.sub <%s> not implemented (yet?)...", SoundTouchXMLParser::tag,
              //           p.elemName.c_str() );
              break;
          }
        }
        break;
      case 4:
        // elog.log( DEBUG, "%s: <level 4> element property.sub <%s>...", SoundTouchXMLParser::tag, p.elemName.c_str() );
        break;
    }
  }

  /**
   * workload for end of an element
   */
  void SoundTouchXMLParser::computeElemEnd( SoundTouchXmlLoopParams &p )
  {
    switch ( p.depth )
    {
      case 3:
        if ( p.updatePtr )
        {
          switch ( p.updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_ZONE:
              // zone member is an attribute
              // elog.log( DEBUG, "%s: <level 3> content  \"%s\" = \"%s\"...", SoundTouchXMLParser::tag, p.elemName.c_str(),
              //           p.attrVal.c_str() );
              //
              // fill in in message
              //
              if ( p.zoneMember )
              {
                p.zoneMember->id = p.attrVal;
                static_cast< SoundTouchZoneUpdate * >( p.updatePtr )->members.push_back( *( std::move( p.zoneMember ) ) );
              }
              static_cast< SoundTouchZoneUpdate * >( p.updatePtr )->isValid = true;
              free( p.zoneMember );
              p.zoneMember = nullptr;
              // elog.log( DEBUG, "%s: zonemembers: %d", SoundTouchXMLParser::tag,
              //           static_cast< SoundTouchZoneUpdate * >( p.updatePtr )->members.size() );
              break;
            case MSG_UPDATE_NOW_PLAYING_CHANGED:
              setNowPlayingMessageSubPropertys( p.updatePtr, p.attrName, p.attrVal );
              break;
            default:
              break;
          }
        }
      case 4:
        if ( p.updatePtr )
        {
          switch ( p.updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_VOLUME:
              setVolumeMessageSubPropertys( p.updatePtr, p.attrName, p.attrVal );
              break;
          }
        }
      default:
        break;
    }
  }

  /**
   * workload for end an attribute
   */
  void SoundTouchXMLParser::computeAttrEnd( SoundTouchXmlLoopParams &p )
  {
    switch ( p.depth - 1 )
    {
      case 0:
        // elog.log( DEBUG, "%s: <root> attrib \"%s/%s\" = \"%s\"", SoundTouchXMLParser::tag, p.elemName.c_str(), p.attrName.c_str(),
        //           p.attrVal.c_str() );
        if ( p.attrName.equals( UPDATE_PROPERTY_ATTR_DEVICE_ID ) )
        {
          p.deviceID = p.attrVal;
        }
        break;
      case 2:
        if ( p.updatePtr )
        {
          //
          // is there the awaited attrib for zone
          //
          if ( p.attrName.equals( UPDATE_PROPERTY_ZONE_ZONE ) && p.attrName.equals( UPDATE_PROPERTY_ZONE_ATTRIB_MASTER ) )
          {
            static_cast< SoundTouchZoneUpdate * >( p.updatePtr )->masterID = p.attrVal;
            // elog.log( DEBUG, "%s: <level 2> attrib \"%s/%s\" = \"%s\"", SoundTouchXMLParser::tag, p.elemName.c_str(),
            //           p.attrName.c_str(), p.attrVal.c_str() );
          }
          //
          // is there an attrib "source" in elem nowPlaying?
          //
          if ( p.attrName.equals( UPDATE_PROPERTY_NPLAY_NOW_PLAYING ) && p.attrName.equals( UPDATE_PROPERTY_ATTR_SOURCE ) )
          {
            // elog.log( DEBUG, "%s: <level 2> attrib \"%s/%s\" = \"%s\"", SoundTouchXMLParser::tag, p.elemName.c_str(),
            //           p.attrName.c_str(), p.attrVal.c_str() );
            static_cast< SoundTouchNowPlayingUpdate * >( p.updatePtr )->source = p.attrVal;
            if ( p.attrVal.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATE_STANDBY ) )
            {
              static_cast< SoundTouchNowPlayingUpdate * >( p.updatePtr )->playStatus = STANDBY_STATE;
            }
            p.updatePtr->isValid = true;
          }
        }
        break;
      case 3:
        if ( p.updatePtr )
        {
          switch ( p.updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_ZONE:
              // zone member is an attribute
              // elog.log( DEBUG, "%s: <level 3> attr  \"%s/%s\" = \"%s\"...", SoundTouchXMLParser::tag, p.elemName.c_str(),
              //           p.attrName.c_str(), p.attrVal.c_str() );
              if ( p.zoneMember )
              {
                IPAddress addr;
                addr.fromString( p.attrVal );
                p.zoneMember->ip = addr;
              }
              // setZoneUpdateMessageSubPropertys( updatePtr, attrName, attrVal );
              break;
            default:
              break;
          }
        }
        break;
      case 4:
        // elog.log( DEBUG, "%s: <level 4> attrib \"%s/%s\" = \"%s\"", SoundTouchXMLParser::tag, p.elemName.c_str(),
        // p.attrName.c_str(),
        //           p.attrVal.c_str() );
        break;
    }
  }

  /**
   * set propertys for an nowPlayingUpdate message
   */
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
      dev->stationName = attrVal;
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
      // elog.log( ERROR, "%s: PLAYSTATE: %s (%d)", SoundTouchXMLParser::tag, attrVal.c_str(), dev->playStatus );
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
    SoundTouchVolumeUpdate *dev = static_cast< SoundTouchVolumeUpdate * >( ptr );
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
      dev->mute = attrVal.equals( "true" ) ? true : false;
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
    else if ( _update.equals( UPDATE_NOW_PLAYING ) )
      return MSG_UPDATE_RECENTS;
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
    else if ( _type.equals( MSG_TYPE_SDKINFO ) )
    {
      return ( WS_USER_SDK_INFO );
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