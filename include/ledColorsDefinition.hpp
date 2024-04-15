#pragma once
#include <FastLED.h>

namespace appprefs
{
  // LED_COLOR_XXX uint32 in hex: RED-GREEN-BLUE
  constexpr uint32_t LED_COLOR_WHITE{ 0xFFFFFF };       //! LED WHITE
  constexpr uint32_t LED_COLOR_BLACK{ 0x0 };            //! LED black
  constexpr uint32_t LED_COLOR_ALERT{ 0xFF0000 };       //! LES RED - ALERT
  constexpr uint32_t LED_COLOR_WARNING{ 0xFAF7A2 };     //! LED yellow-orange - WARNUNG
  constexpr uint32_t LED_COLOR_FORMATTING{ 0xFFB000 };  //! LED orange - format SPIFFS

  constexpr uint32_t LED_COLOR_WLAN_DISCONN{ 0xfa0000 };
  constexpr uint32_t LED_COLOR_WLAN_SEARCH{ 0x664106 };
  constexpr uint32_t LED_COLOR_WLAN_CONNECT{ 0x020200 };
  constexpr uint32_t LED_COLOR_WLAN_CONNECT_AND_SYNC{ 0x000200 };
  constexpr uint32_t LED_COLOR_WLAN_FAIL{ 0x0a0000 };

  constexpr uint32_t LED_COLOR_ALERT_IDLE{ 0x010200 };
  constexpr uint32_t LED_COLOR_ALERT_PREPARE{ 0xFAF7A2 };
  constexpr uint32_t LED_COLOR_ALERT_RUNNING{ 0x00ff00 };
  constexpr uint32_t LED_COLOR_ALERT_FAIL{ 0xFF0000 };
  constexpr uint32_t LED_COLOR_ALERT_RUN_AND_FAIL{ 0xFFff00 };

  constexpr uint32_t LED_COLOR_HTTP_ACTIVE{ 0x50B050 };

}  // namespace appprefs
