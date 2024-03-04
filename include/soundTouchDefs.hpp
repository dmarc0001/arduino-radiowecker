#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>
#include "common.hpp"

namespace soundtouch
{
  constexpr const char *MSG_TYPE_SDKINFO{ "SoundTouchSdkInfo" };                      //! after connection via websocket appear sdkinfo
  constexpr const char *MSG_TYPE_USER_UPDATES{ "userActivityUpdate" };                //! updates from user first level
  constexpr const char *MSG_TYPE_UPDATES{ "updates" };                                //! updates first level
  constexpr const char *UPDATE_VOLUME{ "volumeUpdated" };                             //! volume updated
  constexpr const char *UPDATE_PRESETS{ "presetsUpdated" };                           //! presets updated
  constexpr const char *UPDATE_NOW_PLAYING{ "nowPlayingUpdated" };                    //! playing updated
  constexpr const char *UPDATE_RECENT{ "recentsUpdated" };                            //! update recent playings (ignore here)
  constexpr const char *UPDATE_BASS{ "bassUpdated" };                                 //! bass updated
  constexpr const char *UPDATE_ZONE{ "zoneUpdated" };                                 //! zone updated
  constexpr const char *UPDATE_ACC_MODE{ "acctModeUpdated" };                         // ac mode updated
  constexpr const char *UPDATE_SOURCES{ "sourcesUpdated" };                           //! sources updated
  constexpr const char *UPDATE_NOW_SELECT{ "nowSelectionUpdated" };                   // now selection updated
  constexpr const char *UPDATE_CONNECTION_STATE{ "connectionStateUpdated" };          //! connection state updated
  constexpr const char *UPDATE_INFO{ "infoUpdated" };                                 //! device info updated
  constexpr const char *UPDATE_SOFTWARE_STATE{ "swUpdateStatusUpdated" };             //! software status updated
  constexpr const char *UPDATE_SITE_SURVEY_RES{ "siteSurveyResultsUpdated" };         //! ?
  constexpr const char *UPDATE_PROPERTY_ATTR_DEVICE_ID{ "deviceID" };                 //! attrib device id
  constexpr const char *UPDATE_PROPERTY_ATTR_SOURCE{ "source" };                      //! attrib source (nowPlaying)
  constexpr const char *UPDATE_PROPERTY_VOL_TARGET{ "targetvolume" };                 //! target to volume device
  constexpr const char *UPDATE_PROPERTY_VOL_CURR{ "actualvolume" };                   //! current volume of device
  constexpr const char *UPDATE_PROPERTY_VOL_MUTE{ "muteenabled" };                    //! is device muted
  constexpr const char *UPDATE_PROPERTY_NPLAY_NOW_PLAYING{ "nowPlaying" };            //! subitem nowPlaying
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
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_STANDBY{ "STANDBY" };         //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_PLAY{ "PLAY_STATE" };         //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_PAUSE{ "PAUSE_STATE" };       //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_STOP{ "STOP_STATE" };         //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_BUFFER{ "BUFFERING_STATE" };  //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_PLAYSTATE_INVALIDPLAY{ "INVALID_PLAY_STATE" };  //! playstate
  constexpr const char *UPDATE_PROPERTY_NPLAY_STREAMTYPE_RADIO{ "RADIO_STREAMING" };          //! streamtype
  constexpr const char *UPDATE_PROPERTY_NPLAY_STREAMTYPE_ONDEMAND{ "TRACK_ONDEMAND" };        //! streamtype
  constexpr const char *UPDATE_PROPERTY_ZONE_ZONE{ "zone" };                                  //! zoneinfo level2 tag
  constexpr const char *UPDATE_PROPERTY_ZONE_ATTRIB_MASTER{ "master" };                       // attrib for zone -> master
  constexpr const char *KEY_MUTE{ "MUTE" };                                                   //! key for mute
  constexpr const char *KEY_POWER{ "POWER" };                                                 //! power button
  constexpr const char *KEY_VOL_UP{ "VOLUME_UP" };                                            //! volume key
  constexpr const char *KEY_VOL_DOWN{ "VOLUME_DOWN" };                                        //! volume key
  constexpr const char *KEY_PRESS{ "press" };                                                 //! key pressed
  constexpr const char *KEY_RELEASE{ "release" };                                             //! key released
  constexpr const char *KEY_PRESET_COMMON{ "PRESET_" };            //! for String.startsWith in alert.sources
  constexpr const char *KEY_PRESET_01{ "PRESET_1" };               //! preset of the soundtouch device
  constexpr const char *KEY_PRESET_02{ "PRESET_2" };               //! preset of the soundtouch device
  constexpr const char *KEY_PRESET_03{ "PRESET_3" };               //! preset of the soundtouch device
  constexpr const char *KEY_PRESET_04{ "PRESET_4" };               //! preset of the soundtouch device
  constexpr const char *KEY_PRESET_05{ "PRESET_5" };               //! preset of the soundtouch device
  constexpr const char *KEY_PRESET_06{ "PRESET_6" };               //! preset of the soundtouch device
  constexpr const char *WEB_GET_NOW_PLAYINGZONE{ "/nowPlaying" };  //! get nowPlaying
  constexpr const char *WEB_GET_ZONE{ "/getZone" };                //! get zone question
  constexpr const char *WEB_CMD_VOLUME{ "/volume" };               //! get/set device/zone volume
  constexpr const char *WEB_CMD_KEY{ "/key" };                     //! get/set device/zone mute
  constexpr int32_t TIMEOUNT_WHILE_DEVICE_INIT{ 10000 };           //! timeout in ms while soundtouch device is timeout

