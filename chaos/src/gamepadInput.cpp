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
#include <cassert>
#include <climits>
#include "config.hpp"
#include "gamepadInput.hpp"

using namespace Chaos;

std::unordered_map<std::string, GPInput> GamepadInput::inputNames = {
  { "X", GPInput::X },
  { "CIRCLE",  GPInput::CIRCLE },
  { "TRIANGLE",  GPInput::TRIANGLE },
  { "SQUARE",  GPInput::SQUARE },
  { "L1",  GPInput::L1 },
  { "R1",  GPInput::R1 },
  { "L2",  GPInput::L2 },
  { "R2",  GPInput::L2 },
  { "SHARE",  GPInput::SHARE },
  { "OPTIONS",  GPInput::OPTIONS },
  { "PS",  GPInput::PS },
  { "L3",  GPInput::L3 },
  { "R3",  GPInput::R3 },
  { "TOUCHPAD",  GPInput::TOUCHPAD },
  { "TOUCHPAD_ACTIVE",  GPInput::TOUCHPAD_ACTIVE },
  { "TOUCHPAD_ACTIVE_2",  GPInput::TOUCHPAD_ACTIVE_2 },
  { "LX",  GPInput::LX },
  { "LY",  GPInput::LY },
  { "RX",  GPInput::RX },
  { "RY",  GPInput::RY },
  { "DX",  GPInput::DX },
  { "DY",  GPInput::DY },
  { "ACCX",  GPInput::ACCX },
  { "ACCY",  GPInput::ACCY },
  { "ACCZ",  GPInput::ACCZ },
  { "GYRX",  GPInput::GYRX },
  { "GYRY",  GPInput::GYRY },
  { "GYRZ",  GPInput::GYRZ },
  { "TOUCHPAD_X",  GPInput::TOUCHPAD_X },
  { "TOUCHPAD_Y",  GPInput::TOUCHPAD_Y },
  { "TOUCHPAD_X_2",  GPInput::TOUCHPAD_X_2 },
  { "TOUCHPAD_Y_2",  GPInput::TOUCHPAD_Y_2 },
  { "NONE", GPInput::NONE },
  { "NOTHING", GPInput::NOTHING }
};

// These are the values for the DualShock2, the only controller currently supported. They're
// defined with ID values that can be changed in case we are able to add additional controller
// types some day. NB: the order of insertion must match the enumeration in GPInput, since we
// use that as an index for this vector. The NONE and NOTHING values aren't inserted here,
// since they don't reflect actual inputs from the controller.
std::vector<std::shared_ptr<GamepadInput>> GamepadInput::inputs = {
  std::make_shared<GamepadInput>(GPInput::X, GPInputType::BUTTON, 0),
  std::make_shared<GamepadInput>(GPInput::CIRCLE, GPInputType::BUTTON, 1),
  std::make_shared<GamepadInput>(GPInput::TRIANGLE, GPInputType::BUTTON, 2),
  std::make_shared<GamepadInput>(GPInput::SQUARE, GPInputType::BUTTON, 3),
  std::make_shared<GamepadInput>(GPInput::L1, GPInputType::BUTTON, 4),
  std::make_shared<GamepadInput>(GPInput::R1, GPInputType::BUTTON, 5),
  std::make_shared<GamepadInput>(GPInput::L2, GPInputType::HYBRID, 6, 2),
  std::make_shared<GamepadInput>(GPInput::R2, GPInputType::HYBRID, 7, 5),
  std::make_shared<GamepadInput>(GPInput::SHARE, GPInputType::BUTTON, 8),
  std::make_shared<GamepadInput>(GPInput::OPTIONS, GPInputType::BUTTON, 9),
  std::make_shared<GamepadInput>(GPInput::PS, GPInputType::BUTTON, 10),
  std::make_shared<GamepadInput>(GPInput::L3, GPInputType::BUTTON, 11),
  std::make_shared<GamepadInput>(GPInput::R3, GPInputType::BUTTON, 12),
  std::make_shared<GamepadInput>(GPInput::TOUCHPAD, GPInputType::BUTTON, 13),
  std::make_shared<GamepadInput>(GPInput::TOUCHPAD_ACTIVE, GPInputType::BUTTON, 14),
  std::make_shared<GamepadInput>(GPInput::TOUCHPAD_ACTIVE_2, GPInputType::BUTTON, 15),
  std::make_shared<GamepadInput>(GPInput::LX, GPInputType::AXIS, 0),
  std::make_shared<GamepadInput>(GPInput::LY, GPInputType::AXIS, 1),
  std::make_shared<GamepadInput>(GPInput::RX, GPInputType::AXIS, 3),
  std::make_shared<GamepadInput>(GPInput::RY, GPInputType::AXIS, 4),
  std::make_shared<GamepadInput>(GPInput::DX, GPInputType::THREE_STATE, 6),
  std::make_shared<GamepadInput>(GPInput::DY, GPInputType::THREE_STATE, 7),
  std::make_shared<GamepadInput>(GPInput::ACCX, GPInputType::ACCELEROMETER, 8),
  std::make_shared<GamepadInput>(GPInput::ACCY, GPInputType::ACCELEROMETER, 9),
  std::make_shared<GamepadInput>(GPInput::ACCZ, GPInputType::ACCELEROMETER, 10),
  std::make_shared<GamepadInput>(GPInput::GYRX, GPInputType::GYROSCOPE, 11),
  std::make_shared<GamepadInput>(GPInput::GYRY, GPInputType::GYROSCOPE, 12),
  std::make_shared<GamepadInput>(GPInput::GYRZ, GPInputType::GYROSCOPE, 13),
  std::make_shared<GamepadInput>(GPInput::TOUCHPAD_X, GPInputType::TOUCHPAD, 14),
  std::make_shared<GamepadInput>(GPInput::TOUCHPAD_Y, GPInputType::TOUCHPAD, 15),
  std::make_shared<GamepadInput>(GPInput::TOUCHPAD_X_2, GPInputType::TOUCHPAD, 16),
  std::make_shared<GamepadInput>(GPInput::TOUCHPAD_Y_2, GPInputType::TOUCHPAD, 17),
};

