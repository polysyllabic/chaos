/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
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
#include <string>

namespace Chaos {

  enum ButtonID {
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
  };
 
 enum AxisID {
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
  };

  enum ButtonType {
    TYPE_BUTTON = 0,
    TYPE_AXIS = 1
  };


  /**
   * \brief An enumeration of the possible types of controller inputs.
   *
   * This provides a unified enumeration of all buttons and axes. The value NOTHING is in the
   * to_colsole portion of a signal remap, and indicates that the signal should be dropped
   * rather than remapped. The value NONE is equivalent to a nullptr. We do this both to
   * allow for future support of other controllers that may have different low-leve signals.
   */
  enum class ControllerSignal {
    X, CIRCLE, TRIANGLE, SQUARE,
    L1, R1, L2, R2,
    SHARE, OPTIONS, PS,
    L3, R3,
    TOUCHPAD, TOUCHPAD_ACTIVE, TOUCHPAD_ACTIVE_2,
    LX, LY, RX, RY,
    DX, DY,
    ACCX, ACCY, ACCZ,
    GYRX, GYRY, GYRZ,
    TOUCHPAD_X, TOUCHPAD_Y,
    TOUCHPAD_X_2, TOUCHPAD_Y_2,
    NOTHING, NONE
  };

  /**
   * \brief Define the type of controller input.
   * 
   * This is more specific than the basic ButtonType for hints when remapping controls and other
   * occasions where specific subtypes of controller inputs require differnt handling. The DUMMY
   * class is used for input types that don't actually come from the controller (currently NONE
   * and NOTHING).
   */
  enum class ControllerSignalType {
    BUTTON,
    THREE_STATE,
    AXIS,
    HYBRID,
    ACCELEROMETER,
    GYROSCOPE,
    TOUCHPAD,
    DUMMY
  };

  /**
   * \brief Structure to hold the hard-coded information about the signals coming from the controller.
   * 
   * \todo Add ability to translate all signals so a dualsense can pretend to be a dualshock.
   */
  struct SignalSettings {
    const std::string name;
    ControllerSignal input;
    ControllerSignalType type;
    uint8_t id;
    uint8_t hybrid_id;
};

};