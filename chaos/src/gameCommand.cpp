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
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <cassert>
#include <toml++/toml.h>
#include <plog/Log.h>

#include "gameCommand.hpp"
#include "gameCondition.hpp"

using namespace Chaos;

std::unordered_map<std::string, std::shared_ptr<GameCommand>> GameCommand::commands;
std::string unknown_command = "UNKNOWN (Own pointer not found -- something is wrong)";
/*
 * The constructor for the command binding. The name of the command has already been read and will
 * form the key in the map. Here we set the main binding and any additional conditions.
 */
GameCommand::GameCommand(toml::table& definition) {
  std::optional<std::string> name = definition["name"].value<std::string>();
  assert (name);

  // Binding the command to a specific controller command
  std::optional<std::string> bind = definition["binding"].value<std::string>();
  if (!bind) {
    throw std::runtime_error("Missing command binding.");
  }
  binding = ControllerInput::get(*bind);
  if (! binding) {
    throw std::runtime_error("Specified command binding does not exist.");
  }
  
  std::optional<std::string> cond = definition["condition"].value<std::string>();

  if (definition.contains("unless")) {
    if (cond) {
      throw std::runtime_error("'condition' and 'unless' are mutually exclusive options in a command definition");
    }
    cond = definition["unless"].value<std::string>();
    invert_condition = true;
  } 
  
  // Condition/unless should be the name of a defined GameCondition. The constructors for game
  // commands with conditions should only be called after those conditions have been initialized.
  if (cond) {
    condition = GameCondition::get(*cond);
    if (!condition) {
      throw std::runtime_error("Game command '" + *name + "' refers to unknown condition '" + *cond + "'"); 
    }
  }
}

void GameCommand::buildCommandMapDirect(toml::table& config) {
  // This routine should always be called before buildCommandMapCondition (and only once)
  assert(commands.size() == 0);
  
  // We should have an array of tables. Each table defines one command binding.
  toml::array* arr = config["command"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      if (toml::table* command = elem.as_table()) {
        // skip command definitions that contain conditions
        if (command->contains("condition") || command->contains("unless")) {
          continue;
        }
	      if (! command->contains("name")) {
	        PLOG_ERROR << "Command definition missing required 'name' field: " << command;
          continue;
        }
        std::string cmd_name = command->get("name")->value_or("");
	      if (commands.count(cmd_name) == 1) {
	        PLOG_ERROR << "Duplicate command definition for '" << cmd_name << "'. Later one will be ignored.";
          continue;
	      }
	      PLOG_VERBOSE << "Inserting '" << cmd_name << "' into game command list.";
	      try {
      	  commands.insert({cmd_name, std::make_shared<GameCommand>(*command)});
	      }
	      catch (const std::runtime_error& e) {
	        PLOG_ERROR << "In definition for game command '" << cmd_name << "': " << e.what();
	      }
	    }
    }
  }
  if (commands.size() == 0) {
    PLOG_WARNING << "No unconditional game commands were defined.";
  }
}

void GameCommand::buildCommandMapCondition(toml::table& config) {
  
  // We should have an array of tables. Each table defines one command binding.
  toml::array* arr = config["command"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      if (toml::table* command = elem.as_table()) {
        // We already logged an error if the command is missing a name field, so just skip here
      	if (! command->contains("name")) {
          continue;
        }
	      std::string cmd_name = command->get("name")->value_or("");
        // Skip conditions already defined by call to buldCommandMapDirect
    	  if (commands.count(cmd_name) == 1) {
	        continue;
	      }
	      PLOG_VERBOSE << "inserting '" << cmd_name << "': " << command;
	      try {
    	    commands.insert({cmd_name, std::make_shared<GameCommand>(*command)});
	      }
	      catch (const std::runtime_error& e) {
	        PLOG_ERROR << "In definition for game command '" << cmd_name << "': " << e.what(); 
	      }
      }
    }
  }
  if (commands.size() == 0) {
    throw std::runtime_error("No game commands were defined.");
  }
}

std::shared_ptr<GameCommand> GameCommand::get(const std::string& name) {
  auto iter = commands.find(name);
  if (iter != commands.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<ControllerInput> GameCommand::getRemapped() {
  ControllerSignal r = binding->getRemap();
  // NONE = no remapping, so return the actual signal
  if (r == ControllerSignal::NONE) {
    return binding;
  }
  if (r == ControllerSignal::NOTHING) {
    return nullptr;
  }
  return ControllerInput::get(r);
}

short GameCommand::getState() {
  return binding->getRemappedState();
}
