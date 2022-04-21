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
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include "config.hpp"
#include "enumerations.hpp"
#include "tomlReader.hpp"
#include "gameCommand.hpp"
#include "modifier.hpp"
#include "gameMenu.hpp"

using namespace Chaos;

// Load the TOML file into memory and do initial setup.
// Note that the logger should NOT be initialized when this is constructed. We do that
// based on parameters defined in the TOML file.
TOMLReader::TOMLReader(const std::string& fname) {
  try {
    configuration = toml::parse_file(fname);
  }
  catch (const toml::parse_error& err) {
    // log and re-throw the error
    std::cerr << "Parsing the configuration file failed:\n" << err << std::endl;
    throw err;
  }
  
  // Initialize the logger
  std::string logfile = configuration["log_file"].value_or("chaos.log");
  plog::Severity maxSeverity = (plog::Severity) configuration["log_verbosity"].value_or(3);
  size_t max_size = (size_t) configuration["max_log_size"].value_or(0);
  int max_logs = configuration["max_log_files"].value_or(8);  
  plog::init(maxSeverity, logfile.c_str(), max_size, max_logs);
  
  PLOG_NONE << "Welcome to Chaos " << CHAOS_VERSION << std::endl;

  version = configuration["chaos_toml"].value<std::string_view>();
  if (!version) {
    PLOG_FATAL << "Could not find the version id (chaos_toml). Are you sure this is the right TOML file?\n";
    throw std::runtime_error("Missing chaos_toml identifier");
  }
  
  game = configuration["game"].value<std::string_view>();
  if (!game) {
    PLOG_NONE << "Playing " << *game << std::endl;
  }
  else {
    PLOG_WARNING << "No name specified for this game.\n";
  }

  // Initialize defined sequences and static parameters for sequences
  Sequence::initialize(configuration);

  // Initialize menu items and global paramerts for the menu system
  GameMenu::instance().initialize(configuration);

  // Create the modifiers
  Modifier::buildModList(configuration);
}

void TOMLReader::buildSequence(toml::table& config, const std::string& key, Sequence& sequence) {
  PLOG_VERBOSE << "building sequence: " << config << std::endl;
  toml::array* arr = config[key].as_array();
  std::optional<std::string> event, cmd;
  unsigned int repeat, value;
  int max_val = 1;
  if (arr) {
    for (toml::node& elem : *arr) {
      assert (elem.as_table());
      toml::table& definition = *elem.as_table();

      checkValid(definition, std::vector<std::string>{"event", "command", "delay", "repeat", "value"}, "buildSequence");
      
      event = definition["event"].value<std::string>();
      if (! event) {
	      throw std::runtime_error("Sequence missing required 'event' parameter");
      }

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

      cmd = definition["command"].value<std::string>();
      if (cmd) {
	      signal = GamepadInput::get(*cmd);
	      // If this signal is a hybrid control, this gets the axis max, which is needed for addHold().
        if (signal) {
	        max_val = GamepadInput::getMax(signal);
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
      } else {
      	throw std::runtime_error("Unrecognized event type.");
      }
    }
  }
}


GPInput TOMLReader::getSignal(const toml::table& config, const std::string& key, bool required) {
  if (! config.contains(key)) {
    if (required) {
      throw std::runtime_error("missing required '" + key + "' field");
    }
    return GPInput::NONE;
  }
  std::optional<std::string> signal = config.get(key)->value<std::string>();
  std::shared_ptr<GamepadInput> inp = GamepadInput::get(*signal);
  if (! inp) {
    throw std::runtime_error("Gamepad signal '" + *signal + "' not defined");
  }
  return inp->getSignal();
}

ConditionCheck TOMLReader::getConditionTest(const toml::table& config, const std::string& key) {
  std::optional<std::string_view> ttype = config[key].value<std::string_view>();

  // Default type is magnitude
  ConditionCheck rval = ConditionCheck::ANY;
  if (ttype) {
    if (*ttype == "any") {
      rval = ConditionCheck::ANY;
    } else if (*ttype == "none") {
      rval = ConditionCheck::NONE;
    } else if (*ttype != "all") {
      PLOG_WARNING << "Invalid ConditionTest '" << *ttype << "': using 'all' instead." << std::endl;
    }
  }
  return rval;
}

// warn if any keys in the table are unknown
bool TOMLReader::checkValid(const toml::table& config, const std::vector<std::string>& goodKeys) {
  return checkValid(config, goodKeys, config["name"].value_or("??"));
}

bool TOMLReader::checkValid(const toml::table& config, const std::vector<std::string>& goodKeys,
			    const std::string& name) {
  bool ret = true;
  for (auto&& [k, v] : config) {
    if (std::find(goodKeys.begin(), goodKeys.end(), k) == goodKeys.end()) {
      PLOG_WARNING << "The key '" << k << "' is unused in " << name << std::endl;
      ret = false;
    }
  }
  return ret;
}

std::string_view TOMLReader::getGameName() {
  return (game) ? *game : "UNKNOWN";
}

std::string_view TOMLReader::getVersion() {
  return (version) ? *version : "UNKNOWN";
}

