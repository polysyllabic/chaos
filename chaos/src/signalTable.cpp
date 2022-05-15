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
#include <algorithm>
#include "signalTable.hpp"
#include "controllerInput.hpp"
#include "gameCondition.hpp"
#include "tomlUtils.hpp"

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

// The constructor handles the initialization of hard-coded values
SignalTable::SignalTable() {
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



std::shared_ptr<ControllerInput> SignalTable::getInput(const std::string& name) {
  auto iter = by_name.find(name);
  if (iter != by_name.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<ControllerInput> SignalTable::getInput(ControllerSignal signal) { 
  return inputs[signal];
}

std::shared_ptr<ControllerInput> SignalTable::getInput(const DeviceEvent& event) {  
  auto iter = by_index.find(event.index());
  if (iter != by_index.end()) {
    return iter->second;
  }
  return nullptr;
}

// Does the event ID match this command?
bool SignalTable::matchesID(const DeviceEvent& event, ControllerSignal to) {
  std::shared_ptr<ControllerInput> to_match = getInput(to);
  assert(to);
  // for L2/R2, we need to check both the button and the axis
  if (to_match->getType() == ControllerSignalType::HYBRID) {
    return ((event.type == TYPE_BUTTON && event.id == to_match->getID()) ||
	    (event.type == TYPE_AXIS && event.id == to_match->getHybridAxis()));
  }
  return (event.id == to_match->getID() && event.type == to_match->getButtonType());
}

short SignalTable::touchpadToAxis(ControllerSignal tp_axis, short value) {
  short ret;
  // Use the touchpad value to update the running derivative count
  short derivativeValue = touchpad.getVelocity(tp_axis, value) *
      (touchpad_condition->inCondition()) ? touchpad_scale_if : touchpad_scale;

  if (derivativeValue > 0) {
    ret = derivativeValue + touchpad_skew;
  }
  else if (derivativeValue < 0) {
    ret = derivativeValue - touchpad_skew;
  }
    
  PLOG_DEBUG << "Derivative: " << derivativeValue << ", skew = " << touchpad_skew << ", returning " << ret;
  return ret;
}

std::shared_ptr<ControllerInput> SignalTable::getInput(const toml::table& config, const std::string& key) {
  std::optional<std::string> signal = config[key].value<std::string>();
  if (!signal) {
    throw std::runtime_error("Remap item Missing '" + key + "' field");
  }
  std::shared_ptr<ControllerInput> inp = getInput(*signal);
  if (!inp) {
    throw std::runtime_error("Controller signal '" + *signal + "' not defined");
  }
  return inp;
}

void SignalTable::setCascadingRemap(std::unordered_map<std::shared_ptr<ControllerInput>, SignalRemap>& remaps) {
  for (auto& r : remaps) {
    // Check if the from signal in the remaps list already appears as a to_console entry in the table
    auto it = std::find_if(inputs.begin(), inputs.end(), [r](const auto &rtable)
      {
        return rtable.first.getRemap() == r.first;
      });
    if (it == inputs.end()) {
      // The signal isn't currently being remapped.
      std::shared_ptr<ControllerInput> source = r.first;
      r.first->setRemap(r.second);
    } else {
      // To-signal already remapped. Apply the remap to the signal whose to_console value is currently
      // the from value we're trying to remap
      it->second->setRemap(r.second);
    }
  }
}

void SignalTable::clearRemaps() {
  for (auto [from, remapping] : inputs) {
    remapping->setRemap({nullptr, nullptr, false, false, 0, 1});
  }
}