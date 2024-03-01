#pragma once
#include <memory>
#include "deviceDiscover.hpp"
#include "soundTouchAlert.hpp"

void setup();
void loop();
std::shared_ptr< soundtouch::SoundTouchAlert > doTestThingsIfOnline();
void doTestThingsIfOffline( std::shared_ptr< soundtouch::SoundTouchAlert > );
void testLoop( std::shared_ptr< soundtouch::SoundTouchAlert > );
