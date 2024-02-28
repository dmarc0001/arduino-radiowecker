#include "soundTouchAlert.hpp"
#include "common.hpp"
#include "statics.hpp"

namespace soundtouch
{
  using namespace logger;

  const char *SoundTouchAlert::tag{ "SoundTouchAlert" };

  SoundTouchAlert::SoundTouchAlert( alarmclock::DeviceEntry &_device ) : isInit( false )
  {
    elog.log( DEBUG, "%s: create soundtouch alert instance", SoundTouchAlert::tag );
    sdDevice = std::make_shared< SoundTouchDevice >( _device );
  }

  SoundTouchAlert::~SoundTouchAlert()
  {
    elog.log( DEBUG, "%s: delete soundtouch alert instance", SoundTouchAlert::tag );
  }

  bool SoundTouchAlert::init( unsigned long timeout )
  {
    if ( sdDevice->getDeviceInfos() )
    {
      // int commands was okay;
      //
      // step 2 wait while init the device
      //
      auto endTimeout = millis() + timeout;
      while ( ST_STATE_INIT_ALERT != sdDevice->getDeviceRunningState() && endTimeout > millis() )
      {
        delay( 50 );
      }
      if ( ST_STATE_INIT_ALERT != sdDevice->getDeviceRunningState() )
      {
        elog.log( ERROR, "%s: timeout while init soundtouch device (get infos)...", SoundTouchAlert::tag );
      }
      else
      {
        // device is init
        isInit = true;
      }
    }
  }

}  // namespace soundtouch