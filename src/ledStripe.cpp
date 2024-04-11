#include "common.hpp"
#include "statusObject.hpp"
#include "statics.hpp"
#include "ledStripe.hpp"
#include "appPreferences.hpp"
#include "ledColorsDefinition.hpp"

namespace alertclock
{
  //
  // timer definitions
  //
  constexpr int64_t WLANlongActionDist = getMicrosForMiliSec( 2500 );
  constexpr int64_t WLANshortActionDist = getMicrosForMiliSec( 20 );
  constexpr int64_t HTTPActionFlashDist = getMicrosForMiliSec( 35 );
  constexpr int64_t HTTPActionDarkDist = getMicrosForMiliSec( 60 );
  constexpr int64_t AlertActionDarkDist = getMicrosForMiliSec( 250 );

  //
  // color definitions
  //
  CRGB LEDStripe::inactive_colr{ appprefs::LED_COLOR_BLACK };
  CRGB LEDStripe::wlan_discon_colr{ appprefs::LED_COLOR_WLAN_DISCONN };
  CRGB LEDStripe::wlan_search_colr{ appprefs::LED_COLOR_WLAN_SEARCH };
  CRGB LEDStripe::wlan_connect_colr{ appprefs::LED_COLOR_WLAN_SEARCH };
  CRGB LEDStripe::wlan_connect_and_sync_colr{ appprefs::LED_COLOR_WLAN_CONNECT_AND_SYNC };
  CRGB LEDStripe::wlan_fail_colr{ appprefs::LED_COLOR_WLAN_FAIL };
  CRGB LEDStripe::alert_idle_colr{ appprefs::LED_COLOR_ALERT_IDLE };
  CRGB LEDStripe::alert_prepare_colr{ appprefs::LED_COLOR_ALERT_PREPARE };
  CRGB LEDStripe::alert_running_colr{ appprefs::LED_COLOR_ALERT_RUNNING };
  CRGB LEDStripe::alert_fail_colr{ appprefs::LED_COLOR_ALERT_FAIL };
  CRGB LEDStripe::alert_run_and_fail{ appprefs::LED_COLOR_ALERT_RUN_AND_FAIL };
  CRGB LEDStripe::http_active_colr{ appprefs::LED_COLOR_HTTP_ACTIVE };

  const char *LEDStripe::tag{ "ledstripe" };
  TaskHandle_t LEDStripe::taskHandle{ nullptr };
  CRGB LEDStripe::leds[ appprefs::LED_STRIPE_COUNT ]{};
  CRGB LEDStripe::shadow_leds[ appprefs::LED_STRIPE_COUNT ]{};

  /**
   * init led system
   */
  void LEDStripe::init()
  {
    FastLED.addLeds< WS2811, appprefs::LED_STRIPE_RMT_TX_GPIO, appprefs::LED_RGB_ORDER >( leds, appprefs::LED_STRIPE_COUNT )
        .setCorrection( TypicalLEDStrip );
    FastLED.setBrightness( appprefs::LED_STRIP_BRIGHTNESS );
    LEDStripe::setLed( appprefs::LED_ALL, false, true );
    LEDStripe::setLed( appprefs::LED_WLAN, LEDStripe::wlan_discon_colr, true );
    LEDStripe::start();
    elog.log( logger::DEBUG, "%s: initialized...", LEDStripe::tag );
  }

  void LEDStripe::start()
  {
    elog.log( logger::INFO, "%s: LEDStripe Task start...", LEDStripe::tag );

    if ( LEDStripe::taskHandle )
    {
      vTaskDelete( LEDStripe::taskHandle );
      LEDStripe::taskHandle = nullptr;
    }
    else
    {
      xTaskCreate( LEDStripe::ledTask, "led-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY + 1,
                   &LEDStripe::taskHandle );
    }
  }

  void LEDStripe::ledTask( void * )
  {
    using namespace logger;
    static const char *tag{ "ledtask" };
    elog.log( INFO, "%s: LEDStripe Task starting...", tag );
    using namespace appprefs;
    int64_t nextWLANLedActionTime{ WLANlongActionDist };
    int64_t nextHTTPLedActionTime{ HTTPActionDarkDist };
    int64_t nextAlertStateActionTime{ AlertActionDarkDist };
    int64_t nowTime = esp_timer_get_time();
    int64_t nextMark =
        esp_timer_get_time() + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
    bool led_changed{ false };

    while ( true )
    {
      nowTime = esp_timer_get_time();
      //
      // ist it time for led action
      //
      if ( nextWLANLedActionTime < nowTime )
      {
        // make led things
        nextWLANLedActionTime = LEDStripe::wlanStateLoop( &led_changed );
      }
      if ( nextHTTPLedActionTime < nowTime )
      {
        //
        // time for http-indicator-action
        //
      }
      if ( nextAlertStateActionTime < nowTime )
      {
        //
        // make led alert-status things
        //
        nextAlertStateActionTime = LEDStripe::alertStateLoop( &led_changed );
      }
      if ( led_changed )
      {
        // set LED'S
        FastLED.show();
        led_changed = false;
      }
      //
      // what is the smallest time to next event?
      // need for delay
      //
      yield();
      if ( nextMark < esp_timer_get_time() )
      {
        elog.log( DEBUG, "%s: ==== MARK ====", tag );
        nextMark =
            esp_timer_get_time() + getMicrosForMiliSec( appprefs::TASK_MARK_INTERVAL_MS + static_cast< int32_t >( random( 2000 ) ) );
      }
    }
  }

