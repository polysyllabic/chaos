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

#include "controllerInput.hpp"
#include "controller.hpp"

using namespace Chaos;

// Would we ever want to expose these definitions in the config file?
// These are the values for the DualShock2, the only controller currently supported. They're
// defined with ID values that can be changed in case we are able to add additional controller
// types. For the dualsense, we would need to remap the signals and pretend to be a dualshock,
// because of its encrypted checksum, which currently prevents us from altering signals from 
// that controller.
std::vector<SignalSettings> signal_settings = {
  { "X", ControllerSignal::X, ControllerSignalType::BUTTON, 0, 0 },
  { "CIRCLE",  ControllerSignal::CIRCLE, ControllerSignalType::BUTTON, 1, 0 },
  { "TRIANGLE",  ControllerSignal::TRIANGLE, ControllerSignalType::BUTTON, 2, 0 },
  { "SQUARE",  ControllerSignal::SQUARE, ControllerSignalType::BUTTON, 3, 0 },
  { "L1",  ControllerSignal::L1, ControllerSignalType::BUTTON, 4, 0 },
  { "R1",  ControllerSignal::R1, ControllerSignalType::BUTTON, 5, 0 },
  { "L2",  ControllerSignal::L2, ControllerSignalType::HYBRID, 6, 2 },
  { "R2",  ControllerSignal::R2, ControllerSignalType::HYBRID, 7, 5 },
  { "SHARE",  ControllerSignal::SHARE, ControllerSignalType::BUTTON, 8, 0 },
  { "OPTIONS",  ControllerSignal::OPTIONS, ControllerSignalType::BUTTON, 9, 0 },
  { "PS",  ControllerSignal::PS, ControllerSignalType::BUTTON, 10, 0 },
  { "L3",  ControllerSignal::L3, ControllerSignalType::BUTTON, 11, 0 },
  { "R3",  ControllerSignal::R3, ControllerSignalType::BUTTON, 12, 0 },
  { "TOUCHPAD",  ControllerSignal::TOUCHPAD, ControllerSignalType::BUTTON, 13, 0 },
  { "TOUCHPAD_ACTIVE",  ControllerSignal::TOUCHPAD_ACTIVE, ControllerSignalType::BUTTON, 14, 0 },
  { "TOUCHPAD_ACTIVE_2",  ControllerSignal::TOUCHPAD_ACTIVE_2, ControllerSignalType::BUTTON, 15, 0 },
  { "LX",  ControllerSignal::LX, ControllerSignalType::AXIS, 0, 0 },
  { "LY",  ControllerSignal::LY, ControllerSignalType::AXIS, 1, 0 },
  { "RX",  ControllerSignal::RX, ControllerSignalType::AXIS, 3, 0 },
  { "RY",  ControllerSignal::RY, ControllerSignalType::AXIS, 4, 0 },
  { "DX",  ControllerSignal::DX, ControllerSignalType::THREE_STATE, 6, 0 },
  { "DY",  ControllerSignal::DY, ControllerSignalType::THREE_STATE, 7, 0 },
  { "ACCX",  ControllerSignal::ACCX, ControllerSignalType::ACCELEROMETER, 8, 0 },
  { "ACCY",  ControllerSignal::ACCY, ControllerSignalType::ACCELEROMETER, 9, 0 },
  { "ACCZ",  ControllerSignal::ACCZ, ControllerSignalType::ACCELEROMETER, 10, 0 },
  { "GYRX",  ControllerSignal::GYRX, ControllerSignalType::GYROSCOPE, 11, 0 },
  { "GYRY",  ControllerSignal::GYRY, ControllerSignalType::GYROSCOPE, 12, 0 },
  { "GYRZ",  ControllerSignal::GYRZ, ControllerSignalType::GYROSCOPE, 13, 0 },
  { "TOUCHPAD_X",  ControllerSignal::TOUCHPAD_X, ControllerSignalType::TOUCHPAD, 14, 0 },
  { "TOUCHPAD_Y",  ControllerSignal::TOUCHPAD_Y, ControllerSignalType::TOUCHPAD, 15, 0 },
  { "TOUCHPAD_X_2",  ControllerSignal::TOUCHPAD_X_2, ControllerSignalType::TOUCHPAD, 16, 0 },
  { "TOUCHPAD_Y_2",  ControllerSignal::TOUCHPAD_Y_2, ControllerSignalType::TOUCHPAD, 17, 0 },
  { "NOTHING", ControllerSignal::NOTHING, ControllerSignalType::DUMMY, 0, 0 },
  { "NONE", ControllerSignal::NONE, ControllerSignalType::DUMMY, 0, 0 },
};

std::unordered_map<ControllerSignal, std::shared_ptr<ControllerInput>> ControllerInput::inputs;
std::unordered_map<std::string, std::shared_ptr<ControllerInput>> ControllerInput::by_name;
std::unordered_map<int, std::shared_ptr<ControllerInput>> ControllerInput::by_index;

