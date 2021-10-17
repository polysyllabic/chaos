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

std::unordered_map<std::string, std::unique_ptr<GameCommand>> GameCommand::bindingMap;

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
    // PLOG_ERROR << "Missing command binding for '" << *name << "': " << definition << "\n";
    throw std::runtime_error("Missing command binding");
  }
  // The binding value must exist in the controller commands list
  if (ControllerCommand::buttonNames.contains(*bind)) {
    binding = ControllerCommand::buttonNames.at(*bind);
    binding_remap = binding;
  } else {
    // PLOG_ERROR << "The specified command binding does not exist: " << definition << "\n";
    throw std::runtime_error("Bad command binding");
  }

  std::optional<std::string> cond = definition["condition"].value<std::string>();

  if (definition.contains("unless")) {
    // 'condition' and 'unless' are mutually exclusive options.
    if (cond) {
      // PLOG_ERROR << "Command definition '" << *name <<
      //   "': Cannot use both 'condition' and 'unless' in one command definition.\n";
      std::cerr << "Command definition '" << *name <<
	"': Cannot use both 'condition' and 'unless' in one command definition.\n";
      throw std::runtime_error("Incompatible command options");
    }
    cond = definition["unless"].value<std::string>();
    invertCondition = true;
  } 
  
  // Condition/unless should be the name of a previously defined GameCommand
  if (cond) {
    if (bindingMap.contains(*cond)) {
      // Look up the controller command that maps to this game command. We store the index to the
      // controller command for efficiency.
      condition = bindingMap.at(*cond)->getReal();
      condition_remap = condition;
      // default threshold for a condition
      threshold = 1;
    } else {
      // PLOG_ERROR << "Condition '" << *cond << " must be defined before it is used.\n"; 
      std::cerr << "Condition '" << *cond << " must be defined before it is used.\n";
    }

    // If there's a specific threshold, set it.
    std::optional<int> thresh = definition["threshold"].value<int>();
    if (thresh) {
      threshold = *thresh;
    }
  }
}

/**
 * Factory to create the map of default bindings between the game's commands and the button/joystick
 * commands that invoke these commands.
 *
 * We walk through the portion of the TOML config file that defines command bindings and build a map
 * from that. Note that if the game supports remapping commands in its own menus, the user can update
 * the TOML file to reflect those changes rather than merely relying on the default setup.
 */
void GameCommand::initialize(toml::table& config) {

  auto bnd = config["command"];
  // we should have an array of tables. Each table defines one command binding.
  if (toml::array* arr = bnd.as_array()) {
    for (toml::node& elem : * arr) {
      if (toml::table* command = elem.as_table()) {
	if (command->contains("name")) {
	  std::string cmd_name = command->get("name")->value_or("ERROR");
	  if (bindingMap.contains(cmd_name)) {
	    // PLOG_WARNING << "Duplicate command binding for '" << *cmd_name << "'. Earlier one will be overwritten.\n";
	    std::cerr << "Duplicate command binding for '" << cmd_name << "'. Earlier one will be overwritten.\n";
	  }
	  std::cout << "inserting '" << cmd_name << "': " << command << "\n";
	  bindingMap.insert({cmd_name, std::make_unique<GameCommand>(*command)});
	}
	else {
	  //	  PLOG_ERROR << "Bad command-binding definition: " << *command << "\n";
	  std::cerr << "Bad command-binding definition: " << *command << "\n";
	}
      }
    }
  }
  if (bindingMap.size() == 0) {
    //    PLOG_ERROR << "No command bindings defined\n";
    std::cerr << "No command bindings defined\n";
  }
}


