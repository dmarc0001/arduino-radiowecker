#pragma once
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "soundTouchDefs.hpp"

namespace alarmclock
{
  // previous Declaration
  class AlertEntry;
  class DeviceEntry;

  enum WlanState : uint8_t
  {
    DISCONNECTED,
    SEARCHING,
    CONNECTED,
    TIMESYNCED,
    FAILED
  };

  enum SoundPreset : uint8_t
  {
    PRESET_1,
    PRESET_2,
    PRESET_3,
    PRESET_4,
    PRESET_5,
    PRESET_6,
    PRESET_UNKNOWN
  };

  /**
   * enum for weekdaysm starts at monday
   */
  enum AlertDays : uint8_t
  {
    mo,
    tu,
    we,
    th,
    fr,
    sa,
    su
  };

  using AlRecordList = std::vector< AlertEntry >;
  using DevList = std::vector< DeviceEntry >;
  using AlertDayList = std::vector< AlertDays >;
  using AlertDeviceIdList = std::vector< String >;

  /**
   * one devie (sound Touch)
   */
  class DeviceEntry
  {
    public:
    String name;       //! devic name
    IPAddress addr;    //! ip addr of the device
    String id;         //! id of device
    uint16_t webPort;  //! port for api
    uint16_t wsPort;   //! port for api
    String type;       //! device type
    String note;       //! devie note from user
  };

  /**
   * on alert entry
   */
  class AlertEntry
  {
    public:
    String name;                //! name of the alert
    uint8_t volume;             //! volume to weak up
    String location;            //! have to read in manual api
    String source;              //! preset or string to source
    bool raiseVol;              //! should volume raisng on? down
    uint16_t duration;          //! length in secounds
    String sourceAccount;       //! if an stresaming account needet
    AlertDayList days;          //! if present, days to alert
    AlertDeviceIdList devices;  //! which devices?
    String type;                //! read manual
    bool enable;                //! alert enable?
    String note;                //! user note (cause etc)
    uint8_t alertHour;          //! which hour wake up
    uint8_t alertMinute;        //! which minute wake up
    uint8_t day;                //! if on a day, day number (1-31)
    uint8_t month;              //! if on day, month number (1-12)
  };

}  // namespace alarmclock
