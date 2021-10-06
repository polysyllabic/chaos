/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
#ifndef DEVICETYPES_HPP
#define DEVICETYPES_HPP
#include <cstdint>

namespace Chaos {

  // Originally, the button and axis enumerations gave the specific numeric
  // values for the DualShock. To support other controllers without needing
  // to recompile, we now treat this as an arbitrary enumeration of the
  // button/axis names, and each controller class must define a numerical
  // mapping onto this enumeration.
  enum class Button {
    X,
    CIRCLE,
    TRIANGLE,
    SQUARE,
    L1,
    R1,
    L2,
    R2,
    SHARE,
    OPTIONS,
    PS,
    L3,
    R3,
    TOUCHPAD,
    TOUCHPAD_ACTIVE
  };
 
  enum class Axis {
    LX,
    LY,
    L2,
    RX,
    RY,
    R2,
    DX,
    DY,
    ACCX,
    ACCY,
    ACCZ,
    GYRX,
    GYRY,
    GYRZ,
    TOUCHPAD_X,
    TOUCHPAD_Y
  };

  typedef enum ButtonType {
    TYPE_BUTTON = 0,
    TYPE_AXIS = 1
  } ButtonType;

  typedef struct DeviceEvent {
    unsigned int time;
    short int value;
    uint8_t type;
    uint8_t id;
	
    bool operator==(const DeviceEvent &other) const {
      return type == other.type && id == other.id;
    }

    bool operator<(const DeviceEvent &other) const {
      return type < other.type || (type == other.type && id < other.id);
    }
  } DeviceEvent;

};

#endif