  /**
   * defines which kind of ws message we have
   */
  enum WsMsgType : uint8_t
  {
    WS_USER_ACTIVITY_UPDATES,  //! user has changes made
    WS_UPDATES,                //! updates was made
    WS_USER_SDK_INFO,          //! sdk info was send
    WS_UNKNOWN                 //! unknown type
  };

  /**
   * which state is the Alert with his device(s)
   */
  enum SoundTouchDeviceRunningMode : uint8_t
  {
    ST_STATE_UNINITIALIZED,
    ST_STATE_GET_INFOS,
    ST_STATE_INIT_ALERT,
    ST_STATE_WAIT_FOR_INIT_COMLETE,
    ST_STATE_RUNNING_ALERT,
    ST_STATE_END_ALERT,
    ST_STATE_ERROR,
    ST_STATE_UNKNOWN
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
    MAG_UPDATE_RECENT,               // recent list update (ignore)
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
    STANDBY_STATE,
    STOP_STATE,
    PAUSE_STATE,
    BUFFERING_STATE,
    PLAY_STATE,
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

  struct SoundTouchZoneMember
  {
    IPAddress ip;  //! ip of the zone member
    String id;     //! id (mac) of the zone member
  };

  /**
   * base class for polymorph classes to use teh same typed pointer for all subclasses
   */
  class SoundTouchUpdateTmpl
  {
    protected:
    WsMsgUpdateType msgType{ MSG_UNKNOWN };  //! user act or update

    public:
    bool isValid{ false };  //! have to set valid
    String deviceID;        //! received Device Id
    WsMsgUpdateType getUpdateType()
    {
      return msgType;
    };
  };

  /**
   * class for websocket msg volume updates
   */
  class SoundTouchVolumeUpdate : public SoundTouchUpdateTmpl
  {
    public:
    uint8_t targetVol{ 255 };  //! volume should set to
    uint8_t currVol{ 255 };    //! current volume
    bool mute{ false };        //! mute device
    SoundTouchVolumeUpdate()
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
    String source;
    // attribute : String artImageStatus;
    WsPlayStatus playStatus;
    String description;
    WsStreamingTypes streamingType;
    //
    SoundTouchNowPlayingUpdate()
    {
      // make the right type
      this->msgType = MSG_UPDATE_NOW_PLAYING_CHANGED;
      this->playStatus = STANDBY_STATE;
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

    // <updates deviceID="689E19653E96">
    //   <nowPlayingUpdated>
    //     <nowPlaying deviceID="689E19653E96" source="STANDBY">
    //       <ContentItem source="STANDBY" isPresetable="false" />
    //     </nowPlaying>
    //   </nowPlayingUpdated>
    // </updates>
  };

  /**
   * class for message with zone updates
   */
  class SoundTouchZoneUpdate : public SoundTouchUpdateTmpl
  {
    public:
    String masterID;                              //! if this == 0 no zone
    std::vector< SoundTouchZoneMember > members;  // if zize == 0 no zone
    SoundTouchZoneUpdate()
    {
      // make the right type
      this->msgType = MSG_UPDATE_ZONE;
    };
    // <updates deviceID="689E19653E96">
    //   <zoneUpdated>
    //     <zone master="689E19653E96">
    //       <member ipaddress="192.168.1.68">
    //         689E19653E96
    //       </member>
    //       <member ipaddress="192.168.1.20">
    //         F45EABFBCD9A
    //       </member>
    //     </zone>
    //   </zoneUpdated>
    // </updates>

