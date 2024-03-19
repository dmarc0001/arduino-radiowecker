#include <utility>
#include <yxml.h>
#include "soundTouchXMLParser.hpp"
#include "statics.hpp"

namespace soundtouch
{
  using namespace alertclock;
  using namespace logger;

  const char *SoundTouchXMLParser::tag{ "xmlparser" };

  void SoundTouchXmlLoopParams::init()
  {
    isError = false;       //! is an erroer occured
    type = WS_UNKNOWN;     //! the current message type
    deviceID.clear();      //! what is the current device id
    depth = 0;             //! depth (level) in XML Objectl
    piVal.clear();         //! value of process instruction
    currElemName.clear();  //! name of the elem
    currAttrName.clear();  //! name of the attribute inner of element
    value.clear();         //! value of the attribute inner of the element
    piName.clear();        //! nam,e of the process instruction name
    if ( updatePtr )       //! id exist
      delete updatePtr;    //! delete it
    updatePtr = nullptr;   //! pointer to the current update object
    if ( zoneMember )      //! if exist
      delete zoneMember;   //! delete it
    zoneMember = nullptr;  //! if zoen update => pointzrer to zone members
  }

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
    if ( xmlList.empty() )
      return true;
    static SoundTouchXmlLoopParamsPtr params;
    //
    // if semaphore free, process, if not try it at neyt round
    if ( xSemaphoreTake( parseSem, pdMS_TO_TICKS( 10 ) ) == pdTRUE )
    {
      if ( params )
        params->init();
      else
        params = std::make_shared< SoundTouchXmlLoopParams >();
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
            params->isError = true;
            elog.log( ERROR, "%s: can't parse xml..", SoundTouchXMLParser::tag );
            break;
          case YXML_OK:  // Character consumed, no new token present
            break;
          case YXML_ELEMSTART:
            // element started his name is....
            params->currElemName = std::move( String( x->elem ) );
            params->currElemName.trim();
            computeElemStart( params );
            yield();
            ++( params->depth );
            break;
          case YXML_CONTENT:
            // xml element content add to value
            params->value += std::move( String( x->data ) );
            break;
          case YXML_ELEMEND:
            // element ends here
            --( params->depth );
            params->value.trim();
            computeElemEnd( params );
            params->currElemName.clear();
            params->value.clear();
            // element endet
            yield();
            break;
          case YXML_ATTRSTART:
            // init attribute string
            params->currAttrName.clear();
            break;
          case YXML_ATTRVAL:
            // append to attribute string
            params->value += std::move( String( x->data ) );
            break;
          case YXML_ATTREND:
            params->currAttrName = std::move( String( x->attr ) );
            params->value.trim();
            params->currAttrName.trim();
            //
            // Now we have a full attribute. Its name is in x->attr, and its value is
            // in the string 'attrVal'
            //
            computeAttrEnd( params );
            params->value.clear();
            break;
          case YXML_PISTART: /* Start of a processing instruction          */
            params->piName = std::move( String( x->pi ) );
            params->piVal.clear();
          case YXML_PICONTENT: /* Content of a PI                            */
            params->piVal += String( x->pi );
          case YXML_PIEND: /* End of a processing instruction            */
            elog.log( DEBUG, "%s: <level %d> pi \"%s\" = \"%s\"", SoundTouchXMLParser::tag, params->depth,
                      params->currAttrName.c_str(), params->value.c_str() );
          default:
            break;
        }
        if ( params->isError )
          break;

