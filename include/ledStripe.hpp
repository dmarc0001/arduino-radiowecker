#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <FastLED.h>
#include "appPreferences.hpp"

namespace alertclock
{

  class LEDStripe
  {
    private:
    static const char *tag;
    static CRGB leds[ appprefs::LED_STRIPE_COUNT ];         //! save current color for every led
    static CRGB shadow_leds[ appprefs::LED_STRIPE_COUNT ];  // #! shadow every color for darkness
    static CRGB wlan_discon_colr;                           //! color if wlan disconnected
    static CRGB wlan_search_colr;                           //! color if wland connecting
    static CRGB wlan_connect_colr;                          //! color if wlan connected
    static CRGB wlan_connect_and_sync_colr;                 //! color if WLAN connected and time synced
    static CRGB wlan_fail_col;                              //! color if wlan failed
    static CRGB http_active;                                //! color if webserver is active

    public:
    static TaskHandle_t taskHandle;

    public:
    static void init();                                                     //! init system
    static void setLed( uint8_t, bool, bool = true );                       //! led(s) on/off
    static void setLed( uint8_t, uint8_t, uint8_t, uint8_t, bool = true );  //! on or all led set color
    static void setLed( uint8_t, CRGB &, bool = true );                     //! one or all led set color
    static void setLed( uint8_t, uint32_t, bool = true );                   //! one or all led set color

    private:
    static void start();                     //! start led thread
    static void ledTask( void * );           //! the task fir LED
    static int64_t wlanStateLoop( bool * );  //! chcek wlan state
  };

}  // namespace alertclock
