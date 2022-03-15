/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
//#include "chaosEngine.hpp"

using namespace Chaos;

unsigned int Sequence::press_time;
unsigned int Sequence::release_time;

// A constructor to
Sequence::Sequence(toml::aray& config) {
  // expect an array of inline tables
  for (toml::node& elem : *arr) {
    assert (elem.as_table());
    toml::table& definition = *elem.as_table();

    checkValid(definition, std::vector<std::string>{"event", "command", "delay", "repeat", "value"}, "Sequence::Sequence");
      
    std::optional<std::string> event = definition["event"].value<std::string>();
    if (! event) {
     throw std::runtime_error("Sequence missing required 'event' parameter");
    }

    std::optional<std::string> cmd = definition["command"].value<std::string>();

    double delay = definition["delay"].value_or(0.0);
    if (delay < 0) {
     throw std::runtime_error("Delay must be a non-negative number of seconds.");
    }
    short delay_time = (short) delay*1000000;
      
    repeat = definition["repeat"].value_or(1);
    if (repeat < 1) {
      PLOG_ERROR << "The value of 'repeat' should be an integer greater than 1. Will ignore.";
      repeat = 1;
    }

    // CHECK: should value really be unsigned?
    value = definition["value"].value_or(max_val);

    std::shared_ptr<GamepadInput> signal;
    std::shared_ptr<MenuItem> menu_command;
  
    if (cmd) {
      if (*event == "set menu" || *event == "restore menu") {
        menu_command = MenuItem::get(*cmd);
        if (! menu_command) {
          throw std::runtime_error("Unrecognized menu command: " + *cmd);
        }
      } else {
        signal = GamepadInput::get(*cmd);
	      // If this signal is a hybrid control, this gets the axis max, which is needed for addHold().
        if (signal) {
	        max_val = GamepadInput::getMax(signal);
        }
      } 
    }

    PLOG_DEBUG << " - sequence event: ";
    if (*event == "delay") {
	    if (delay_time == 0) {
	      PLOG_ERROR << "You've tried to add a delay of 0 microseconds. This will be ignored.\n";
	    } else {
	      sequence.addDelay(delay_time);
        PLOG_DEBUG << " delay " << delay_time << " microseconds\n";
	    }
    } else if (*event == "disable") {
	    sequence.disableControls();
    } else if (*event == "hold") {
	    if (!signal) {
	      throw std::runtime_error("A 'hold' event must be associated with a valid command.");
    	}
	    if (repeat > 1) {
	      PLOG_WARNING << "Repeat is not supported with 'hold' and will be ignored. (Did you want 'press'?)\n";
    	}
	    sequence.addHold(signal, value, 0);
      PLOG_DEBUG << " hold " << signal->getName() << std::endl;
    } else if (*event == "press") {
	    if (!signal) {
	      throw std::runtime_error("A 'press' event must be associated with a valid command.");
	    }
	    for (int i = 0; i < repeat; i++) {
	      sequence.addPress(signal, value);
	      if (delay_time > 0) {
	        sequence.addDelay(delay_time);
	      }
        PLOG_DEBUG << " press " << signal->getName() << "with delay of " << delay_time << " microseconds." << std::endl;
	    }
    } else if (*event == "release") {
	    if (!signal) {
	      throw std::runtime_error("A 'release' event must be associated with a valid command.");
	    }
	    if (repeat > 1) {
	      PLOG_WARNING << "Repeat is not supported with 'release' and will be ignored. (Did you want 'press'?)\n";
	    }
	    sequence.addRelease(signal, delay_time);
      PLOG_DEBUG << " release " << signal->getName() << "with delay of " << delay_time << " microseconds." << std::endl;
    } else if (*event == "set menu") {
      if (! menu_command->isOption()) {
        PLOG_ERROR << "Event 'set menu' invokes intermediate menu '" << *cmd << "' and not a menu option.\n";
      } else {
        addMenuSelect(menu_item, value);
      }
    } else if (*event == "restore menu") {
      addMenuRestore(menu_item);
    } else {
    	throw std::runtime_error("Unrecognized event type.");
    }

}

void Sequence::initialize(toml::table& config) {
  // global parameters for sequences
  press_time = (unsigned int) config["button_press_time"].value_or(0.0625) * 1000000;
  release_time = (unsigned int) config["button_release_time"].value_or(0.0625) * 1000000;

  // initialize pre-defined sequences
  toml::array* arr = config["sequence"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      toml::table* seq = elem.as_table();
        if (seq) {
          if (! seq->contains("name")) {
          PLOG_ERROR << "Sequence definition missing required name field: " << *seq << std::endl;
          continue;
        }
        std::string seq_name = seq->get("name")->value_or("");
        if (! seq->contains("sequence")) {
          PLOG_ERROR << "Sequence definition missing required sequence field: " << *seq << std::endl;
          continue;
        }
        toml::array* s = seq->get("sequence")->as_array();
        try {
          PLOG_VERBOSE << "Adding pre-defined sequence '" << seq_name << "'" << std::endl;
       	  sequences.insert({seq_name, std::make_shared<Sequence>(*s)});
        }
        catch (const std::runtime_error& e) {
          PLOG_ERROR << "In definition for Sequence '" << seq_name << "': " << e.what() << std::endl; 
        }
      } else {
        PLOG_ERROR << "Each sequence definition must be a table.\n";
      }
    }
  } else {
    PLOG_WARNING << "No sequences are defined." << std::endl;
  }
}

