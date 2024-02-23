#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>
#include "common.hpp"

namespace soundtouch
{
  constexpr const char *MSG_TYPE_USER_UPDATES{ "userActivityUpdate" };                //! updates from user first level
  constexpr const char *MSG_TYPE_UPDATES{ "updates" };                                //! updates first level
  constexpr const char *UPDATE_VOLUME{ "volumeUpdated" };                             //! volume updated
  constexpr const char *UPDATE_PRESETS{ "presetsUpdated" };                           //! presets updated
  constexpr const char *UPDATE_NOW_PLAYING{ "nowPlayingUpdated" };                    //! playing updated
  constexpr const char *UPDATE_BASS{ "bassUpdated" };                                 //! bass updated
  constexpr const char *UPDATE_ZONE{ "zoneUpdated" };                                 //! zone updated
  constexpr const char *UPDATE_RECENT{ "recentsUpdated" };                            //! recent users updated
  constexpr const char *UPDATE_ACC_MODE{ "acctModeUpdated" };                         // ac mode updated
  constexpr const char *UPDATE_SOURCES{ "sourcesUpdated" };                           //! sources updated
  constexpr const char *UPDATE_NOW_SELECT{ "nowSelectionUpdated" };                   // now selection updated
  constexpr const char *UPDATE_CONNECTION_STATE{ "connectionStateUpdated" };          //! connection state updated
  constexpr const char *UPDATE_INFO{ "infoUpdated" };                                 //! device info updated
  constexpr const char *UPDATE_SOFTWARE_STATE{ "swUpdateStatusUpdated" };             //! software status updated
  constexpr const char *UPDATE_SITE_SURVEY_RES{ "siteSurveyResultsUpdated" };         //! ?
  constexpr const char *UPDATE_PROPERTY_VOL_TARGET{ "targetvolume" };                 //! target to volume device
  constexpr const char *UPDATE_PROPERTY_VOL_CURR{ "actualvolume" };                   //! current volume of device
  constexpr const char *UPDATE_PROPERTY_VOL_MUTE{ "muteenabled" };                    //! is device muted
  constexpr const char *UPDATE_PROPERTY_NPLAY_CONTENT{ "contenItem" };                //! subitem contentItem
  constexpr const char *UPDATE_PROPERTY_NPLAY_TRACK{ "track" };                       //! which track is playing
  constexpr const char *UPDATE_PROPERTY_NPLAY_ARTIST{ "artist" };                     //! which artist is playing
  constexpr const char *UPDATE_PROPERTY_NPLAY_ALBUM{ "album" };                       //! which album is playing
  constexpr const char *UPDATE_PROPERTY_NPLAY_ART{ "art" };                           //! an icon for station/artist
  constexpr const char *UPDATE_PROPERTY_NPLAY_IMAGE_ART{ "artImageStatus" };          //! infos where the image for station/artis is
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATUS{ "playStatus" };             //! which playstatus (play, pause,buffering off)
  constexpr const char *UPDATE_PROPERTY_NPLAY_DESCRIPTION{ "description" };           //! descroiption for playing
  constexpr const char *UPDATE_PROPERTY_NPLAY_STATIONLOC{ "stationLocation" };        //! where is the station
  constexpr const char *UPDATE_PROPERTY_NPLAY_STATIONNAME{ "stationName" };           //! what is the name of the station
  constexpr const char *UPDATE_PROPERTY_NPLAY_STREAMTYPE{ "streamType" };             //! which type of stream
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_PLAY{ "PLAY_STATE" };         //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_PAUSE{ "PAUSE_STATE" };       //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_STOP{ "STOP_STATE" };         //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_BUFFER{ "BUFFERING_STATE" };  //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_INVALIDPLAY{ "INVALID_PLAY_STATE" };  //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_STREAMTYPE_RADIO{ "RADIO_STREAMING" };          //! streamtype
  constexpr const char *UPDATE_PROPERTY_NPLAY_STREAMTYPE_ONDEMAND{ "TRACK_ONDEMAND" };        //! streamtype

