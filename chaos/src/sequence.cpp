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
#include <unistd.h>
#include <plog/Log.h>

#include "sequence.hpp"
#include "configuration.hpp"
#include "menuItem.hpp"

using namespace Chaos;

std::unordered_map<std::string, std::shared_ptr<Sequence>> Sequence::sequences;
unsigned int Sequence::press_time;
unsigned int Sequence::release_time;

void Sequence::initialize(toml::table& config) {
  // global parameters for sequences
  press_time = (unsigned int) (config["controller"]["button_press_time"].value_or(0.0625) * 1000000);
  PLOG_VERBOSE << "press_time = " << press_time << " microseconds";

  release_time = (unsigned int) (config["controller"]["button_release_time"].value_or(0.0625) * 1000000);
  PLOG_VERBOSE << "release_time = " << release_time << " microseconds";

  // initialize pre-defined sequences
  toml::array* arr = config["sequence"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      toml::table* seq = elem.as_table();
      if (!seq) {
        PLOG_ERROR << "Each sequence definition must be a table";
        continue;
      }
      std::optional<std::string> seq_name = (*seq)["name"].value<std::string>();
      if (! seq_name) {
        PLOG_ERROR << "Sequence definition missing required name field";
        continue;
      }
      if (! seq->contains("sequence")) {
        PLOG_ERROR << "Sequence definition missing required sequence field";
        continue;
      }
      try {
        PLOG_VERBOSE << "Adding pre-defined sequence '" << *seq_name << "'";
        std::shared_ptr<Sequence> s = std::make_shared<Sequence>();
        Configuration::buildSequence(*seq, "sequence", *s);
       	sequences.insert({*seq_name, s});
      }
      catch (const std::runtime_error& e) {
        PLOG_ERROR << "In definition for Sequence '" << *seq_name << "': " << e.what(); 
      }
    }
  } else {
    PLOG_WARNING << "No pre-defined sequences found.";
  }
}

void Sequence::disableControls() {
  events.push_back( {0, 0, TYPE_AXIS, ControllerInput::get(ControllerSignal::DX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, ControllerInput::get(ControllerSignal::DY)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, ControllerInput::get(ControllerSignal::RX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, ControllerInput::get(ControllerSignal::RY)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, ControllerInput::get(ControllerSignal::LX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, ControllerInput::get(ControllerSignal::LY)->getID()} );
  events.push_back( {0, -128, TYPE_AXIS, ControllerInput::get(ControllerSignal::L2)->getHybridAxis()} );
  events.push_back( {0, -128, TYPE_AXIS, ControllerInput::get(ControllerSignal::R2)->getHybridAxis()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::X)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::SQUARE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::TRIANGLE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::CIRCLE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::R1)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::L1)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::R2)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::L2)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::R3)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, ControllerInput::get(ControllerSignal::L3)->getID()} );
}

void Sequence::addPress(std::shared_ptr<ControllerInput> signal, short value) {
  addHold(signal, value, press_time);
  addRelease(signal, release_time);
}

void Sequence::addHold(std::shared_ptr<ControllerInput> signal, short value, unsigned int hold_time) {
  assert(signal);
  short int hybrid_value;

  // If a value is passed for a hybrid signal (L2/R2), it applies to the axis signal. The button
  // signal will just use 1.
  if (signal->getType() == ControllerSignalType::HYBRID) {
    hybrid_value = (value == 0) ? JOYSTICK_MAX : value;
    value = 1;
  } else if (value == 0) {
    // For other types, if value not set, we use the maximum value for that signal type.
    value = (signal->getType() == ControllerSignalType::BUTTON ||
	     signal->getType() == ControllerSignalType::THREE_STATE) ? 1 : JOYSTICK_MAX;
  }
  
  events.push_back( {hold_time, value, signal->getButtonType(), signal->getID()} );
  if (signal->getType() == ControllerSignalType::HYBRID) {
    events.push_back( {hold_time, hybrid_value, TYPE_AXIS, signal->getHybridAxis()} );
  }
}

void Sequence::addRelease(std::shared_ptr<ControllerInput> signal, unsigned int release_time) {
  assert(signal);

  events.push_back( {release_time, 0, signal->getButtonType(), signal->getID()} );
  if (signal->getType() == ControllerSignalType::HYBRID) {
    events.push_back( {release_time, JOYSTICK_MIN, TYPE_AXIS, signal->getHybridAxis()} );
  }
}

void Sequence::addDelay(unsigned int delay) {
  events.push_back( {delay, 0, 255, 255} );
}

void Sequence::addSequence(const std::string& name) {
  std::shared_ptr<Sequence> seq = get(name);
  if (seq) {
    addSequence(seq);
  } else {
    PLOG_ERROR << "Attempted to add undefined sequence '" << name;
  }
}

void Sequence::addSequence(std::shared_ptr<Sequence> seq) {
  assert (seq);
  events.insert(events.end(), seq->getEvents().begin(), seq->getEvents().end());
}

void Sequence::send() {
  for (std::vector<DeviceEvent>::iterator it = events.begin(); it != events.end(); it++) {
    DeviceEvent& event = (*it);
    PLOG_VERBOSE << "Sending event for button " << (int) event.type << ": " << (int) event.id
	       << ":" << (int) event.value << "; sleeping for " << (int) event.time << " microseconds";
    Controller::instance().applyEvent(event);
    if (event.time) {
      usleep(event.time);
    }
  }
}

bool Sequence::sendParallel(double sequenceTime) {
  unsigned int elapsed = (unsigned int) sequenceTime * 1000000;
  for (DeviceEvent& e = events[current_step]; current_step <= events.size(); e = events[++current_step]) {
    PLOG_VERBOSE << "parallel sent step " << current_step;
    if (e.isDelay()) {
      wait_until += e.time;
      // return until delay expires, then move to the next step
      if (elapsed < wait_until) {
        return false;
      }
    } else {
      // send out events until we hit the next delay
      Controller::instance().applyEvent(e);
    }
  }
  // We're at the end of the sequence. Reset the steps and time for the next iteration
  current_step = 0;
  wait_until = 0;
  return true;
}

void Sequence::clear() {
    events.clear();
}

bool Sequence::empty() {
  return events.empty();
}

std::shared_ptr<Sequence> Sequence::get(const std::string& name) {
  auto iter = sequences.find(name);
  if (iter != sequences.end()) {
    return iter->second;
  }
  return nullptr;
}