void Sequence::disableControls() {
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::DX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::DY)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::RX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::RY)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::LX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::LY)->getID()} );
  events.push_back( {0, -128, TYPE_AXIS, GamepadInput::get(GPInput::L2)->getHybridAxis()} );
  events.push_back( {0, -128, TYPE_AXIS, GamepadInput::get(GPInput::R2)->getHybridAxis()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::X)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::SQUARE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::TRIANGLE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::CIRCLE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::R1)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::L1)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::R2)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::L2)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::R3)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::L3)->getID()} );
  events.push_back( {DELAY_AFTER_CONTROL_DISABLE, 0, TYPE_AXIS, GamepadInput::get(GPInput::LY)->getID()} );
}

void Sequence::addPress(std::shared_ptr<GamepadInput> signal, short value) {
  addHold(signal, value, TIME_PER_BUTTON_RELEASE);
  addRelease(signal, TIME_PER_BUTTON_RELEASE);
}

void Sequence::addHold(std::shared_ptr<GamepadInput> signal, short value, unsigned int hold_time) {
  assert(signal);
  short int hybrid_value;

  // If a value is passed for a hybrid signal (L2/R2), it applies to the axis signal. The button
  // signal will just use 1.
  if (signal->getType() == GPInputType::HYBRID) {
    hybrid_value = (value == 0) ? JOYSTICK_MAX : value;
    value = 1;
  } else if (value == 0) {
    // For other types, if value not set, we use the maximum value for that signal type.
    value = (signal->getType() == GPInputType::BUTTON ||
	     signal->getType() == GPInputType::THREE_STATE) ? 1 : JOYSTICK_MAX;
  }
  
  events.push_back( {hold_time, value, signal->getButtonType(), signal->getID()} );
  if (signal->getType() == GPInputType::HYBRID) {
    events.push_back( {hold_time, hybrid_value, TYPE_AXIS, signal->getHybridAxis()} );
  }
}

void Sequence::addRelease(std::shared_ptr<GamepadInput> signal, unsigned int release_time) {
  assert(signal);

  events.push_back( {release_time, 0, signal->getButtonType(), signal->getID()} );
  if (signal->getType() == GPInputType::HYBRID) {
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
    PLOG_ERROR << "Attempted to add undefined sequence '" << name << "'\n";
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
	       << ":" << (int) event.value << "\t sleeping for " << event.time << '\n';
    Controller::instance().applyEvent( event );
    if (event.time) {
      usleep(event.time);
    }
  }
}

DeviceEvent Sequence::getEvent(double sequenceTime) {
  unsigned int last_time = 0;
  unsigned int elapsed = (unsigned int) sequenceTime * 1000000;
  for (auto& seq : events) {
    if (elapsed >= last_time && elapsed < last_time + seq.time) {
      return seq;
    }
  }
  return {0, 0, 255, 255};
}

void Sequence::clear() {
    events.clear();
}

bool Sequence::empty() {
  return events.empty();
}