  /**
   * defines which kind of ws message we have
   */
  enum WsMsgType : uint8_t
  {
    WS_USER_ACTIVITY_UPDATES,  //! user has changes made
    WS_UPDATES,                //! updates was made
    WS_UNKNOWN                 //! unknown type
  };

  /**
   * which update (WS_UPDATES)
   */
  enum WsMsgUpdateType : uint8_t
  {
    MSG_UPDATE_VOLUME,               // volumeUpdated
    MSG_UPDATE_NOW_PLAYING_CHANGED,  // nowPlayingUpdated
    MSG_UPDATE_ZONE,                 // zoneUpdated
    MSG_UPDATE_INFOS,                // infoUpdated
    MSG_UPDATE_CONNECTION_STATE,     // connectionStateUpdated
    MSG_UPDATE_PRESETS,              // presetsUpdated
    MSG_UPDATE_BASS,                 // bassUpdated
    MSG_UPDATE_RECENTS,              // recentsUpdated
    MSG_UPDATE_ACCMODE,              // acctModeUpdated
    MSG_UPDATE_SOURCES,              // sourcesUpdated
    MSG_UPDATE_NOW_SELECTION,        // nowSelectionUpdated
    MSG_UPDATE_SOFTWARE,             // swUpdateStatusUpdated
    MSG_UPDATE_SITE_THINGS,          // siteSurveyResultsUpdated
    MSG_UNKNOWN
  };

  /**
   * playstati for soundtouch device
   */
  enum WsPlayStatus : uint8_t
  {
    PLAY_STATE,
    PAUSE_STATE,
    STOP_STATE,
    BUFFERING_STATE,
    INVALID_PLAY_STATUS
  };

  /**
   * some streaming types in nowPlaying
   */
  enum WsStreamingTypes : uint8_t
  {
    STREAM_RADIO_STREAMING,
    STREAM_TRACK_ONDEMAND,
    STREAM_UNKNOWN
  };

  /**
   * substruct for play item
   */
  struct SoundTouchContentItem
  {
    String itemName;
    String source;
    String type;
    String location;
    String sourceAccount;
    String containerArt;
    bool isPresetable;
  };

  /**
   * base class for polymorph classes to use teh same typed poiuntzrer for all subclasses
   */
  class SoundTouchUpdateTmpl
  {
    protected:
    WsMsgUpdateType msgType{ MSG_UNKNOWN };  //! user act or update

    public:
    bool isValid{ false };  //! have to set valid
    WsMsgUpdateType getUpdateType()
    {
      return msgType;
    };
  };

  /**
   * class for websocket msg volume updates
   */
  class SoundTouchVolume : public SoundTouchUpdateTmpl
  {
    public:
    uint8_t targetVol{ 255 };  //! volume should set to
    uint8_t currVol{ 255 };    //! current volume
    bool mute{ false };        //! mute device
    SoundTouchVolume()
    {
      // make the right type
      this->msgType = MSG_UPDATE_VOLUME;
    };
  };

  /**
   * class for websocket msg now playing updates
   */
  class SoundTouchNowPlayingUpdate : public SoundTouchUpdateTmpl
  {
    public:
    SoundTouchContentItem contenItem;
    String track;
    String artist;
    String album;
    String stationName;
    String art;
    // attribute : String artImageStatus;
    WsPlayStatus playStatus;
    String description;
    WsStreamingTypes streamingType;
    //
    SoundTouchNowPlayingUpdate()
    {
      // make the right type
      this->msgType = MSG_UPDATE_NOW_PLAYING_CHANGED;
    };
  };
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

  //
  // base class for polymorph classes
  //
  using SoundTouchUpdateTmplPtr = std::shared_ptr< SoundTouchUpdateTmpl >;

  //
  // makes memory education easy
  //
  using SoundTouchVolumePtr = std::shared_ptr< SoundTouchVolume >;
  using SoundTouchNowPlayingUpdatePtr = std::shared_ptr< SoundTouchNowPlayingUpdate >;

  //
  // easyer for semantic
  //
  using XmlMessageList = std::vector< String >;
  using DecodetMessageList = std::vector< SoundTouchUpdateTmplPtr >;

}  // namespace soundtouch