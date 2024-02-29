#include "alertTask.hpp"
#include "common.hpp"
#include "statics.hpp"
#include "statusObject.hpp"

namespace alarmclock
{
  using namespace logger;

  int64_t nextMark = esp_timer_get_time() + getMicrosForMiliSec( 21007L );
  const char *AlertTask::tag{ "alerttask" };
  bool AlertTask::isRunning{ false };

  void AlertTask::alTask( void * )
  {
    AlertTask::isRunning = true;
    static bool isMsgSend{ false };
    while ( true )
    {
      yield();
      //
      // DEBUGGING
      //
      if ( nextMark < esp_timer_get_time() )
      {
        elog.log( DEBUG, "%s: ==== MARK ==== alTask", AlertTask::tag );
        nextMark = esp_timer_get_time() + getMicrosForMiliSec( 21003L );
      }
    }
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
      xTaskCreate( AlertTask::alTask, "alert-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY +1 , NULL );
    }
  }
};  // namespace alarmclock