std::unordered_map<int, GPInput> GamepadInput::by_index = {
  { inputs[GPInput::X]->getIndex(), GPInput::X },
  { inputs[GPInput::CIRCLE]->getIndex(), GPInput::CIRCLE },
  { inputs[GPInput::TRIANGLE]->getIndex(), GPInput::TRIANGLE },
  { inputs[GPInput::SQUARE]->getIndex(), GPInput::SQUARE },
  { inputs[GPInput::L1]->getIndex(), GPInput::L1 },
  { inputs[GPInput::R1]->getIndex(), GPInput::R1 },
  { inputs[GPInput::L2]->getIndex(), GPInput::L2 },
  { inputs[GPInput::L2]->getHybridAxisIndex(), GPInput::L2 },
  { inputs[GPInput::R2]->getIndex(), GPInput::L2 },
  { inputs[GPInput::R2]->getHybridAxisIndex(), GPInput::R2 },
  { inputs[GPInput::SHARE]->getIndex(), GPInput::SHARE },
  { inputs[GPInput::OPTIONS]->getIndex(), GPInput::OPTIONS },
  { inputs[GPInput::PS]->getIndex(), GPInput::PS },
  { inputs[GPInput::L3]->getIndex(), GPInput::L3 },
  { inputs[GPInput::R3]->getIndex(), GPInput::R3 },
  { inputs[GPInput::TOUCHPAD]->getIndex(), GPInput::TOUCHPAD },
  { inputs[GPInput::TOUCHPAD_ACTIVE]->getIndex(), GPInput::TOUCHPAD_ACTIVE },
  { inputs[GPInput::TOUCHPAD_ACTIVE_2]->getIndex(), GPInput::TOUCHPAD_ACTIVE_2 },
  { inputs[GPInput::LX]->getIndex(), GPInput::LX },
  { inputs[GPInput::LY]->getIndex(), GPInput::LY },
  { inputs[GPInput::RX]->getIndex(), GPInput::RX },
  { inputs[GPInput::RY]->getIndex(), GPInput::RY },
  { inputs[GPInput::DX]->getIndex(), GPInput::DX },
  { inputs[GPInput::DY]->getIndex(), GPInput::DY },
  { inputs[GPInput::ACCX]->getIndex(), GPInput::ACCX },
  { inputs[GPInput::ACCY]->getIndex(), GPInput::ACCY },
  { inputs[GPInput::ACCZ]->getIndex(), GPInput::ACCZ },
  { inputs[GPInput::GYRX]->getIndex(), GPInput::GYRX },
  { inputs[GPInput::GYRY]->getIndex(), GPInput::GYRY },
  { inputs[GPInput::GYRZ]->getIndex(), GPInput::GYRZ },
  { inputs[GPInput::TOUCHPAD_X]->getIndex(), GPInput::TOUCHPAD_X },
  { inputs[GPInput::TOUCHPAD_Y]->getIndex(), GPInput::TOUCHPAD_Y },
  { inputs[GPInput::TOUCHPAD_X_2]->getIndex(), GPInput::TOUCHPAD_X_2 },
  { inputs[GPInput::TOUCHPAD_Y_2]->getIndex(),  GPInput::TOUCHPAD_Y_2 }
};

