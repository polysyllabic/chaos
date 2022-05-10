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
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include "config.hpp"
#include "enumerations.hpp"
#include "configuration.hpp"
#include "gameCommand.hpp"
#include "modifier.hpp"
#include "gameMenu.hpp"

using namespace Chaos;

// Parse the TOML file into memory and do initial setup. All initializations done from
// the configuration file should be dispatched from here.
Configuration::Configuration(const std::string& fname) {
  toml::table configuration;
  try {
    configuration = toml::parse_file(fname);
  }
  catch (const toml::parse_error& err) {
    std::cerr << "Parsing the configuration file failed:" << err << std::endl;
    throw err;
  }
  // to do: handle case of non-existent directory
  // Initialize the logger
  std::string logfile = configuration["logging"]["log_file"].value_or("chaos.log");  
  plog::Severity maxSeverity = (plog::Severity) configuration["logging"]["log_verbosity"].value_or(3);
  size_t max_size = (size_t) configuration["logging"]["max_log_size"].value_or(0);
  int max_logs = configuration["logging"]["max_log_files"].value_or(8);  
  plog::init(maxSeverity, logfile.c_str(), max_size, max_logs);
  
  PLOG_NONE << "Welcome to Chaos " << CHAOS_VERSION;

  // Check version tag to make sure we're looking at a chaos TOML file
  std::optional<std::string_view> version = configuration["chaos_toml"].value<std::string_view>();
  if (!version) {
    PLOG_FATAL << "Could not find the version id (chaos_toml). Are you sure this is the right TOML file?";
    throw std::runtime_error("Missing chaos_toml identifier");
  }
  
  // Get the basic game information. We send these to the interface. 
  game = configuration["game"].value_or<std::string>("Unknown game");
  PLOG_INFO << "Playing " << game;

  active_modifiers = configuration["mod_defaults"]["active_modifiers"].value_or(3);
  if (active_modifiers < 1) {
    PLOG_ERROR << "You asked for " << active_modifiers << ". There must be at least one.";
    active_modifiers = 1;
  } else if (active_modifiers > 5) {
    PLOG_WARNING << "Having too many active modifiers may cause undesirable side-effects.";
  }
  PLOG_INFO << "Active modifiers: " << active_modifiers;

  time_per_modifier = configuration["mod_defaults"]["time_per_modifier"].value_or(180.0);
  if (time_per_modifier < 10) {
    PLOG_ERROR << "Modifiers should be active for at least 10 seconds.";
    time_per_modifier = 10;
  }
  PLOG_INFO << "Time per modifier: " << time_per_modifier << " seconds";

  // Initialize basic controller input signals
  ControllerInput::initialize(configuration);

  // Process those commands that do not have conditions
  GameCommand::buildCommandMapDirect(configuration);

  // Process all conditions
  GameCondition::buildConditionList(configuration);

  // Process those commands that have conditions
  GameCommand::buildCommandMapCondition(configuration);

  // Initialize defined sequences and static parameters for sequences
  Sequence::initialize(configuration);

  // Initialize menu items and global paramerts for the menu system
  GameMenu::instance().initialize(configuration);

  // Create the modifiers
  Modifier::buildModList(configuration);
}