    // <updates deviceID="689E19653E96">
    //   <zoneUpdated>
    //     <zone master="F45EABFBCD9A" senderIPAddress="192.168.1.20" senderIsMaster="true">
    //       <member ipaddress="192.168.1.68">
    //         689E19653E96
    //       </member>
    //     </zone>
    //   </zoneUpdated>
    // </updates>
  };

  class SoundTouchDeviceState
  {
    public:
    String deviceID;
    SoundTouchVolumeUpdate currVolume;
    WsPlayStatus currentPlayState;
    String masterID;  //! if MasterID => its zone Master
    std::vector< SoundTouchZoneMember > members;
    SoundTouchContentItem playItem;
    WsPlayStatus playStatus;
    struct
    {
      bool isVolume;
      bool isPlaying;
      bool isZone;
    } stateChecked;
    SoundTouchDeviceState()
    {
      stateChecked.isVolume = false;
      stateChecked.isPlaying = false;
      stateChecked.isZone = false;
      masterID.clear();
    }
  };

  //
  // base class for polymorph classes
  //
  using SoundTouchUpdateTmplPtr = std::shared_ptr< SoundTouchUpdateTmpl >;

  //
  // makes memory education easy
  //
  using SoundTouchVolumePtr = std::shared_ptr< SoundTouchVolumeUpdate >;
  using SoundTouchNowPlayingUpdatePtr = std::shared_ptr< SoundTouchNowPlayingUpdate >;
  using SoundTouchZoneUpdatePtr = std::shared_ptr< SoundTouchZoneUpdate >;

  //
  // easyer for semantic
  //
  using XmlMessageList = std::vector< String >;
  using DecodetMessageList = std::vector< SoundTouchUpdateTmplPtr >;

}  // namespace soundtouch

// clang-format off
//
// some useful things http://soundtouchdevice:8090/sources
//
// <sources deviceID="689E19653E96">
// <sourceItem source="AUX" sourceAccount="AUX" status="READY" isLocal="true" multiroomallowed="true">AUX IN</sourceItem>
// <sourceItem source="BLUETOOTH" status="UNAVAILABLE" isLocal="true" multiroomallowed="true"/>
// <sourceItem source="NOTIFICATION" status="UNAVAILABLE" isLocal="false" multiroomallowed="true"/>
// <sourceItem source="AMAZON" sourceAccount="useraccountname" status="READY" isLocal="false" multiroomallowed="true">useraccountname</sourceItem>
// <sourceItem source="QPLAY" sourceAccount="QPlay1UserName" status="UNAVAILABLE" isLocal="true" multiroomallowed="true">QPlay1UserName</sourceItem>
// <sourceItem source="QPLAY" sourceAccount="QPlay2UserName" status="UNAVAILABLE" isLocal="true" multiroomallowed="true">QPlay2UserName</sourceItem>
// <sourceItem source="UPNP" sourceAccount="UPnPUserName" status="UNAVAILABLE" isLocal="false" multiroomallowed="true">UPnPUserName</sourceItem>
// <sourceItem source="SPOTIFY" sourceAccount="SpotifyConnectUserName" status="UNAVAILABLE" isLocal="false" multiroomallowed="true">SpotifyConnectUserName</sourceItem>
// <sourceItem source="STORED_MUSIC_MEDIA_RENDERER" sourceAccount="StoredMusicUserName" status="UNAVAILABLE" isLocal="false" multiroomallowed="true">StoredMusicUserName</sourceItem>
// <sourceItem source="SPOTIFY" sourceAccount="SpotifyAlexaUserName" status="UNAVAILABLE" isLocal="false" multiroomallowed="true">SpotifyAlexaUserName</sourceItem>
// <sourceItem source="ALEXA" status="READY" isLocal="false" multiroomallowed="true"/>
// <sourceItem source="TUNEIN" status="READY" isLocal="false" multiroomallowed="true"/>
// <sourceItem source="LOCAL_INTERNET_RADIO" status="READY" isLocal="false" multiroomallowed="true"/>
// </sources>
//
// clang-format on