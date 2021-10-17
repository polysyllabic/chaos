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

#include "config.hpp"
#include "controllerState.hpp"
#include "dualshock.hpp"

#ifdef USE_DUALSENSE
#include "dualsense.hpp"
#endif

using namespace Chaos;

std::vector<ControllerCommand> ControllerState::buttonInfo;

ControllerState::~ControllerState() {}

void* ControllerState::getHackedState() {
  shouldClearTouchpadCount = true;
  return hackedState;
}

ControllerState* ControllerState::factory(int vendor, int product) {

  // These are the values for the DualShock2, the only controller currently supported. They're
  // defined with ID values that can be changed in case we are able to add additional controller
  // types some day. NB: the order of insertion must match the enumeration in GPInput, since we
  // use that as an index for this vector.
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 1)); // GPInput::X
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 2)); // GPInput::TRIANGLE
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 3)); // GPInput::SQUARE
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 4)); // GPInput::L1
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 5)); // GPInput::R1
  buttonInfo.push_back(ControllerCommand(GPInputType::HYBRID, 6, 2)); // GPInput::L2
  buttonInfo.push_back(ControllerCommand(GPInputType::HYBRID, 7, 5)); // GPInput::R2
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 8)); // GPInput::SHARE
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 9)); // GPInput::OPTIONS
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 10)); // GPInput::PS
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 11)); // GPInput::L3
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 12)); // GPInput::R3
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 13)); // GPInput::TOUCHPAD
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 14)); // GPInput::TOUCHPAD_ACTIVE
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 15)); // GPInput::TOUCHPAD_ACTIVE_2
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 0)); // GPInput::LX
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 1)); // GPInput::LY
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 3)); // GPInput::RX
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 4)); // GPInput::RY
  buttonInfo.push_back(ControllerCommand(GPInputType::THREE_STATE, 6)); // GPInput::DX
  buttonInfo.push_back(ControllerCommand(GPInputType::THREE_STATE, 7)); // GPInput::DY
  buttonInfo.push_back(ControllerCommand(GPInputType::ACCELEROMETER, 8)); // GPInput::ACCX
  buttonInfo.push_back(ControllerCommand(GPInputType::ACCELEROMETER, 9)); // GPInput::ACCY
  buttonInfo.push_back(ControllerCommand(GPInputType::ACCELEROMETER, 10)); // GPInput::ACCZ
  buttonInfo.push_back(ControllerCommand(GPInputType::GYROSCOPE, 11)); // GPInput::GYRX
  buttonInfo.push_back(ControllerCommand(GPInputType::GYROSCOPE, 12)); // GPInput::GYRY
  buttonInfo.push_back(ControllerCommand(GPInputType::GYROSCOPE, 13)); // GPInput::GYRZ
  buttonInfo.push_back(ControllerCommand(GPInputType::TOUCHPAD, 14)); // GPInput::TOUCHPAD_X
  buttonInfo.push_back(ControllerCommand(GPInputType::TOUCHPAD, 15)); // GPInput::TOUCHPAD_Y
  buttonInfo.push_back(ControllerCommand(GPInputType::TOUCHPAD, 16)); // GPInput::TOUCHPAD_X_2
  buttonInfo.push_back(ControllerCommand(GPInputType::TOUCHPAD, 17)); // GPInput::TOUCHPAD_Y_2

  assert(buttonInfo.size() == GPInput::__COUNT__);
  
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

