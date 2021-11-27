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
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <cassert>
#include <toml++/toml.h>
#include <plog/Log.h>

#include "gameCommand.hpp"

using namespace Chaos;

std::unordered_map<std::string, std::shared_ptr<GameCommand>> GameCommand::commands;

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
  std::shared_ptr<GamepadInput> bnd = GamepadInput::get(*bind);
  if (! bnd) {
    throw std::runtime_error("Specified command binding does not exist.");
  }
  binding = bnd->getSignal();
  
  std::optional<std::string> cond = definition["condition"].value<std::string>();

  if (definition.contains("unless")) {
    if (cond) {
      throw std::runtime_error("'condition' and 'unless' are mutually exclusive options");
    }
    cond = definition["unless"].value<std::string>();
    invertCondition = true;
  } 
  
  // Condition/unless should be the name of a previously defined GameCommand
  if (cond) {
    if (commands.count(*cond) == 1) {
      condition = commands.at(*cond)->getBinding();
    } else {
      throw std::runtime_error("Condition refers to unknown game command."); 
    }

    // If there's a specific threshold, set it.
    threshold = definition["threshold"].value_or(1);
  }
}

/**
 * Factory to create the map of default bindings between the game's commands and the button/joystick
 * commands that invoke these commands.
 *
 * We walk through the portion of the TOML config file that defines command bindings and build a map
 * from that. Note that if the game supports remapping commands in its own menus, the user can update
 * the TOML file to reflect those changes rather than merely relying on the default setup.
 *
 * We log non-fatal errors, and throw fatal ones.
 */
void GameCommand::initialize(toml::table& config) {
  
  assert(commands.size() == 0);
  
  // We should have an array of tables. Each table defines one command binding.
  toml::array* arr = config["command"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      if (toml::table* command = elem.as_table()) {
	if (command->contains("name")) {
	  std::string cmd_name = command->get("name")->value_or("ERROR");
	  if (commands.count(cmd_name) == 1) {
	    PLOG_WARNING << "Duplicate command binding for '" << cmd_name << "'. Earlier one will be overwritten.\n";
	  }
	  PLOG_VERBOSE << "inserting '" << cmd_name << "': " << command << "\n";
	  try {
	    commands.insert({cmd_name, std::make_shared<GameCommand>(*command)});
	  }
	  catch (const std::runtime_error& e) {
	    PLOG_ERROR << "In definition for game command '" << cmd_name << "': " << e.what() << std::endl; 
	  }
	}
	else {
	  PLOG_ERROR << "Bad command-binding definition: " << *command << "\n";
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
  return NULL;
}

GPInput GameCommand::getInput(const std::string& name) {
  std::shared_ptr<GameCommand> cmd = get(name);
  if (cmd) {
    return cmd->getBinding();
  }
  PLOG_WARNING << "In GameCommand::getInput - command '" << name << "' not recognized\n";
  return GPInput::NONE;
}
