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
#include "config.h"
#include "controllerState.hpp"
#include "dualshock.hpp"

#ifdef USE_DUALSENSE
#include "dualsense.hpp"
#endif

using namespace Chaos;

ControllerState::~ControllerState() {
}

void* ControllerState::getHackedState() {
  return hackedState;
}

ControllerState* ControllerState::factory(int vendor, int product) {
  if (vendor == 0x054c && product == 0x09cc) {
    return new Dualshock;
  } else if (vendor == 0x054c && product == 0x0ce6) {
#ifdef USE_DUALSENSE
    return new Dualsense;
#else
    PLOG_ERROR << "The DualSense controller is not currently supported.\n";
#endif    
  } else if (vendor==0x2f24  && product==0x00f8) {	// Magic-S Pro
    return new Dualshock;
  }	
  return NULL;
}

short int ControllerState::unpackJoystick(uint8_t& input) {
  return ((short int)input)-128;
}

uint8_t ControllerState::packJoystick(short int& input) {
  return input+128;
}

short int ControllerState::fixShort(short int input) {
  return input;
  // return ((input & 0x00ff) << 8) | ((input & 0xff00) >> 8);
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

