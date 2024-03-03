#include <time.h>
#include "alertTask.hpp"
#include "common.hpp"
#include "statics.hpp"
#include "statusObject.hpp"

namespace alarmclock
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
    int64_t setNextTimeCheckAlerts{ ( esp_timer_get_time() + getMicrosForSec( 12000 ) ) };
    int64_t nextMark =
        esp_timer_get_time() + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
    int64_t setNextTimeAlertsLoop = esp_timer_get_time() + getMicrosForSec( 250 );

    AlertTask::isRunning = true;
    while ( true )
    {
      //
      // check zyclic the current alerts
      //
      if ( setNextTimeAlertsLoop < esp_timer_get_time() )
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
            AlertTask::activeAlertList.erase( alert );
            break;
          }
        }
        setNextTimeAlertsLoop = esp_timer_get_time() + getMicrosForSec( 250 );
      }
      //
      // then i have some time for checking all alerts :-)
      //
      if ( setNextTimeCheckAlerts < esp_timer_get_time() )
      {
        time_t now;
        time( &now );
        tm *lt = localtime( &now );

        //
        // search through the alert List if alert have to compute
        //
        for ( auto alertIter = StatusObject::alertList.begin(); alertIter != StatusObject::alertList.end(); ++alertIter )
        {
          AlertEntryPtr alert = *alertIter;
          if ( !alert->enable )
            continue;
          //
          // check time
          // hour/min/sec == hour*60*60 + min*60 + sec
          // is alert in my area to make action?
          //
          int32_t secounds_today = static_cast< int32_t >( ( lt->tm_hour * 60 * 60 ) + ( lt->tm_min * 60 ) + lt->tm_sec );
          int32_t secounds_alert = static_cast< int32_t >( ( alert->alertHour * 60 * 60 ) + alert->alertMinute );
          int32_t secounds_diff = secounds_today - secounds_alert;
          if ( !( 10 < secounds_diff < 25 ) )
          {
            // No, time is not in may area
            // and reset "inUse" if ther is
            if ( alert->inUse )
              alert->inUse = false;
            continue;
          }
          //
          // time firts, check date
          //
          if ( alert->month != 255 && alert->day != 255 )
          {
            //
            // only at a special day in this year
            //
            if ( ( alert->month == lt->tm_mon ) && ( alert->day == lt->tm_mday ) )
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
                AlertTask::startAlert( alert );
                continue;
              }
            }
            else
            {
              for ( auto day : alert->days )
              {
                // is an day in the array like the current day?
                if ( static_cast< int >( day ) == lt->tm_wday )
                {
                  if ( !alert->inUse )
                  {
                    // start ALERT
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
        setNextTimeCheckAlerts = esp_timer_get_time() + getMicrosForSec( 12500 );
        yield();
      }
      //
      // DEBUGGING
      //
      if ( nextMark < esp_timer_get_time() )
      {
        //
        // say mark for check if the task is running
        //
        elog.log( DEBUG, "%s: ==== MARK ====", AlertTask::tag );
        nextMark =
            esp_timer_get_time() + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
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
        AlertTask::activeAlertList.push_back( stAlert );
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
          // first time, timeout activating
          timeoutTime = ( esp_timer_get_time() + getMicrosForSec( TIMEOUNT_WHILE_DEVICE_INIT ) );
          oldMode != ST_STATE_RUNNING_ALERT;
        }
        // test if checkRunning Alert is false and timeout is over
        if ( !_alert->checkRunningAlert() && ( timeoutTime < esp_timer_get_time() ) )
        {
          // timeout!
          elog.log( ERROR, "main: device running TIMEOUT or Alert aborted" );
          delay( 20 );  //! DEBUG: debug
          return false;
        }
        break;
      default:
        // _alert->checkRunningAlert();
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
};  // namespace alarmclock