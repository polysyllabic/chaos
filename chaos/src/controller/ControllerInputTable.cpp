/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021, 2025 The Twitch Controls Chaos developers. See the AUTHORS
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
#include <TOMLUtils.hpp>

#include "ControllerInputTable.hpp"
#include "ControllerInput.hpp"

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
ControllerInputTable::ControllerInputTable(Controller& c) {
  for (SignalSettings s : signal_settings) {
    PLOG_VERBOSE << "Initializing signal " << s.name;
    std::shared_ptr<ControllerInput> sig = std::make_shared<ControllerInput>(c, s);
    inputs.insert({s.input, sig});
    by_name.insert({s.name, sig});
    by_index.insert({sig->getIndex(), sig});
    if (s.type == ControllerSignalType::HYBRID) {
      by_index.insert({sig->getHybridAxisIndex(), sig});
    }
  }
}

std::shared_ptr<ControllerInput> ControllerInputTable::getInput(const std::string& name) {
  auto iter = by_name.find(name);
  if (iter != by_name.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<ControllerInput> ControllerInputTable::getInput(ControllerSignal signal) { 
  return inputs[signal];
}

std::shared_ptr<ControllerInput> ControllerInputTable::getInput(const DeviceEvent& event) {  
  auto iter = by_index.find(event.index());
  if (iter != by_index.end()) {
    return iter->second;
  }
  return nullptr;
}

// Does the event ID match this command?
bool ControllerInputTable::matchesID(const DeviceEvent& event, ControllerSignal to) {
  std::shared_ptr<ControllerInput> to_match = getInput(to);
  assert(to);
  // for L2/R2, we need to check both the button and the axis
  if (to_match->getType() == ControllerSignalType::HYBRID) {
    return ((event.type == TYPE_BUTTON && event.id == to_match->getID()) ||
	    (event.type == TYPE_AXIS && event.id == to_match->getHybridAxis()));
  }
  return (event.id == to_match->getID() && event.type == to_match->getButtonType());
}

std::shared_ptr<ControllerInput> ControllerInputTable::getInput(const toml::table& config, const std::string& key) {
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

// This is the game-specific initialization
int ControllerInputTable::initializeInputs(const toml::table& config) {
  int errors = 0;

  bool use_velocity = config["remapping"]["touchpad_velosity"].value_or(false);
  Touchpad::setVelocity(use_velocity);

  double scale = config["remapping"]["touchpad_velocity_scale"].value_or(1.0);
  if (scale == 0) {
    PLOG_ERROR << "Touchpad velocity scale cannot be 0. Setting to 1";
    ++errors;
    scale = 1;
  }
  Touchpad::setVelocityScale(scale);

  short skew = config["remapping"]["touchpad_skew"].value_or(0);
  Touchpad::setSkew(skew);

  PLOG_VERBOSE << "Touchpad velocity scale = " << scale << "; skew = " << skew;

  if (! use_velocity) {
    double scale_x = config["remapping"]["touchpad_scale_x"].value_or(1.0);
    if (scale_x == 0) {
      PLOG_ERROR << "Touchpad scale_x cannot be 0. Setting to 1";
      ++errors;
      scale_x = 1;
    }
    double scale_y = config["remapping"]["touchpad_scale_y"].value_or(1.0);
    if (scale_y == 0) {
      PLOG_ERROR << "Touchpad scale_y cannot be 0. Setting to 1";
      ++errors;
      scale_y = 1;
    }
    Touchpad::setScaleXY(scale_x, scale_y);
  }

  return errors;
}

void ControllerInputTable::addToVector(const toml::table& config, const std::string& key,
                                   std::vector<std::shared_ptr<ControllerInput>>& vec) {

  PLOG_DEBUG << "Adding " << key;

  if (config.contains(key)) {
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error(key + " must be an array of strings");
   	}
	
    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
   	  std::shared_ptr<ControllerInput> item = getInput(*cmd);
      if (item) {
        vec.push_back(item);
        PLOG_VERBOSE << "Added '" + *cmd + "' to the " + key + " vector.";
      } else {
        throw std::runtime_error("Unrecognized controller input: " + *cmd + " in " + key);
     	}
    }
  } else {
    PLOG_DEBUG << "No " << key << " array to add";
  }
}
