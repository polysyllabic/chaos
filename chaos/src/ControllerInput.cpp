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

using namespace Chaos;


ControllerInput::ControllerInput(Controller& c, const SignalSettings& settings) :
  controller{c},
  name{settings.name},
  actual{settings.input},
  to_console{nullptr},
  to_negative{nullptr},
  remap_threshold{0},
  button_to_min{false},
  remap_invert{false},
  remap_scale{1.0},
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

void ControllerInput::setRemap(const SignalRemap& remapping) { 
  to_console = remapping.to_console;
  to_negative = remapping.to_negative;
  button_to_min = remapping.to_min;
  // When remapping invert signals, a true invert flips the truth status of the current invert
  // boolean. A false invert does nothing. We do this to handle the case where a second
  // inversion is applied on top of an active first. We want the two inversions to cancel
  // each other out.
  if (remapping.invert) {
    remap_invert = !remapping.invert;
  }
  remap_threshold = remapping.threshold;
  remap_scale = remapping.scale;
  assert(remap_scale);
}

// translate input value using the current remap settings
short ControllerInput::remapValue(std::shared_ptr<ControllerInput> to, short value) {
  short rval = value;
  if (getType() == ControllerSignalType::DUMMY || to->getType() == ControllerSignalType::DUMMY) {
    return 0;
  }
  // Scaling only needs to occur when remapping between different input classes and the value is
  // non-zero. Any specific remappings not listed here will be passed through unchanged. The ones
  // that make no sense should have been screened out during initialization of the remap modifiers.
  if (value && input_type != to->getType()) {
    switch (input_type) {
    case ControllerSignalType::BUTTON:
      switch (to->getType()) {
      case ControllerSignalType::THREE_STATE:
        // If the source button maps to the negative dpad value, use -1, otherwise, use 1
        rval = toMin() ? -1 : 1;
        break;
      case ControllerSignalType::AXIS:
        // If the source button maps to the negative axis value, use min, otherwise, use max
        rval = toMin() ? JOYSTICK_MIN : JOYSTICK_MAX;
      // No change required for hybrid
      }
      break;
    case ControllerSignalType::HYBRID:
      // Hybrid to three-state behaves like button to three-state.
      if (to->getType() == ControllerSignalType::THREE_STATE) {
        rval = toMin() ? -1 : 1;
      }
      // Going from hybrid to button or axis, there's no value change.
      break;
    case ControllerSignalType::THREE_STATE:
      switch (to->getType()) {
      case ControllerSignalType::AXIS:
	      // Scale to axis extremes
      	rval = joystickLimit(JOYSTICK_MAX*value);
      	break;
      case ControllerSignalType::BUTTON:
      case ControllerSignalType::HYBRID:
        // if we're mapping a -1 dpad onto a button, it should change to 1
        rval = 1;
      }
      break;
    case ControllerSignalType::AXIS:
      switch (to->getType()) {
      case ControllerSignalType::BUTTON:
      case ControllerSignalType::THREE_STATE:
        short new_val = (input_type == ControllerSignalType::BUTTON) ? 1 : -1;
        if (value > 0) {
          rval = (!button_to_min && value > remap_threshold) ? 1 : 0;
        } else if (value < 0) {
          rval = (button_to_min && value < -remap_threshold) ? new_val : 0;
        }
   	  }
      break;
    case ControllerSignalType::ACCELEROMETER:
      assert(remap_scale);
      if (to->getType() == ControllerSignalType::AXIS) {
	      rval = joystickLimit((short) (-value/remap_scale));
      }
      break;
      // Calling routine will handle touchpad-to-axis value conversion
/*    case ControllerSignalType::TOUCHPAD:
      if (to->getType() == ControllerSignalType::AXIS) {
	      rval = touchpadToAxis(getRemap(), value);
      } */
    } // end outer switch
  } 
  // Apply any inversion.
  return remapInverted() ? joystickLimit(-rval) : rval;
}

// Peeking at the current state of the remapped signal
short ControllerInput::getRemappedState() {
  if (to_console == nullptr) {
    // No remapping
    return getState();
  }

  ControllerSignalType source_type = to_console->getType();
  
  // If the signal we're looking at is NOTHING or NONE, it's always 0
  if (source_type == ControllerSignalType::DUMMY) {
    return 0;
  }

  // Get the current value of the remapped signal
  short value = to_console->getState();
  short rval = value;
  PLOG_DEBUG << to_console->getName() << " currently " << value;

  // Handle the cases when remapping between different input classes
  if (getType() != source_type) {
    switch (getType()) {
    case ControllerSignalType::BUTTON:
    case ControllerSignalType::HYBRID:
      switch (source_type) {
      case ControllerSignalType::THREE_STATE:
      case ControllerSignalType::AXIS:
        if (toMin()) {
          rval = (value < -getThreshold()) ? 1 : 0;
        } else {
          rval = (value > getThreshold()) ? 1 : 0;
        }
      }
      break;
    case ControllerSignalType::THREE_STATE:
      switch (source_type) {
      case ControllerSignalType::AXIS:      
        if (value < 0) {
      	  rval = (value < -getThreshold()) ? -1 : 0;
        } else if (value > 0) {
      	  rval = (value > getThreshold()) ? 1 : 0;
        }
      	break;
      case ControllerSignalType::BUTTON:
      case ControllerSignalType::HYBRID:
        // Need to check both positive and negative remap values. We add them so that if both
        // buttons are pressed they cancel. The positive remap value is already in value
        rval = value - to_negative->getState();
      }
      break;
    case ControllerSignalType::AXIS:
      switch (source_type) {
      case ControllerSignalType::BUTTON:
        rval = joystickLimit((value - to_negative->getState())*JOYSTICK_MAX);
        break;
      case ControllerSignalType::THREE_STATE:
        rval = joystickLimit(value*JOYSTICK_MAX);
	      break;
   	  }
      // Don't bother changing an AXIS value when it goes to ACCELEROMETER, GYROSCOPE, or TOUCHPAD
    } // end outer switch
  } 
  // Return value with any inversion.
  return remapInverted() ? joystickLimit(-rval) : rval;
}
