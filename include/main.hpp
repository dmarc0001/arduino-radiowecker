#pragma once
#include <memory>
#include "deviceDiscover.hpp"
#include "soundtouchDevice.hpp"

void setup();
void loop();
std::shared_ptr< soundtouch::SoundtouchDevice > doTestThingsIfOnline();
void doTestThingsIfOffline( std::shared_ptr< soundtouch::SoundtouchDevice > );
