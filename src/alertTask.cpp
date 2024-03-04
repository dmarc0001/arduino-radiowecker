#include <time.h>
#include "appStructs.hpp"
#include "alertTask.hpp"
#include "common.hpp"
#include "statics.hpp"
#include "statusObject.hpp"

namespace alertclock
{
  using namespace logger;

  const char *AlertTask::tag{ "alerttask" };
  bool AlertTask::isRunning{ false };
  soundtouch::SoundTouchAlertPtrList AlertTask::activeAlertList;

  /**
   * watch for time to alert
   * sends alert a minute before start, to make some preparations on the device
   */
  void AlertTask::alTask( void * )
  {
    static int64_t nextMark =
        esp_timer_get_time() + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
    static int64_t setNextTimeAlertsLoop{ 200LL };
    static int64_t setNextTimeCheckAlerts{ 0LL };
    static uint8_t rounds;
    alertclock::WlanState oldWlanState = StatusObject::getWlanState();

    AlertTask::isRunning = true;
    while ( true )
    {
      int64_t currentTimeStamp = esp_timer_get_time();
      //
      // check sometimes the timsync state
      //
      if ( rounds < 60 )
      {
        ++rounds;
      }
      else
      {
        rounds = 0;
        if ( StatusObject::getWlanState() != oldWlanState )
        {
          oldWlanState = StatusObject::getWlanState();
          if ( oldWlanState == alertclock::WlanState::TIMESYNCED )
          {
            int64_t setNextTimeAlertsLoop = currentTimeStamp + getMicrosForMiliSec( 250 );
            int64_t setNextTimeCheckAlerts = currentTimeStamp + getMicrosForMiliSec( 1200 );
          }
        }
      }
      if ( oldWlanState != alertclock::WlanState::TIMESYNCED )
      {
        //
        // no timesync, no aleerts
        //
        yield();
        delay( 100 );
        continue;
      }
      //
      // check zyclic the current alerts
      //
      if ( setNextTimeAlertsLoop < currentTimeStamp )
      {
        //
        // loop over all alerts
        // maybe more than one, also on a esp32
        //
        for ( auto alert = AlertTask::activeAlertList.begin(); alert != AlertTask::activeAlertList.end(); ++alert )
        {
          if ( !AlertTask::alertLoop( *alert ) )
          {
            //
            // if the return false, remove the alert smartpointer from the list
            // std::shared_ptr makes deleting the object, so i habe no problems
            // with garbage in memory
            // the marker "inUse" in alert entry wil reset below
            // if the destructor forgot this
            //

            elog.log( INFO, "%s: remove alert <%s> from active list!", AlertTask::tag, ( *alert )->getAlertName().c_str() );
            AlertTask::activeAlertList.erase( alert );
            break;
          }
        }
        setNextTimeAlertsLoop = currentTimeStamp + getMicrosForMiliSec( 250 );
      }
      //
      // then i have some time for checking all alerts :-)
      //
      if ( setNextTimeCheckAlerts < currentTimeStamp )
      {
        tm localTime{ 0 };
        if ( !getLocalTime( &localTime ) )
        {
          elog.log( CRITICAL, "%s: failed to obtain system time!", AlertTask::tag );
        }
        // else
        // {
        //   elog.log( INFO, "%s: local Time is %02d:%02d:%02d", AlertTask::tag, localTime.tm_hour, localTime.tm_min, localTime.tm_sec
        //   );
        // }
        //
        // search through the alert List if alert have to compute
        //
        for ( auto alertIter = StatusObject::alertList.begin(); alertIter != StatusObject::alertList.end(); ++alertIter )
        {
          AlertEntryPtr alert = *alertIter;
          if ( !alert->enable )
          {
            continue;
          }
          //
          // check time
          // hour/min/sec == hour*60*60 + min*60 + sec
          // is alert in my area to make action?
          //
          int secounds_today = ( localTime.tm_hour * 60 * 60 ) + ( localTime.tm_min * 60 ) + localTime.tm_sec;
          int secounds_alert = ( alert->alertHour * 60 * 60 ) + ( alert->alertMinute * 60 );
          int secounds_diff = secounds_alert - secounds_today;
          if ( ( secounds_diff > -10 ) && ( secounds_diff < 60 ) )
          {
            // TODO: only debug
            elog.log( DEBUG, "%s: time to alert <%s>: %02d sec", AlertTask::tag, alert->name.c_str(), secounds_diff );
          }
          if ( ( secounds_diff >= 0 ) && ( secounds_diff < 30 ) )
          {
            // No, time is not in may area
            // and reset "inUse" if ther is
            elog.log( DEBUG, "%s: alert is in area for compute...", AlertTask::tag );
            if ( alert->inUse )
            {
              elog.log( DEBUG, "%s: alert is in area for compute but running...", AlertTask::tag );
              continue;
            }
            elog.log( DEBUG, "%s: alert is in area for compute...", AlertTask::tag );
          }
          else
            continue;
          elog.log( DEBUG, "%s: alert computing...", AlertTask::tag );
          //
          // time firts, check date
          //
          if ( alert->month != 255 && alert->day != 255 )
          {
            elog.log( INFO, "%s: day and month given! (%02d.%02s)", AlertTask::tag, alert->month, alert->day );
            //
            // only at a special day in this year
            //
            if ( ( alert->month == localTime.tm_mon ) && ( alert->day == localTime.tm_mday ) )
            {
              if ( !alert->inUse )
              {
                // start ALERT
                AlertTask::startAlert( alert );
                continue;
              }
            }
          }
          else
          {
            //
            // at an weekday or all weekdays
            //
            if ( alert->days.empty() )
            {
              if ( !alert->inUse )
              {
                // start ALERT
                elog.log( INFO, "%s: alert all days!", AlertTask::tag );
                AlertTask::startAlert( alert );
                continue;
              }
            }
            else
            {
              for ( auto day : alert->days )
              {
                // is an day in the array like the current day?
                if ( static_cast< int >( day ) == localTime.tm_wday )
                {
                  if ( !alert->inUse )
                  {
                    // start ALERT
                    elog.log( INFO, "%s: alert at this weekday!", AlertTask::tag );
                    AlertTask::startAlert( alert );
                    continue;
                  }
                }
              }
            }
          }
        }
        //
        // next check
        //
        setNextTimeCheckAlerts = currentTimeStamp + getMicrosForMiliSec( 12500 );
      }
      yield();
      //
      // DEBUG: mark
      //
      if ( nextMark < currentTimeStamp )
      {
        //
        // say mark for check if the task is running
        //
        elog.log( DEBUG, "%s: ==== MARK ====", AlertTask::tag );
        nextMark =
            currentTimeStamp + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
      }
    }
    AlertTask::isRunning = false;
  }

