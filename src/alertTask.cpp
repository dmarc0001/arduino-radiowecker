#include "alertTask.hpp"
#include "common.hpp"
#include "statics.hpp"
#include "statusObject.hpp"

namespace alarmclock
{
  using namespace logger;

  const char *AlertTask::tag{ "alertTask" };
  bool AlertTask::isRunning{ false };

  void AlertTask::alTask( void * )
  {
    AlertTask::isRunning = true;
    static bool isMsgSend{ false };
    while ( true )
    {
      sleep( 1 );
      //
      // DEBUGGING
      //
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
      xTaskCreate( AlertTask::alTask, "alert-task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, NULL );
    }
  }
};  // namespace alarmclock