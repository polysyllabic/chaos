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
#include <cassert>
#include <climits>
#include <vector>

#include <plog/Log.h>

#include "ControllerInput.hpp"
#include "Controller.hpp"
#include "signals.hpp"

using namespace Chaos;


ControllerInput::ControllerInput(Controller& c, const SignalSettings& settings) :
  controller{c},
  name{settings.name},
  actual{settings.input},
  input_type{settings.type},
  button_id{settings.id},
  hybrid_axis{settings.hybrid_id}  {
  button_index = ((int) getButtonType() << 8) + (int) button_id;
  if (input_type == ControllerSignalType::HYBRID) {
    hybrid_index = ((int) TYPE_AXIS << 8) + (int) hybrid_axis;
  }
}

ControllerSignalType ControllerInput::getType(const DeviceEvent& event) {
  assert (event.type <= TYPE_AXIS);
  ControllerSignalType rval;
  if (event.type == TYPE_BUTTON) {
    rval = (event.id == BUTTON_L2 || event.id == BUTTON_R2) ? ControllerSignalType::HYBRID : ControllerSignalType::BUTTON;
  } else {
    switch (event.type) {
    case AXIS_L2:
    case AXIS_R2:
      rval = ControllerSignalType::HYBRID;
      break;
    case AXIS_DX:
    case AXIS_DY:
      rval = ControllerSignalType::THREE_STATE;
      break;
    case AXIS_ACCX:
    case AXIS_ACCY:
    case AXIS_ACCZ:
      rval = ControllerSignalType::ACCELEROMETER;
      break;
    case AXIS_GYRX:
    case AXIS_GYRY:
    case AXIS_GYRZ:
      rval = ControllerSignalType::GYROSCOPE;
      break;
    case AXIS_TOUCHPAD_X:
    case AXIS_TOUCHPAD_Y:
      rval = ControllerSignalType::TOUCHPAD;
      break;
    default:
      rval = ControllerSignalType::AXIS;
    }
  }
  return rval;
}

short ControllerInput::getMin(ButtonType type) {
  switch (input_type) {
  case ControllerSignalType::BUTTON:
  case ControllerSignalType::DUMMY:
    return 0;
  case ControllerSignalType::THREE_STATE:
    return -1;
  case ControllerSignalType::HYBRID:
    return (type == TYPE_AXIS) ? JOYSTICK_MIN : 0;
  case ControllerSignalType::AXIS:
    return JOYSTICK_MIN;
  }
  // The accelerometer, gyroscope, and touchpad signals probably should never be set to the extreme,
  // but we allow it
  return SHRT_MIN;
}

short ControllerInput::getMax(ButtonType type) {
  switch (input_type) {
  case ControllerSignalType::BUTTON:
  case ControllerSignalType::THREE_STATE:
    return 1;
  case ControllerSignalType::HYBRID:
    return (type == TYPE_AXIS) ? JOYSTICK_MAX : 1;
  case ControllerSignalType::AXIS:
    return JOYSTICK_MAX;    
  case ControllerSignalType::DUMMY:
    return 0;
  }
  // Accelerometer, gyroscope, and touchpad fall through to here
  return SHRT_MAX;
}

// necessary anymore?
short ControllerInput::getMax(std::shared_ptr<ControllerInput> signal) {
  return signal->getMax(TYPE_AXIS);
}

short ControllerInput::getState() {
  return (getType() == ControllerSignalType::DUMMY) ? 0 :
    controller.getState(button_id, getButtonType());  
}

bool ControllerInput::matches(const DeviceEvent& event) {
  bool rval = (event.index() == button_index || 
    (input_type == ControllerSignalType::HYBRID && event.index() == hybrid_index));
  PLOG_DEBUG << name << ": button_index=" << button_index << "; hybrid_index=" << hybrid_index << "; event index=" << 
    event.index() << "; match=" << (rval ? "YES" : "NO");
  return rval;
}