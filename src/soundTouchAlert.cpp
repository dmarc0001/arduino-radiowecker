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
  SoundTouchAlert::SoundTouchAlert( SoundTouchDevicePtr _device, alertclock::AlertEntryPtr _alertEntr )
      : isInit( false ), sdDevice( _device ), alertEntr( _alertEntr )
  {
    elog.log( DEBUG, "%s: create soundtouch alert instance", SoundTouchAlert::tag );
    alertEntr->inUse = true;
  }

  /**
   * if this object was destroing
   */
  SoundTouchAlert::~SoundTouchAlert()
  {
    alertEntr->inUse = false;
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
    if ( wasOkay && sdDevice->getDeviceRunningMode() == ST_STATE_ERROR )
    {
      wasOkay = false;
    }
    if ( wasOkay && sdDevice->isZoneMemberOrMaster() )
    {
      elog.log( ERROR, "%s: device is in a zone, can't start alert, ABORT!", SoundTouchAlert::tag );
      sdDevice->setDeviceRunningMode( ST_STATE_ERROR );
      wasOkay = false;
    }
    if ( wasOkay )
    {
      sdDevice->setDeviceRunningMode( ST_STATE_WAIT_FOR_INIT_COMLETE );
      // save old volume after alaert
      oldVolume = sdDevice->getCurrentVolume();
      wasOkay = sdDevice->setCurrentVolume( 0 );
    }
    //
    // power on on implicit when press preset key
    //
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
        wasOkay = sdDevice->setCurrentVolume( 0 );
      }
      else
      {
        wasOkay = false;
        elog.log( ERROR, "%s: sorry only presets supportet yet!", SoundTouchAlert::tag );
      }
    }
    return wasOkay;
  }

  /**
   * have to call in ca 200ms steps
   * control the different ststes of the device and alert
   */
  bool SoundTouchAlert::checkRunningAlert()
  {
    WsPlayStatus currPlayState = sdDevice->getPlayState();
    //
    // check if the device is playing
    //
    if ( ( ( currPlayState == BUFFERING_STATE ) || ( currPlayState == PLAY_STATE ) ) &&
         ( sdDevice->getDeviceRunningMode() != ST_STATE_ERROR ) )
    {
      //
      // device running
      //
      runningAlertWasOkay = true;
    }
    //
    // if the alert time was come?
    //
    if ( runningAlertWasOkay && !alertIsStart )
    {
      //
      // if not is start yet, check the time difference
      //
      tm localTime{ 0 };
      if ( !getLocalTime( &localTime ) )
      {
        elog.log( CRITICAL, "%s: failed to obtain system time!", SoundTouchAlert::tag );
        return false;
      }
      int secounds_today = ( localTime.tm_hour * 3600 ) + ( localTime.tm_min * 60 ) + localTime.tm_sec;
      int secounds_alert = ( alertEntr->alertHour * 3600 ) + ( alertEntr->alertMinute * 60 );
      int secounds_diff = secounds_alert - secounds_today;
      if ( secounds_diff <= 0 )
      {
        // yes its time to tune up the volume
        alertIsStart = true;
        // and set the absolute time for alert end
        elog.log( DEBUG, "%s: alert <%d>duration is <%d> secounds!", SoundTouchAlert::tag, alertEntr->name.c_str(),
                  alertEntr->duration );
        runningAlertVolEndTime = esp_timer_get_time() + getMicrosForSec( static_cast< int32_t >( alertEntr->duration ) );
      }
      else
      {
        // not tune up the volume yet
        uint8_t currVol = sdDevice->getCurrentVolume();
        if ( currVol > 0 )
          sdDevice->setCurrentVolume( 0 );
        return true;
      }
    }
    //
    // first check if the volume okay
    // and if device is playing
    //
    if ( runningAlertWasOkay && ( sdDevice->getPlayState() == PLAY_STATE ) )
    {
      uint8_t destVol = ( alertEntr->volume > static_cast< uint8_t >( 100U ) ) ? static_cast< uint8_t >( 100 ) : alertEntr->volume;
      uint8_t currVol = sdDevice->getCurrentVolume();
      if ( runningAlertVolEndTime > esp_timer_get_time() )
      {
        // if the end of the alert is not given
        if ( destVol > currVol )
        {
          if ( alertEntr->raiseVol )
          {
            // not too often, it's problematic
            if ( runningAlertNextVolStep < esp_timer_get_time() )
            {
              //
              // if the user has via remote make volume down
              //
              if ( currVol < runningAlertNextDestVol - 4 )
              {
                // then lets the volume so
                alertEntr->raiseVol = false;
              }
              else
              {
                // normal volume up to the destination
                runningAlertNextDestVol = ++currVol;
                // raise up volume one step
                runningAlertWasOkay = sdDevice->touchBtn( KEY_VOL_UP );
                // raise up volume one step
                // runningAlertWasOkay = sdDevice->touchBtn( KEY_VOL_UP );
              }
              runningAlertNextVolStep = esp_timer_get_time() + getMicrosForMiliSec( 800L );
            }
          }
          else
          {
            // set volume
            runningAlertWasOkay = sdDevice->setCurrentVolume( destVol );
          }
        }
      }
    }

    //
    // now show, if the alert have to end
    //
    if ( runningAlertWasOkay && ( runningAlertVolEndTime < esp_timer_get_time() ) )
    {
      // not too often...
      if ( runningAlertNextVolStep < esp_timer_get_time() )
      {
        uint8_t currVol = sdDevice->getCurrentVolume();
        // tune volume down
        if ( alertEntr->raiseVol )
        {
          // if volume 0 then end the alert
          if ( currVol == 0 )
          {
            //
            // first power off, its right!
            //
            if ( ( currPlayState == BUFFERING_STATE ) || ( currPlayState == PLAY_STATE ) )
              sdDevice->touchBtn( KEY_POWER );
            // then the old volume from "before"
            runningAlertWasOkay = sdDevice->setCurrentVolume( oldVolume );
            return false;
          }
          // else make volume lower
          runningAlertWasOkay = sdDevice->touchBtn( KEY_VOL_DOWN );
          runningAlertWasOkay = sdDevice->touchBtn( KEY_VOL_DOWN );
        }
        else
        {
          //
          // first power off, its right!
          //
          if ( ( currPlayState == BUFFERING_STATE ) || ( currPlayState == PLAY_STATE ) )
            sdDevice->touchBtn( KEY_POWER );
          // then the old volume from "before"
          runningAlertWasOkay = sdDevice->setCurrentVolume( oldVolume );
          return false;
        }
        runningAlertNextVolStep = esp_timer_get_time() + getMicrosForMiliSec( 800L );
      }
    }
    return runningAlertWasOkay;
  }
}  // namespace soundtouch