ControllerInput::ControllerInput(const SignalSettings& settings) :
  name{settings.name},
  actual{settings.input},
  remap(ControllerSignal::NONE, ControllerSignal::NONE, false, false, 0, 1),
  input_type{settings.type},
  button_id{settings.id},
  hybrid_axis{settings.hybrid_id}  {
  button_index = ((int) getButtonType() << 8) + (int) button_id;
  if (input_type == ControllerSignalType::HYBRID) {
    hybrid_index = ((int) TYPE_AXIS << 8) + (int) hybrid_axis;
  }
}

void ControllerInput::initialize() {
  // we use the hard-coded values in signal_settings to initialize our maps
  for (SignalSettings s : signal_settings) {
    PLOG_VERBOSE << "Initializing signal " << s.name;
    std::shared_ptr<ControllerInput> sig = std::make_shared<ControllerInput>(s);
    inputs.insert({s.input, sig});
    by_name.insert({s.name, sig});
    by_index.insert({sig->getIndex(), sig});
    if (s.type == ControllerSignalType::HYBRID) {
      by_index.insert({sig->getHybridAxisIndex(), sig});
    }
  }
}

std::shared_ptr<ControllerInput> ControllerInput::get(const std::string& name) {
  auto iter = by_name.find(name);
  if (iter != by_name.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<ControllerInput> ControllerInput::get(ControllerSignal signal) { 
  return inputs[signal];
  }

std::shared_ptr<ControllerInput> ControllerInput::get(const DeviceEvent& event) {  
  auto iter = by_index.find(event.index());
  if (iter != by_index.end()) {
    return iter->second;
  }
  return nullptr;
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

// Does the event ID match this command?
bool ControllerInput::matchesID(const DeviceEvent& event, ControllerSignal gp) {
  // for L2/R2, we need to check both the button and the axis
  if (inputs[gp]->getType() == ControllerSignalType::HYBRID) {
    return ((event.type == TYPE_BUTTON && event.id == inputs[gp]->getID()) ||
	    (event.type == TYPE_AXIS && event.id == inputs[gp]->getHybridAxis()));
  }
  return (event.id == inputs[gp]->getID() && event.type == inputs[gp]->getButtonType());
}

void ControllerInput::setCascadingRemap(const SignalRemap& remapping) {
  for (auto r : inputs) {
    // to-signal already remapped, so apply to Cascade
    if (r.second->getRemap() == remapping.to_console) {
      r.second->setRemap(remapping);
      return;
    }
  }
  // signal not currently remapped
  remap = remapping;
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
    Controller::instance().getState(button_id, getButtonType());  
}

// translate input value using the current remap settings
short ControllerInput::remapValue(short value) {
  short rval = value;
  std::shared_ptr<ControllerInput> to = get(getRemap());
  if (getType() == ControllerSignalType::DUMMY || to->getType() == ControllerSignalType::DUMMY) {
    return 0;
  }
  // Scaling only needs to occur when remapping between different input classes and the value is
  // non-zero. Any specific remappings not listed here will be passed through unchanged. The ones
  // that make no sense should have been screened out during initialization of the remap modifiers.
  if (value && getType() != to->getType()) {
    switch (getType()) {
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
        short new_val = (getType() == ControllerSignalType::BUTTON) ? 1 : -1;
        if (value > 0) {
          rval = (!toMin() && value > remap.threshold) ? 1 : 0;
        } else if (value < 0) {
          rval = (toMin() && value < -remap.threshold) ? new_val : 0;
        }
   	  }
      break;
    case ControllerSignalType::ACCELEROMETER:
      assert(getRemapSensitivity());
      if (to->getType() == ControllerSignalType::AXIS) {
	      rval = joystickLimit((short) (-value/getRemapSensitivity()));
      }
      break;
    case ControllerSignalType::TOUCHPAD:
      if (getType() == ControllerSignalType::AXIS) {
	      rval = Touchpad::instance().toAxis(getRemap(), value);
      }
    } // end outer switch
  } 
  // Apply any inversion.
  return remapInverted() ? joystickLimit(-rval) : rval;
}

// Peeking at the current state of the remapped signal
short ControllerInput::getRemappedState() {
  if (getRemap() == ControllerSignal::NONE) {
    // No remapping
    return getState();
  }
  // Get the remapped signal
  std::shared_ptr<ControllerInput> source = get(getRemap());  
  assert(source);

  ControllerSignalType source_type = source->getType();
  
  // If the signal we're looking at is NOTHING or NONE, it's always 0
  if (source_type == ControllerSignalType::DUMMY) {
    return 0;
  }

  // Get the current value of the remapped signal
  short value = source->getState();
  short rval = value;
  PLOG_DEBUG << source->getName() << " currently " << value;

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
        rval = value - get(getNegRemap())->getState();
      }
      break;
    case ControllerSignalType::AXIS:
      switch (source_type) {
      case ControllerSignalType::BUTTON:
        rval = joystickLimit((value - get(getNegRemap())->getState())*JOYSTICK_MAX);
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
