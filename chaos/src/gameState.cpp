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
#include <stdexcept>
#include <plog/Log.h>

#include "gameState.hpp"
#include "gameCommand.hpp"

using namespace Chaos;

std::unordered_map<std::string, std::shared_ptr<GameState>> GameState::states;

GameState::GameState(const toml::table& config) {
  std::shared_ptr<GameCommand> cmd;
  std::optional<std::string> true_when = config["trueOn"].value<std::string>();
  if (true_when) {
    cmd = GameCommand::get(*true_when);
    if (cmd) {
      signal_on = cmd->getBinding();
    } else {
      throw std::runtime_error("The command '" + *true_when + "' for 'trueOn' is not defined.");
    }
  } else {
    throw std::runtime_error("Missing 'trueOn' parameter for gamestate.");
  }
    
  std::optional<std::string> false_when = config["falseOn"].value<std::string>();
  if (false_when) {
    cmd = GameCommand::get(*false_when);
    if (cmd) {
      signal_off = cmd->getBinding();
    } else {
      throw std::runtime_error("The command '" + *false_when + "' for 'falseOn' is not defined.");
    }
  } else {
    throw std::runtime_error("Missing 'trueOn' parameter for gamestate.");    
  }
}

/**
 * Process the part of the TOML config file that defines game sates and create a map.
 * The map is used when creating mods.
 */
void GameState::initialize(toml::table& config) {
  toml::array* arr = config["gamestate"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      toml::table* gstate = elem.as_table();
      if (! gstate) {
	PLOG_ERROR << "Definition of a 'gamestate' must be as a table\n";
	continue;
      }
      if (! gstate->contains("name")) {
	PLOG_ERROR << "Missing required 'name' field" << *gstate << "\n";
	continue;
      }
      std::optional<std::string> state_name = gstate->get("name")->value<std::string>();
      // check that the definition is unique
      if (states.count(*state_name) == 1) {
	PLOG_ERROR << "The game state '" << *state_name << " has already been defined\n";
	continue;
      } 
      try {
	states.insert({*state_name, std::make_shared<GameState>(*gstate)});
      }
      catch (std::runtime_error e) {
	PLOG_ERROR << "Game condition '" << *state_name << "' not created: " << e.what() << std::endl;
      }
    }
  }
}

std::shared_ptr<GameState> GameState::get(const std::string& name) {
  auto iter = states.find(name);
  if (iter != states.end()) {
    return iter->second;
  }
  return NULL;  
}