  /**
   * loop to compute LED for alert state
   */
  int64_t LEDStripe::alertStateLoop( bool *led_changed )
  {
    using namespace appprefs;
    static bool stateLedSwitch{ true };
    static volatile AlertState cAlertState{ AlertState::ALERT_UNKNOWN };
    int64_t nextDist{ AlertActionDarkDist };
    //
    //
    if ( cAlertState != StatusObject::getAlertState() )
    {
      *led_changed = true;
      // mark state
      cAlertState = StatusObject::getAlertState();
      switch ( cAlertState )
      {
        case AlertState::ALERT_NONE:
          LEDStripe::setLed( LED_ALERT, LEDStripe::alert_idle_colr, false );
          break;

        case AlertState::ALERT_PREPARING:
          LEDStripe::setLed( LED_ALERT, LEDStripe::alert_prepare_colr, false );
          break;

        case AlertState::ALERT_RUNNING:
          LEDStripe::setLed( LED_ALERT, LEDStripe::alert_running_colr, false );
          break;

        case AlertState::ALERT_FAIL:
          LEDStripe::setLed( LED_ALERT, LEDStripe::alert_fail_colr, false );
          nextDist = AlertActionDarkDist << 5;
          break;

        case AlertState::ALERT_RUN_AND_FAIL:
          LEDStripe::setLed( LED_ALERT, LEDStripe::alert_run_and_fail, false );
          nextDist = AlertActionDarkDist << 5;
          break;

        default:
        case AlertState::ALERT_UNKNOWN:
          LEDStripe::setLed( LED_ALERT, LEDStripe::alert_idle_colr, false );
          break;
      }
    }
    int64_t nextLoopTime = esp_timer_get_time() + nextDist;
    stateLedSwitch = !stateLedSwitch;
    return nextLoopTime;
  }

  /**
   * loop to compute LED for wlan state
   */
  int64_t LEDStripe::wlanStateLoop( bool *led_changed )
  {
    using namespace appprefs;
    static bool wlanLedSwitch{ true };
    static volatile WlanState cWlanState{ WlanState::FAILED };
    if ( cWlanState != StatusObject::getWlanState() )
    {
      *led_changed = true;
      // mark state
      cWlanState = StatusObject::getWlanState();
      switch ( cWlanState )
      {
        case WlanState::DISCONNECTED:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_discon_colr, false );
          break;
        case WlanState::SEARCHING:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_search_colr, false );
          break;
        case WlanState::CONNECTED:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_connect_colr, false );
          break;
        case WlanState::TIMESYNCED:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_connect_and_sync_colr, false );
          // do here nothing
          break;
        default:
        case WlanState::FAILED:
          LEDStripe::setLed( LED_WLAN, LEDStripe::wlan_fail_colr, false );
          break;
      }
    }
    int64_t nextLoopTime = esp_timer_get_time() + WLANlongActionDist;
    wlanLedSwitch = !wlanLedSwitch;
    return nextLoopTime;
  }

  /**
   * switch LED on or off
   */
  void LEDStripe::setLed( uint8_t led, bool onOff, bool imediate )
  {
    if ( onOff )
    {
      // ON
      if ( led == appprefs::LED_ALL )
      {
        // all LED
        if ( imediate )
          FastLED.show();
      }
      if ( led < appprefs::LED_STRIPE_COUNT )
      {
        // okay, one LED switch ON => restore shadow to current
        leds[ led ] = shadow_leds[ led ];
        if ( imediate )
          FastLED.show();
      }
    }
    else
    {
      // LED OFF
      if ( led == appprefs::LED_ALL )
      {
        // all LED
        CRGB loc_led{};
        if ( imediate )
          FastLED.showColor( loc_led );
      }
      if ( led < appprefs::LED_STRIPE_COUNT )
      {
        // okay one ld switch OFF
        leds[ led ].r = 0;
        leds[ led ].g = 0;
        leds[ led ].b = 0;
        if ( imediate )
          FastLED.show();
      }
    }
  }

  /**
   * LED (alle oder eine) auf einen Wert setzten
   */
  void LEDStripe::setLed( uint8_t led, uint8_t red, uint8_t green, uint8_t blue, bool imediate )
  {
    if ( led == appprefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < appprefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ].r = leds[ i ].r = red;
        shadow_leds[ i ].g = leds[ i ].g = green;
        shadow_leds[ i ].b = leds[ i ].b = blue;
      }
      if ( imediate )
        FastLED.show();
    }
    if ( led < appprefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ].r = leds[ led ].r = red;
      shadow_leds[ led ].g = leds[ led ].g = green;
      shadow_leds[ led ].b = leds[ led ].b = blue;
      if ( imediate )
        FastLED.show();
    }
  }

  /**
   * eine oder alle LED auf eine Farbe setzten
   */
  void LEDStripe::setLed( uint8_t led, CRGB &rgb, bool imediate )
  {
    if ( led == appprefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < appprefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ] = leds[ i ] = rgb;
      }
      if ( imediate )
        FastLED.show();
    }
    if ( led < appprefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ] = leds[ led ] = rgb;
      if ( imediate )
        FastLED.show();
    }
  }

  /**
   * eine oder alle LED auf eine RAW Farbe in RGB setzten
   */
  void LEDStripe::setLed( uint8_t led, uint32_t color, bool imediate )
  {
    if ( led == appprefs::LED_ALL )
    {
      // all LED
      for ( uint8_t i = 0; i < appprefs::LED_STRIPE_COUNT; ++i )
      {
        // okay one ld switch OFF
        shadow_leds[ i ] = leds[ i ] = CRGB( color );
      }
      if ( imediate )
        FastLED.show();
    }
    if ( led < appprefs::LED_STRIPE_COUNT )
    {
      // okay one ld switch OFF
      shadow_leds[ led ] = leds[ led ] = CRGB( color );
      if ( imediate )
        FastLED.show();
    }
  }

}  // namespace alertclock
