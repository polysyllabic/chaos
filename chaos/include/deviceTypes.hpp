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
#pragma once
#include <cstdint>

namespace Chaos {

  typedef enum ButtonID {
  BUTTON_X = 0,
  BUTTON_CIRCLE = 1,
  BUTTON_TRIANGLE = 2,
  BUTTON_SQUARE = 3,
  BUTTON_L1 = 4,
  BUTTON_R1 = 5,
  BUTTON_L2 = 6,
  BUTTON_R2 = 7,
  BUTTON_SHARE = 8,
  BUTTON_OPTIONS = 9,
  BUTTON_PS = 10,
  BUTTON_L3 = 11,
  BUTTON_R3 = 12,
  BUTTON_TOUCHPAD = 13,
  BUTTON_TOUCHPAD_ACTIVE = 14,
  BUTTON_TOUCHPAD_ACTIVE_2 = 15
} ButtonID;
 
 typedef enum AxisID {
   AXIS_LX = 0,
   AXIS_LY = 1,
   AXIS_L2 = 2,
   AXIS_RX = 3,
   AXIS_RY = 4,
   AXIS_R2 = 5,
   AXIS_DX = 6,
   AXIS_DY = 7,
   AXIS_ACCX = 8,
   AXIS_ACCY = 9,
   AXIS_ACCZ = 10,
   AXIS_GYRX = 11,
   AXIS_GYRY = 12,
   AXIS_GYRZ = 13,
   AXIS_TOUCHPAD_X = 14,
   AXIS_TOUCHPAD_Y = 15,
   AXIS_TOUCHPAD_X_2 = 16,
   AXIS_TOUCHPAD_Y_2 = 17
 } AxisID;

  typedef enum ButtonType {
    TYPE_BUTTON = 0,
    TYPE_AXIS = 1
  } ButtonType;

  typedef struct DeviceEvent {
    unsigned int time;
    short int value;
    uint8_t type;
    uint8_t id;

    int index() const {
      return ((int) type << 8) + (int) id;
    }
    
    bool operator==(const DeviceEvent &other) const {
      return type == other.type && id == other.id;
    }

    bool operator<(const DeviceEvent &other) const {
      return type < other.type || (type == other.type && id < other.id);
    }
  } DeviceEvent;

};
