/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <cstddef>
#include <plog/Log.h>

#include <ControllerState.hpp>
#include <Dualshock.hpp>
#include <signals.hpp>

#ifdef USE_DUALSENSE
#include "Dualsense.hpp"
#endif

using namespace Chaos;

double ControllerState::touchpad_inactive_delay = 0.04;

ControllerState::ControllerState() :
  stateLength(0),
  touchpad_active(false),
  touchpad_timeout_emitted(false),
  touchpad_axis_seen(false) {}

ControllerState::~ControllerState() {}

void* ControllerState::getHackedState() {
  return hackedState;
}

void ControllerState::setTouchpadInactiveDelay(double delay) {
  if (delay < 0.0) {
    PLOG_WARNING << "touchpad_inactive_delay cannot be negative. Using 0.04";
    delay = 0.04;
  }
  touchpad_inactive_delay = delay;
}

void ControllerState::noteTouchpadActiveEvent(short value) {
  // TOUCHPAD_ACTIVE is a button-style signal where non-zero means active.
  touchpad_active = (value != 0);
  touchpad_timeout_emitted = false;
}

void ControllerState::noteTouchpadAxisEvent() {
  touchpad_axis_seen = true;
  touchpad_timeout_emitted = false;
  last_touchpad_axis_event = std::chrono::steady_clock::now();
}

void ControllerState::addTouchpadInactivityEvents(std::vector<DeviceEvent>& events) {
  if (touchpad_inactive_delay <= 0.0 || !touchpad_active || !touchpad_axis_seen || touchpad_timeout_emitted) {
    return;
  }

  auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - last_touchpad_axis_event);
  if (elapsed.count() < touchpad_inactive_delay) {
    return;
  }

  PLOG_DEBUG << "Touchpad inactive for " << elapsed.count()
             << " sec; injecting release and axis reset events.";
  events.push_back({0, 0, TYPE_BUTTON, BUTTON_TOUCHPAD_ACTIVE});
  events.push_back({0, 0, TYPE_AXIS, AXIS_TOUCHPAD_X});
  events.push_back({0, 0, TYPE_AXIS, AXIS_TOUCHPAD_Y});
  touchpad_active = false;
  touchpad_timeout_emitted = true;
  touchpad_axis_seen = false;
}

ControllerState* ControllerState::factory(int vendor, int product) {
  
  if (vendor == 0x054c && product == 0x09cc) {
    return new Dualshock;
  } else if (vendor == 0x054c && product == 0x0ce6) {
#ifdef USE_DUALSENSE
    return new Dualsense;
#else
    PLOG_ERROR << "The DualSense controller is not currently supported.";
#endif    
  } else if (vendor==0x2f24  && product==0x00f8) {	// Magic-S Pro
    return new Dualshock;
  }	
  return nullptr;
}

short int ControllerState::positionDY( const uint8_t& input ) {
  if (input == 0x08) {
    return 0;
  } if (input <= 1 || input == 7) {
    return -1;
  } else if ( input >= 3 && input <= 5) {
    return 1;
  }
  return 0;
}

short int ControllerState::positionDX( const uint8_t& input ) {
  if (input == 0x08) {
    return 0;
  } if (input >= 1 && input <= 3) {
    return 1;
  } else if ( input >= 5 && input <= 7) {
    return -1;
  }
  return 0;
}

uint8_t ControllerState::packDpad( const short int& dx, const short int& dy ) {
  switch (dx) {
  case -1:
    return 6-dy;
    break;
  case 1:
    return 2+dy;
    break;
  case 0:
    if (dy == 0) {
      return 0x08;
    }
    return 2*(1+dy);
  default:
    break;
  }
  return 0x08;
}
