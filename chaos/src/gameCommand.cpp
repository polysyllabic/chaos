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