  bool AlertTask::startAlert( AlertEntryPtr _alertEntr )
  {
    String deviceId = _alertEntr->devices.front();
    for ( auto device : StatusObject::devList )
    {
      if ( device->id.equals( deviceId ) )
      {
        //
        // device is present, create an soundtouchAlert
        //
        soundtouch::SoundTouchDevicePtr stDevice = std::make_shared< soundtouch::SoundTouchDevice >( device );
        soundtouch::SoundTouchAlertPtr stAlert = std::make_shared< soundtouch::SoundTouchAlert >( stDevice, _alertEntr );
        //
        // make alert as "inUse" will make the stAlert object in constructor
        // put the alert into the List of active alerts
        //
        if ( stAlert->init() )
        {
          AlertTask::activeAlertList.push_back( stAlert );
        }
        else
        {
          elog.log( CRITICAL, "%s: alert cant init!", AlertTask::tag );
        }
      }
    }
    return true;
  }

  bool AlertTask::alertLoop( soundtouch::SoundTouchAlertPtr _alert )
  {
    using namespace soundtouch;
    using namespace logger;
    //
    static int64_t timeoutTime{ ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) ) };
    static SoundTouchDeviceRunningMode oldMode{ ST_STATE_UNKNOWN };
    //
    if ( !_alert )
      return false;
    switch ( _alert->getDeviceRunningMode() )
    {
      case ST_STATE_UNINITIALIZED:
        if ( oldMode != ST_STATE_UNINITIALIZED )
        {
          // first time, timeout activating
          timeoutTime = ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) );
          oldMode != ST_STATE_UNINITIALIZED;
        }
        break;
      case ST_STATE_GET_INFOS:
        if ( oldMode != ST_STATE_GET_INFOS )
        {
          // first time, timeout activating
          timeoutTime = ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) );
          oldMode != ST_STATE_GET_INFOS;
        }
        // wait if timeout or next level
        if ( timeoutTime < esp_timer_get_time() )
        {
          // timeout!
          elog.log( ERROR, "main: device init TIMEOUT, Abort alert!" );
          return false;
        }
        break;
      case ST_STATE_INIT_ALERT:
        oldMode = ST_STATE_INIT_ALERT;
        _alert->prepareAlertDevivce();
        timeoutTime = esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT );
        break;
      case ST_STATE_WAIT_FOR_INIT_COMLETE:
        oldMode = ST_STATE_WAIT_FOR_INIT_COMLETE;
        // timeout
        if ( timeoutTime < esp_timer_get_time() )
        {
          // timeout!
          elog.log( ERROR, "%s: device prepare TIMEOUT, Abort alert!", AlertTask::tag );
          return false;
        }
        break;
      case ST_STATE_RUNNING_ALERT:
        if ( oldMode != ST_STATE_RUNNING_ALERT )
        {
          elog.log( DEBUG, "%s: device running STATE RUNNING ALERT NOW", AlertTask::tag );
          // first time, timeout activating
          timeoutTime = ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) );
          oldMode = ST_STATE_RUNNING_ALERT;
        }
        // test if checkRunning Alert is false and timeout is over
        if ( !_alert->checkRunningAlert() )
        {
          // okay, device is not running
          // is inner of the timeout time
          if ( timeoutTime > esp_timer_get_time() )
          {
            //
            // no timeout time is outdated
            //
            return true;
          }
          elog.log( INFO, "%s: device not running more, alert ends", AlertTask::tag );
          return false;
        }
        if ( _alert->getPlayState() == WsPlayStatus::PLAY_STATE )
        {
          //
          // timeout ends
          //
          timeoutTime = esp_timer_get_time();
        }
        break;
      default:
        return _alert->checkRunningAlert();
        break;
    }
    return true;
  }

  /**
   * start (if not running yet) the data writer task
   */
  void AlertTask::start()
  {
    elog.log( INFO, "%s: Task start...", AlertTask::tag );

    if ( AlertTask::isRunning )
    {
      elog.log( ERROR, "%s: Task is already running, abort.", AlertTask::tag );
    }
    else
    {
      xTaskCreate( AlertTask::alTask, "alert-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY + 1, NULL );
    }
  }
};  // namespace alertclock