GamepadInput::GamepadInput(GPInput gp_input, GPInputType gp_type, uint8_t id, uint8_t hybrid_id) :
  remap(gp_input, GPInput::NONE, GPInput::NONE, false, false, 0, 1),
  input_type{gp_type}, button_id{id}, hybrid_axis{hybrid_id}  {
  button_index = ((int) getButtonType() << 8) + (int) button_id;
  if (input_type == GPInputType::HYBRID) {
    hybrid_index = ((int) TYPE_AXIS << 8) + (int) hybrid_axis;
  }
}

std::shared_ptr<GamepadInput> GamepadInput::get(const std::string& name) {
  auto iter = inputNames.find(name);
  if (iter != inputNames.end()) {
    return inputs[iter->second];
  }
  return NULL;
}

std::string GamepadInput::getName(GPInput signal) {
  for (auto it = inputNames.begin(); it != inputNames.end(); it++) {
    if (it->second == signal) {
      return it->first;
    }
  }
  return "NOT FOUND";
}

std::string GamepadInput::getName() {
  return getName(remap.from_controller);
}

GPInputType GamepadInput::getType(const DeviceEvent& event) {
  assert (event.type <= TYPE_AXIS);
  GPInputType rval;
  if (event.type == TYPE_BUTTON) {
    rval = (event.id == BUTTON_L2 || event.id == BUTTON_R2) ? GPInputType::HYBRID : GPInputType::BUTTON;
  } else {
    switch (event.type) {
    case AXIS_L2:
    case AXIS_R2:
      rval = GPInputType::HYBRID;
    case AXIS_DX:
    case AXIS_DY:
      rval = GPInputType::THREE_STATE;
    case AXIS_ACCX:
    case AXIS_ACCY:
    case AXIS_ACCZ:
      rval = GPInputType::ACCELEROMETER;
    case AXIS_GYRX:
    case AXIS_GYRY:
    case AXIS_GYRZ:
      rval = GPInputType::GYROSCOPE;
    case AXIS_TOUCHPAD_X:
    case AXIS_TOUCHPAD_Y:
      rval = GPInputType::TOUCHPAD;
    default:
      rval = GPInputType::AXIS;
    }
  }
  return rval;
}

// Does the event ID match this command?
bool GamepadInput::matchesID(const DeviceEvent& event, GPInput gp) {
  // for L2/R2, we need to check both the button and the axis
  if (inputs[gp]->getType() == GPInputType::HYBRID) {
    return ((event.type == TYPE_BUTTON && event.id == inputs[gp]->getID()) ||
	    (event.type == TYPE_AXIS && event.id == inputs[gp]->getHybridAxis()));
  }
  return (event.id == inputs[gp]->getID() && event.type == inputs[gp]->getButtonType());
}

void GamepadInput::setCascadingRemap(const SignalRemap& remapping) {
  for (auto r : inputs) {
    // to-signal already remapped, so apply to Cascade
    if (r->getRemap() == remapping.to_console) {
      r->setRemap(remapping);
      return;
    }
  }
  // signal not currently remapped
  remap = remapping;
}

short int GamepadInput::getMin(ButtonType type) {
  switch (input_type) {
  case GPInputType::BUTTON:
    return 0;
  case GPInputType::THREE_STATE:
    return -1;
  case GPInputType::HYBRID:
    return (type == TYPE_AXIS) ? JOYSTICK_MIN : 0;
  case GPInputType::AXIS:
    return JOYSTICK_MIN;
  }
  // The accelerometer, gyroscope, and touchpad signals probably should never be set to the extreme,
  // but we allow it
  return SHRT_MIN;
}

short int GamepadInput::getMax(ButtonType type) {
  switch (input_type) {
  case GPInputType::BUTTON:
  case GPInputType::THREE_STATE:
    return 1;
  case GPInputType::HYBRID:
    return (type == TYPE_AXIS) ? JOYSTICK_MAX : 1;
  case GPInputType::AXIS:
    return JOYSTICK_MAX;    
  }
  // Accelerometer, gyroscope, and touchpad fall through to here
  return SHRT_MAX;
}

short int GamepadInput::getMax(std::shared_ptr<GamepadInput> signal) {
  return signal->getMax(TYPE_AXIS);
}

short int GamepadInput::getState() {
  auto& signal = inputs[remap.from_controller];
  return Controller::instance()->getState(signal);  
}