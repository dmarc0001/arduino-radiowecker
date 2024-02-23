#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>
#include "common.hpp"

namespace soundtouch
{
  constexpr const char *MSG_TYPE_USER_UPDATES{ "userActivityUpdate" };         //! updates from user first level
  constexpr const char *MSG_TYPE_UPDATES{ "updates" };                         //! updates first level
  constexpr const char *UPDATE_VOLUME{ "volumeUpdated" };                      //! volume updated
  constexpr const char *UPDATE_PRESETS{ "presetsUpdated" };                    //! presets updated
  constexpr const char *UPDATE_NOW_PLAYING{ "nowPlayingUpdated" };             //! playing updated
  constexpr const char *UPDATE_BASS{ "bassUpdated" };                          //! bass updated
  constexpr const char *UPDATE_ZONE{ "zoneUpdated" };                          //! zone updated
  constexpr const char *UPDATE_RECENT{ "recentsUpdated" };                     //! recent users updated
  constexpr const char *UPDATE_ACC_MODE{ "acctModeUpdated" };                  // ac mode updated
  constexpr const char *UPDATE_SOURCES{ "sourcesUpdated" };                    //! sources updated
  constexpr const char *UPDATE_NOW_SELECT{ "nowSelectionUpdated" };            // now selection updated
  constexpr const char *UPDATE_CONNECTION_STATE{ "connectionStateUpdated" };   //! connection state updated
  constexpr const char *UPDATE_INFO{ "infoUpdated" };                          //! device info updated
  constexpr const char *UPDATE_SOFTWARE_STATE{ "swUpdateStatusUpdated" };      //! software status updated
  constexpr const char *UPDATE_SITE_SURVEY_RES{ "siteSurveyResultsUpdated" };  //! ?
  constexpr const char *UPDATE_PROPERTY_TARGETVOL{ "targetvolume" };           //! target to volume device
  constexpr const char *UPDATE_PROPERTY_CURRVOL{ "actualvolume" };             //! current volume of device
  constexpr const char *UPDATE_PROPERTY_MUTEVOL{ "muteenabled" };              //! is device muted

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
    MSG_UPDATE_PRESETS,              // presetsUpdated
    MSG_UPDATE_NOW_PLAYING_CHANGED,  // nowPlayingUpdated
    MSG_UPDATE_BASS,                 // bassUpdated
    MSG_UPDATE_ZONE,                 // zoneUpdated
    MSG_UPDATE_RECENTS,              // recentsUpdated
    MSG_UPDATE_ACCMODE,              // acctModeUpdated
    MSG_UPDATE_SOURCES,              // sourcesUpdated
    MSG_UPDATE_NOW_SELECTION,        // nowSelectionUpdated
    MSG_UPDATE_CONNECTION_STATE,     // connectionStateUpdated
    MSG_UPDATE_INFOS,                // infoUpdated
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
   * substruct for play item
   */
  struct SoundTouchContentItem
  {
    String itemName;
    String source;
    String location;
    String sourceAccount;
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
    // String artImageStatus;
    WsPlayStatus playStatus;
    String description;
    String stationLocation;
    String nowPlaying;
    //
    SoundTouchNowPlayingUpdate()
    {
      // make the right type
      this->msgType = MSG_UPDATE_NOW_PLAYING_CHANGED;
    };
  };

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