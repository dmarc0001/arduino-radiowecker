#include "soundTouchAlert.hpp"
#include "common.hpp"
#include "statics.hpp"

namespace soundtouch
{
  using namespace logger;

  const char *SoundTouchAlert::tag{ "SoundTouchAlert" };

  /**
   * construct the object
   */
  SoundTouchAlert::SoundTouchAlert( alarmclock::DeviceEntry &_device, alarmclock::AlertEntry &_alert ) : isInit( false )
  {
    elog.log( DEBUG, "%s: create soundtouch alert instance", SoundTouchAlert::tag );
    sdDevice = std::make_shared< SoundTouchDevice >( _device );
    alertEntr = std::make_shared< alarmclock::AlertEntry >( _alert );
  }

  /**
   * if this object was destroing
   */
  SoundTouchAlert::~SoundTouchAlert()
  {
    elog.log( DEBUG, "%s: delete soundtouch alert instance", SoundTouchAlert::tag );
  }

  /**
   * init the object, return true if success
   */
  bool SoundTouchAlert::init()
  {
    return sdDevice->getDeviceInfos();
  }

  SoundTouchDeviceRunningMode SoundTouchAlert::getDeviceRunningMode()
  {
    //
    // set isInit if is initiated
    //
    SoundTouchDeviceRunningMode mode{ sdDevice->getDeviceRunningMode() };
    if ( !isInit && ( mode > ST_STATE_INIT_ALERT ) )
      isInit = true;
    return mode;
  }

  /**
   * play the alert
   */
  bool SoundTouchAlert::prepareAlertDevivce()
  {
    bool wasOkay{ true };
    elog.log( DEBUG, "%s: ask for play state...", SoundTouchAlert::tag );
    delay( 300 );
    WsPlayStatus playstate = sdDevice->getPlayState();
    if ( !( playstate == WsPlayStatus::STANDBY_STATE || playstate == WsPlayStatus::STOP_STATE ||
            playstate == WsPlayStatus::PAUSE_STATE ) )
    {
      elog.log( ERROR, "%s: device is playing another source, can't start alert, ABORT!", SoundTouchAlert::tag );
      sdDevice->setDeviceRunningMode( ST_STATE_ERROR );
      wasOkay = false;
    }
    if ( wasOkay && sdDevice->isZoneMemberOrMaster() )
    {
      elog.log( ERROR, "%s: device is in a zone, can't start alert, ABORT!", SoundTouchAlert::tag );
      sdDevice->setDeviceRunningMode( ST_STATE_ERROR );
      wasOkay = false;
    }
    sdDevice->setDeviceRunningMode( ST_STATE_WAIT_FOR_INIT_COMLETE );
    // save old volume after alaert
    oldVolume = sdDevice->getCurrentVolume();
    if ( wasOkay && sdDevice->setPower( true ) )
    {
      wasOkay = sdDevice->setCurrentVolume( 0 );
      if ( wasOkay )
      {
        // set source (here only preset)
        if ( alertEntr->source.startsWith( KEY_PRESET_COMMON ) )
        {
          //
          // yes presets are supported!
          //
          wasOkay = sdDevice->touchBtn( alertEntr->source );
          if ( wasOkay )
            sdDevice->setDeviceRunningMode( ST_STATE_RUNNING_ALERT );
        }
        else
        {
          wasOkay = false;
          elog.log( ERROR, "%s: sorry only presets supportet yet!", SoundTouchAlert::tag );
        }
      }
    }
    return wasOkay;
  }

  bool SoundTouchAlert::checkRunningAlert()
  {
    bool wasOkay{ true };
    static int64_t nextVoumeStep{ 0LL };
    //
    // check if the device is playing
    //
    if ( ( sdDevice->getPlayState() != BUFFERING_STATE ) && ( sdDevice->getPlayState() != PLAY_STATE ) )
    {
      //
      // device not running
      //
      wasOkay = false;
    }
    //
    // first check if the volume okay
    //
    if ( wasOkay )
    {
      uint8_t destVol = ( alertEntr->volume > static_cast< uint8_t >( 100U ) ) ? static_cast< uint8_t >( 100 ) : alertEntr->volume;
      uint8_t currVol = sdDevice->getCurrentVolume();
      if ( destVol > currVol )
      {
        // not too often, it's problematic
        if ( nextVoumeStep < esp_timer_get_time() )
        {
          if ( alertEntr->raiseVol )
          {
            // raise up volume one step
            wasOkay = sdDevice->touchBtn( KEY_VOL_UP );
          }
          else
          {
            // set volume
            wasOkay = sdDevice->setCurrentVolume( destVol );
          }
          nextVoumeStep = esp_timer_get_time() + getMicrosForMiliSec( 600L );
        }
      }
    }
    return wasOkay;
  }
}  // namespace soundtouch