void Configuration::buildSequence(toml::table& config, const std::string& key, Sequence& sequence) {
  toml::array* arr = config[key].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      assert (elem.as_table());
      toml::table& definition = *elem.as_table();

      checkValid(definition, std::vector<std::string>{"event", "command", "delay", "repeat", "value"}, "buildSequence");
      
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
	      PLOG_ERROR << "The value of 'repeat' should be an integer greater than 1. Will ignore.";
	      repeat = 1;
      }

      // Sequences are stored in terms of ControllerInput signals, but the TOML file uses command
      // names. We use the default mapping because signals are always sent out in the form the
      // console expects.
      std::optional<std::string> cmd = definition["command"].value<std::string>();
      std::shared_ptr<GameCommand> command = (cmd) ? GameCommand::get(*cmd) : nullptr;
      std::shared_ptr<ControllerInput> signal = (command) ? command->getInput() : nullptr;

	    // If this signal is a hybrid control, this gets the axis max, which is needed for addHold().
	    int max_val = (signal) ? signal->getMax(TYPE_AXIS) : 0;

      int value = definition["value"].value_or(max_val);

      if (*event == "delay") {
	      if (delay_time == 0) {
	        PLOG_ERROR << "You've tried to add a delay of 0 microseconds. This will be ignored.";
	      } else {
	        sequence.addDelay(delay_time);
          PLOG_VERBOSE << "Delay = " << delay_time << " microseconds";
	      }
      } else if (*event == "disable") {
	      sequence.disableControls();
      } else if (*event == "hold") {
	      if (!signal) {
	        throw std::runtime_error("A 'hold' event must be associated with a valid command.");
      	}
	      if (repeat > 1) {
	        PLOG_WARNING << "Repeat is not supported with 'hold' and will be ignored. (Did you want 'press'?)";
      	}
	      sequence.addHold(signal, value, 0);
        PLOG_VERBOSE << "Hold " << signal->getName();
      } else if (*event == "press") {
	      if (!signal) {
	        throw std::runtime_error("A 'press' event must be associated with a valid command.");
        }
	      for (int i = 0; i < repeat; i++) {
	        sequence.addPress(signal, value);
	        if (delay_time > 0) {
	          sequence.addDelay(delay_time);
	        }
          PLOG_VERBOSE << "Press " << signal->getName() << " with delay of " << delay_time << " microseconds.";
	      }
      } else if (*event == "release") {
	      if (!signal) {
	        throw std::runtime_error("A 'release' event must be associated with a valid command.");
	      }
	      if (repeat > 1) {
	        PLOG_WARNING << "Repeat is not supported with 'release' and will be ignored. (Did you want 'press'?)";
	      }
	      sequence.addRelease(signal, delay_time);
        PLOG_VERBOSE << "Release " << signal->getName() << " with delay of " << delay_time << " microseconds.";
      } else {
      	throw std::runtime_error("Unrecognized event type.");
      }
    }
  }
}

ControllerSignal Configuration::getSignal(const toml::table& config, const std::string& key, bool required) {
  if (! config.contains(key)) {
    if (required) {
      throw std::runtime_error("missing required '" + key + "' field");
    }
    return ControllerSignal::NONE;
  }
  std::optional<std::string> signal = config.get(key)->value<std::string>();
  std::shared_ptr<ControllerInput> inp = ControllerInput::get(*signal);
  if (! inp) {
    throw std::runtime_error("Controller signal '" + *signal + "' not defined");
  }
  return inp->getSignal();
}

ConditionCheck Configuration::getConditionTest(const toml::table& config, const std::string& key) {
  std::optional<std::string_view> ttype = config[key].value<std::string_view>();

  // Default type is magnitude
  ConditionCheck rval = ConditionCheck::ANY;
  if (ttype) {
    if (*ttype == "any") {
      rval = ConditionCheck::ANY;
    } else if (*ttype == "none") {
      rval = ConditionCheck::NONE;
    } else if (*ttype != "all") {
      PLOG_WARNING << "Invalid ConditionTest '" << *ttype << "': using 'all' instead.";
    }
  }
  return rval;
}

// warn if any keys in the table are unknown
bool Configuration::checkValid(const toml::table& config, const std::vector<std::string>& goodKeys) {
  return checkValid(config, goodKeys, config["name"].value_or("??"));
}

bool Configuration::checkValid(const toml::table& config, const std::vector<std::string>& goodKeys,
			    const std::string& name) {
  bool ret = true;
  for (auto&& [k, v] : config) {
    if (std::find(goodKeys.begin(), goodKeys.end(), k) == goodKeys.end()) {
      PLOG_WARNING << "The key '" << k << "' is unused in " << name;
      ret = false;
    }
  }
  return ret;
}