        /* Handle any tokens we are interested in */
        yield();
      }
      if ( !params->isError )
      {
        elog.log( DEBUG, "%s: parse xml message...DONE", SoundTouchXMLParser::tag );
      }
      else
      {
        elog.log( ERROR, "%s: parse xml message...UNDONE", SoundTouchXMLParser::tag );
      }
      // DO NOT FORGET!!!!!!
      free( x );
      if ( params->updatePtr )
      {
        if ( params->updatePtr->isValid )
        {
          elog.log( DEBUG, "%s: decoded message to queue!", SoundTouchXMLParser::tag );
          if ( params->updatePtr->deviceID.isEmpty() )
          {
            params->updatePtr->deviceID = params->deviceID;
          }
          SoundTouchUpdateTmplPtr savePtr( std::move( params->updatePtr ) );
          msgList.push_back( savePtr );
          params->updatePtr = nullptr;
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
    if ( params )
      return !( params->isError );
    return ( false );
  }

  /**
   * if there is an supported message, create an object
   */
  void SoundTouchXMLParser::createSoundTouchObject( SoundTouchXmlLoopParamsPtr p )
  {
    switch ( getSoundTouchUpdateType( p->currElemName ) )
    {
      case MSG_UPDATE_VOLUME:
        // make an object for volume
        if ( p->updatePtr )
          free( p->updatePtr );
        p->updatePtr = new SoundTouchVolumeUpdate();
        break;
      case MSG_UPDATE_NOW_PLAYING_CHANGED:
        // make an object for nowPlayingUpdate
        if ( p->updatePtr )
          free( p->updatePtr );
        p->updatePtr = new SoundTouchNowPlayingUpdate();
        break;
      case MSG_UPDATE_ZONE:
        // make an object for zone updates
        if ( p->updatePtr )
          free( p->updatePtr );
        p->updatePtr = new SoundTouchZoneUpdate();
        break;
      case MSG_UPDATE_RECENTS:
        // elog.log( DEBUG, "%s: ignore recents update!", SoundTouchXMLParser::tag );
      default:
        if ( p->updatePtr )
        {
          free( p->updatePtr );
          p->updatePtr = nullptr;
        }
        elog.log( INFO, "%s: <level 1> element update type <%s> not implemented (yet?)...", SoundTouchXMLParser::tag,
                  p->currElemName.c_str() );
        break;
    }
  }

  /**
   * workload for start of an element
   */
  void SoundTouchXMLParser::computeElemStart( SoundTouchXmlLoopParamsPtr p )
  {
    // elog.log( DEBUG, "%s: elemStart level: %d %s", SoundTouchXMLParser::tag, p->depth, p->currElemName.c_str() );
    // delay( 40 );
    switch ( p->depth )
    {
      case 0:
        // root level
        //
        // first/lowest/root level (rootlevel)
        //
        // elog.log( DEBUG, "%s: <root> element is <%s> level <%d>", SoundTouchXMLParser::tag, p->currAttrName.c_str(),
        //           p->depth );
        p->type = getSoundTouchMessageType( p->currElemName );
        // not interesting for me
        p->isError = ( p->type == WS_UNKNOWN );
        if ( p->isError )
        {
          elog.log( ERROR, "%s: <root> element <%s> level <%d> is unknown type, abort parsing!", SoundTouchXMLParser::tag,
                    p->currAttrName.c_str(), p->depth );
        }
        break;

      case 1:
        //
        // which Message type
        //
        switch ( p->type )
        {
          case WS_UPDATES:
            // create object etc
            // elog.log( INFO, "%s: UPDATE Message found!", SoundTouchXMLParser::tag );
            createSoundTouchObject( p );
            break;

          default:
            p->isError = true;
            if ( p->updatePtr )
              free( p->updatePtr );
            p->updatePtr = nullptr;
            elog.log( ERROR, "%s: <root> element <%s> level <%d> is unknown type, abort parsing!", SoundTouchXMLParser::tag,
                      p->currAttrName.c_str(), p->depth );
            break;
        }

      case 2:
        // elog.log( DEBUG, "%s: <level 2> element update property <%s>...", SoundTouchXMLParser::tag, p->currElemName.c_str() );
        if ( p->updatePtr )
        {
          switch ( p->updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_ZONE:
              // elog.log( DEBUG, "%s: <level 2> await attribute for zone master...", SoundTouchXMLParser::tag );
              //
              // equal if there is an content or not, its okay
              //
              p->updatePtr->isValid = true;
              // await attribute ipaddr
              break;
            default:
              break;
          }
        }
        break;

      case 3:
        // elog.log( DEBUG, "%s: <level 3> element property.sub <%s>...", SoundTouchXMLParser::tag, p->currElemName.c_str() );
        if ( p->updatePtr )
        {
          switch ( p->updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_VOLUME:
              // values at level 4 while elem end
              break;
            case MSG_UPDATE_NOW_PLAYING_CHANGED:
              // values getting from attribs
              break;
            case MSG_UPDATE_ZONE:
              // elog.log( DEBUG, "%s: <level 2> await attribute for member...", SoundTouchXMLParser::tag );
              if ( p->zoneMember )
                free( p->zoneMember );
              p->zoneMember = new SoundTouchZoneMember;
              p->updatePtr->isValid = true;
              break;
            default:
              if ( p->updatePtr )
              {
                free( p->updatePtr );
                p->updatePtr = nullptr;
              }
              // elog.log( INFO, "%s: <level 3> element property.sub <%s> not implemented (yet?)...", SoundTouchXMLParser::tag,
              //           p->currElemName.c_str() );
              break;
          }
        }
        break;
      case 4:
        // here are (at the moment) only contents
        break;

      default:
        //
        // not implemented
        //
        break;
    }
  }

  /**
   * workload for end of an element
   */
  void SoundTouchXMLParser::computeElemEnd( SoundTouchXmlLoopParamsPtr p )
  {
    // elog.log( DEBUG, "%s: elemEnd level: %d elem: <%s> value: <%s>", SoundTouchXMLParser::tag, p->depth, p->currElemName.c_str(),
    //           p->value.c_str() );
    // delay( 40 );
    switch ( p->depth )
    {
      case 0:
      case 1:
      case 2:
        // elog.log( DEBUG, "%s: elemEnd level: 0..2 ", SoundTouchXMLParser::tag );
        break;

      case 3:
        if ( p->updatePtr )
        {
          switch ( p->updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_VOLUME:
              getVolumeMessagePropertys( p->updatePtr, p->currElemName, p->value );
              break;
            case MSG_UPDATE_NOW_PLAYING_CHANGED:
              //
              // level 3 in now playing updates changed are the interesting items as content, sometimes as attrib
              //
              getNowPlayingMessagePropertys( p->updatePtr, p->currElemName, p->value );
              break;
            case MSG_UPDATE_ZONE:
              // zone member is an attribute
              //
              // member ID is cpontent of the tag fill in in message
              //
              if ( p->zoneMember )
              {
                p->zoneMember->id = p->value;
                static_cast< SoundTouchZoneUpdate * >( p->updatePtr )->members.push_back( *( std::move( p->zoneMember ) ) );
              }
              static_cast< SoundTouchZoneUpdate * >( p->updatePtr )->isValid = true;
              free( p->zoneMember );
              p->zoneMember = nullptr;
              // elog.log( DEBUG, "%s: zonemembers: %d", SoundTouchXMLParser::tag,
              //           static_cast< SoundTouchZoneUpdate * >( p->updatePtr )->members.size() );
              break;
            default:
              break;
          }
        }
      case 4:
        if ( p->updatePtr )
        {
          switch ( p->updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_NOW_PLAYING_CHANGED:
              //
              // level 4 in now playing updates is inner from <ContentItem>
              //
              // <itemName> and <containerArt> are availible
              break;
            default:
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
  void SoundTouchXMLParser::computeAttrEnd( SoundTouchXmlLoopParamsPtr p )
  {
    // elog.log( DEBUG, "%s: attrEnd level: %d, elem: <%s>, attr: <%s>, value: <%s>", SoundTouchXMLParser::tag, p->depth - 1,
    //           p->currElemName.c_str(), p->currAttrName.c_str(), p->value.c_str() );
    switch ( p->depth - 1 )
    {
      case 0:
        //
        // in root level only devive Id as attrib is availible
        //
        if ( p->currAttrName.equals( UPDATE_PROPERTY_ATTR_DEVICE_ID ) )
        {
          p->deviceID = p->value;
        }
        break;

      case 2:
        //
        // in level
        if ( p->updatePtr )
        {
          switch ( p->updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_VOLUME:
              // values at level 4 while elem end
              break;
            case MSG_UPDATE_NOW_PLAYING_CHANGED:
              // values getting from attribs
              if ( p->currAttrName.equals( UPDATE_PROPERTY_ATTR_SOURCE ) )
              {
                // elog.log( DEBUG, "%s: level %d attrib \"%s/%s\" = \"%s\"", SoundTouchXMLParser::tag, p->depth - 1,
                //           p->currElemName.c_str(), p->currAttrName.c_str(), p->value.c_str() );
                static_cast< SoundTouchNowPlayingUpdate * >( p->updatePtr )->source = p->value;
                if ( p->value.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATE_STANDBY ) )
                {
                  // elog.log( INFO, "%s: STANDBY!", SoundTouchXMLParser::tag );
                  static_cast< SoundTouchNowPlayingUpdate * >( p->updatePtr )->playStatus = STANDBY_STATE;
                }
                p->updatePtr->isValid = true;
              }
              break;
            case MSG_UPDATE_ZONE:
              if ( p->currAttrName.equals( UPDATE_PROPERTY_ZONE_ATTRIB_MASTER ) )
              {
                static_cast< SoundTouchZoneUpdate * >( p->updatePtr )->masterID = p->value;
                // elog.log( DEBUG, "%s: <level 2> attrib \"%s/%s\" = \"%s\"", SoundTouchXMLParser::tag, p->currElemName.c_str(),
                //           p->currAttrName.c_str(), p->value.c_str() );
              }
              break;

            default:
              break;
          }

          //
          // is there the awaited attrib for zone
          //
        }
        break;
      case 3:
        if ( p->updatePtr )
        {
          switch ( p->updatePtr->getUpdateType() )
          {
            case MSG_UPDATE_ZONE:
              // zone member is an attribute
              // elog.log( DEBUG, "%s: <level 3> attr  \"%s/%s\" = \"%s\"...", SoundTouchXMLParser::tag, p->currElemName.c_str(),
              //           p->currAttrName.c_str(), p->attrVal.c_str() );
              if ( p->zoneMember )
              {
                IPAddress addr;
                addr.fromString( p->value );
                p->zoneMember->ip = addr;
              }
              break;
            default:
              break;
          }
        }
        break;
      case 4:
        // elog.log( DEBUG, "%s: <level 4> attrib \"%s/%s\" = \"%s\"", SoundTouchXMLParser::tag, p->currElemName.c_str(),
        // p->currAttrName.c_str(),
        //           p->attrVal.c_str() );
        break;
    }
  }

  /**
   * set propertys for an nowPlayingUpdate message
   */
  bool SoundTouchXMLParser::getNowPlayingMessagePropertys( SoundTouchUpdateTmpl *ptr, String &currElemName, String &elemVal )
  {
    //
    // polymorph class, convert pointer
    //
    SoundTouchNowPlayingUpdate *dev = static_cast< SoundTouchNowPlayingUpdate * >( ptr );
    if ( currElemName.equals( UPDATE_PROPERTY_NPLAY_CONTENT ) )
    {
      // subentry contenItem found
      // not nessesary here
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_NPLAY_TRACK ) )
    {
      // which track is playing
      dev->track = elemVal;
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_NPLAY_ALBUM ) )
    {
      // which album is playing
      dev->album = elemVal;
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_NPLAY_STATIONNAME ) )
    {
      // which album is playing
      dev->stationName = elemVal;
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_NPLAY_ART ) )
    {
      // which album is playing
      dev->art = elemVal;
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_NPLAY_PLAYSTATUS ) )
    {
      // which album is playing
      dev->playStatus = getPlayingStateType( elemVal );
      // elog.log( ERROR, "%s: PLAYSTATE: %s (%d)", SoundTouchXMLParser::tag, elemVal.c_str(), dev->playStatus );
      dev->isValid = true;
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_NPLAY_STREAMTYPE ) )
    {
      // which album is playing
      dev->streamingType = getPlayingStreamingType( elemVal );
    }
    return dev->isValid;
    // <updates deviceID="689E19653E96">
    //   <nowPlayingUpdated>
    //     <nowPlaying deviceID="689E19653E96" source="TUNEIN" sourceAccount="">
    //       <ContentItem source="TUNEIN" type="stationurl" location="/v1/playback/station/s24950" sourceAccount=""
    //       isPresetable="true">
    //         <itemName>91.4 Berliner Rundfunk</itemName>
    //         <containerArt>http://cdn-profiles.tunein.com/s24950/images/logoq.jpg?t=160315</containerArt>
    //       </ContentItem>
    //       <track>Berliner Rundfunk</track>
    //       <artist>Berliner Rundfunk 91.4 - Die besten Hits aller Zeiten</artist>
    //       <album></album>
    //       <stationName>Berliner Rundfunk</stationName>
    //       <art artImageStatus="IMAGE_PRESENT">http://cdn-profiles.tunein.com/s24950/images/logog.jpg?t=637387494910000000</art>
    //       <favoriteEnabled />
    //       <playStatus>BUFFERING_STATE</playStatus>
    //       <streamType>RADIO_STREAMING</streamType>
    //     </nowPlaying>
    //   </nowPlayingUpdated>
    // </updates>
  }

  /**
   * set sub property in volume setting
   */
  bool SoundTouchXMLParser::getVolumeMessagePropertys( SoundTouchUpdateTmpl *ptr, String &currElemName, String &attrVal )
  {
    //
    // polymorph class, convert pointer
    //
    SoundTouchVolumeUpdate *dev = static_cast< SoundTouchVolumeUpdate * >( ptr );
    //
    if ( currElemName.equals( UPDATE_PROPERTY_VOL_TARGET ) )
    {
      dev->targetVol = static_cast< uint8_t >( attrVal.toInt() & 0xff );
      elog.log( DEBUG, "%s: volume target: %d", SoundTouchXMLParser::tag, dev->targetVol );
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_VOL_CURR ) )
    {
      dev->currVol = static_cast< uint8_t >( attrVal.toInt() & 0xff );
    }
    else if ( currElemName.equals( UPDATE_PROPERTY_VOL_MUTE ) )
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
  WsMsgUpdateType SoundTouchXMLParser::getSoundTouchUpdateType( String &_update )
  {
    if ( _update.equals( UPDATE_VOLUME ) )
      return MSG_UPDATE_VOLUME;
    else if ( _update.equals( UPDATE_NOW_PLAYING_UPDATED ) )
      return MSG_UPDATE_NOW_PLAYING_CHANGED;
    else if ( _update.equals( UPDATE_PRESETS ) )
      return MSG_UPDATE_PRESETS;
    else if ( _update.equals( UPDATE_ZONE ) )
      return MSG_UPDATE_ZONE;
    else if ( _update.equals( UPDATE_NOW_PLAYING_UPDATED ) )
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
   * get type of message, level 0
   */
  WsMsgType SoundTouchXMLParser::getSoundTouchMessageType( String &_type )
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
  WsPlayStatus SoundTouchXMLParser::getPlayingStateType( String &_state )
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
  WsStreamingTypes SoundTouchXMLParser::getPlayingStreamingType( String &_type )
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