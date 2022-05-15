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
#include "tomlUtils.hpp"
#include "menuItem.hpp"
#include "controller.hpp"
#include "controllerInput.hpp"
#include "gameCommand.hpp"
#include "game.hpp"

using namespace Chaos;

Sequence::Sequence(Controller& c) : controller{c} {}

Sequence::Sequence(toml::table& config, const std::string& key, Game& game) {
  toml::array* arr = config[key].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      assert (elem.as_table());
      toml::table& definition = *elem.as_table();

      TOMLUtils::checkValid(definition, std::vector<std::string>{"event", "command", "delay", "repeat", "value"}, "buildSequence");
      
      std::optional<std::string> event = definition["event"].value<std::string>();
      if (! event) {
	      throw std::runtime_error("Sequence missing required 'event' parameter");
      }

      double delay = definition["delay"].value_or(0.0);
      if (delay < 0) {
	      throw std::runtime_error("Delay must be a non-negative number of seconds.");
      }      
      unsigned int delay_time = (unsigned int) (delay * 1000000);
      int repeat = definition["repeat"].value_or(1);
      if (repeat < 1) {
	      PLOG_WARNING << "The value of 'repeat' should be an integer greater than 1. Will ignore.";
	      repeat = 1;
      }

      // Sequences are stored in terms of ControllerInput signals, but the TOML file uses command
      // names. We use the default mapping because signals are always sent out in the form the
      // console expects.
      std::optional<std::string> cmd = definition["command"].value<std::string>();

      // Append a predefined sequence
      if (*event == "sequence") {
        if (cmd) {
          std::shared_ptr<Sequence> new_seq = game.getSequence(*cmd);
          if (new_seq) {
	          throw std::runtime_error("Undefined sequence: " + *cmd);
          }
          addSequence(new_seq);
        }
      } else {
        std::shared_ptr<GameCommand> command = (cmd) ? game.getCommand(*cmd) : nullptr;
        std::shared_ptr<ControllerInput> signal = (command) ? command->getInput() : nullptr;

	      // If this signal is a hybrid control, this gets the axis max, which is needed for addHold
	      int max_val = (signal) ? signal->getMax(TYPE_AXIS) : 0;
        int value = definition["value"].value_or(max_val);

        if (*event == "delay") {
	        if (delay_time == 0) {
	          PLOG_WARNING << "You've tried to add a delay of 0 microseconds. This will be ignored.";
	        } else {
	          addDelay(delay_time);
            PLOG_VERBOSE << "Delay = " << delay_time << " microseconds";
	        }
        } else if (*event == "hold") {
	        if (!signal) {
	          throw std::runtime_error("Undefined hold command: " + *cmd);
      	  }
	        if (repeat > 1) {
	          PLOG_WARNING << "Repeat is not supported with 'hold' and will be ignored.";
      	  }
	        addHold(signal, value, 0);
          PLOG_VERBOSE << "Hold " << signal->getName();
        } else if (*event == "press") {
	        if (!signal) {
	          throw std::runtime_error("Undefined press command: " + *cmd);
          }
	        for (int i = 0; i < repeat; i++) {
	          addPress(signal, value);
	          if (delay_time > 0) {
	            addDelay(delay_time);
	          }
            PLOG_VERBOSE << "Press " << signal->getName() << " with delay of " << delay_time << " microseconds.";
	        }
        } else if (*event == "release") {
	        if (!signal) {
	          throw std::runtime_error("Undefined release command: " + *cmd);
	        }
	        if (repeat > 1) {
	          PLOG_WARNING << "Repeat is not supported with 'release' and will be ignored.";
	        }
	        addRelease(signal, delay_time);
          PLOG_VERBOSE << "Release " << signal->getName() << " with delay of " << delay_time << " microseconds.";
        } else {
      	  throw std::runtime_error("Unrecognized event type: " + *event);
        }
      }
    }
  }
}

void Sequence::addEvent(DeviceEvent event) {
  events.push_back(event);
}

void Sequence::addSequence(std::shared_ptr<Sequence> seq) {
  assert (seq);
  events.insert(events.end(), seq->getEvents().begin(), seq->getEvents().end());
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
  
  events.push_back({hold_time, value, (uint8_t) signal->getButtonType(), signal->getID()});
  if (signal->getType() == ControllerSignalType::HYBRID) {
    events.push_back( {hold_time, hybrid_value, TYPE_AXIS, signal->getHybridAxis()} );
  }
}

void Sequence::addRelease(std::shared_ptr<ControllerInput> signal, unsigned int release_time) {
  assert(signal);

  events.push_back({release_time, 0, (uint8_t) signal->getButtonType(), signal->getID()});
  if (signal->getType() == ControllerSignalType::HYBRID) {
    events.push_back( {release_time, JOYSTICK_MIN, TYPE_AXIS, signal->getHybridAxis()} );
  }
}

void Sequence::addDelay(unsigned int delay) {
  events.push_back( {delay, 0, 255, 255} );
}


void Sequence::send() {
  for (std::vector<DeviceEvent>::iterator it = events.begin(); it != events.end(); it++) {
    DeviceEvent& event = (*it);
    PLOG_VERBOSE << "Sending event for button " << (int) event.type << ": " << (int) event.id
	       << ":" << (int) event.value << "; sleeping for " << (int) event.time << " microseconds";
    controller.applyEvent(event);
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
      controller.applyEvent(e);
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



