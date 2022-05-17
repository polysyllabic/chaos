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
#include <plog/Log.h>

#include "config.hpp"
#include "SequenceTable.hpp"
#include "ControllerInput.hpp"
#include "GameCommandTable.hpp"
#include "GameCommand.hpp"
#include "TOMLUtils.hpp"

using namespace Chaos;

int SequenceTable::buildSequenceList(toml::table& config, GameCommandTable& commands,
                                     Controller& controller) {
  int parse_errors = 0;
  // global parameters for sequences
  Sequence::setPressTime(config["controller"]["button_press_time"].value_or(0.0625));
  Sequence::setReleaseTime(config["controller"]["button_release_time"].value_or(0.0625));

  if (sequence_map.size() > 0) {
    PLOG_VERBOSE << "Clearing existing Sequence data";
    sequence_map.clear();
  }

  // Initialize pre-defined sequences defined in the TOML file
  toml::array* arr = config["sequence"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      toml::table* seq = elem.as_table();
      if (!seq) {
        ++parse_errors;
        PLOG_ERROR << "Each sequence definition must be a table";
        continue;
      }
      std::optional<std::string> seq_name = (*seq)["name"].value<std::string>();
      if (! seq_name) {
        ++parse_errors;
        PLOG_ERROR << "Sequence definition missing required 'name' field";
        continue;
      }

      toml::array* event_list = config["sequence"].as_array();
      if (! event_list) {
        ++parse_errors;
        PLOG_ERROR << "Sequence definition missing required 'sequence' field";
        continue;
      }
      try {
        std::shared_ptr<Sequence> s = makeSequence(*seq, "sequence", commands, controller, true);
        PLOG_VERBOSE << "Adding pre-defined sequence: " << *seq_name;
       	auto [it, result] = sequence_map.try_emplace(*seq_name, s);
        if (! result) {
          ++parse_errors;
          PLOG_ERROR << "Duplicate sequence ignored: " << *seq_name;
        }
      }
      catch (const std::runtime_error& e) {
        ++parse_errors;
        PLOG_ERROR << "In definition for Sequence '" << *seq_name << "': " << e.what(); 
      }
    }
  } else {
    PLOG_WARNING << "No pre-defined sequences found.";
  }
  return parse_errors;
}

std::shared_ptr<Sequence> SequenceTable::makeSequence(toml::table& config, 
                                                      const std::string& key,
                                                      GameCommandTable& commands,
                                                      Controller& controller,
                                                      bool required) {
  std::shared_ptr<Sequence> seq{nullptr};
  toml::array* event_list = config[key].as_array();

  if (! event_list) {
    if (required) {
      throw std::runtime_error("Missing required '" + key + "' key");
    }
    return seq;
  }

  for (toml::node& elem : *event_list) {
    toml::table definition = *elem.as_table();
    TOMLUtils::checkValid(definition, std::vector<std::string>{"event", "command", "delay",
                          "repeat", "value"}, "makeSequence");
      
    std::optional<std::string> event = definition["event"].value<std::string>();
    if (! event) {
	    throw std::runtime_error("Sequence missing required 'event' parameter");
    }

    double delay = definition["delay"].value_or(0.0);
    if (delay < 0) {
	    throw std::runtime_error("Delay must be a non-negative number of seconds.");
    }
    unsigned int delay_time = (unsigned int) (delay * SEC_TO_MICROSEC);

    int repeat = definition["repeat"].value_or(1);
    if (repeat < 1) {
	    PLOG_WARNING << "The value of 'repeat' should be an integer greater than 1. Will ignore.";
	    repeat = 1;
    }

    if (*event == "delay") {
      if (delay_time == 0) {
        PLOG_WARNING << "You've tried to add a delay of 0 microseconds. This will be ignored.";
	    } else {
	      seq->addDelay(delay_time);
        PLOG_VERBOSE << "Delay = " << delay_time << " microseconds";
	    }
      continue;
    }

    // Sequences are stored in terms of ControllerInput signals, but the TOML file uses command
    // names. We use the default mapping because signals are always sent out in the form the
    // console expects.
    std::optional<std::string> cmd = definition["command"].value<std::string>();

    // Append a predefined sequence
    if (*event == "sequence") {
      if (cmd) {
        std::shared_ptr<Sequence> new_seq = getSequence(*cmd);
        if (new_seq) {
	        throw std::runtime_error("Undefined sequence: " + *cmd);
        }
        seq->addSequence(new_seq);
      }
    } else {
      // The remaining events all require a defined command.
      std::shared_ptr<GameCommand> command = (cmd) ? commands.getCommand(*cmd) : nullptr;
      if (!command) {
        throw std::runtime_error("Undefined command: " + *cmd);
   	  }
      std::shared_ptr<ControllerInput> signal = command->getInput();

	    // If this signal is a hybrid control, this gets the axis max, which is needed for addHold
	    int max_val = (signal) ? signal->getMax(TYPE_AXIS) : 0;
      int value = definition["value"].value_or(max_val);

      if (*event == "hold") {
	      if (repeat > 1) {
	        PLOG_WARNING << "Repeat is not supported with 'hold' and will be ignored.";
    	  }
	      seq->addHold(signal, value, 0);
        PLOG_VERBOSE << "Hold " << signal->getName();
      } else if (*event == "press") {
	      for (int i = 0; i < repeat; i++) {
	        seq->addPress(signal, value);
	        if (delay_time > 0) {
	          seq->addDelay(delay_time);
            PLOG_VERBOSE << "Press " << signal->getName() << " with a delay of " << delay_time << " microseconds";
	        } else {
            PLOG_VERBOSE << "Press " << signal->getName();
          }
	      }
      } else if (*event == "release") {
	      if (repeat > 1) {
	        PLOG_WARNING << "Repeat is not supported with 'release' and will be ignored.";
	      }
	      seq->addRelease(signal, delay_time);
        PLOG_VERBOSE << "Release " << signal->getName() << " (delay =" << delay_time << " microseconds)";
      } else {
    	  throw std::runtime_error("Unrecognized event type: " + *event);
      }
    }
  }
  return seq;
}

std::shared_ptr<Sequence> SequenceTable::getSequence(const std::string& name) {
  auto iter = sequence_map.find(name);
  if (iter != sequence_map.end()) {
    return iter->second;
  }
  return nullptr;
}

void SequenceTable::addSequence(Sequence& seq, const std::string& name) {
  std::shared_ptr<Sequence> new_sequence = getSequence(name);
  if (new_sequence) {
    seq.addSequence(new_sequence);
  }
}
