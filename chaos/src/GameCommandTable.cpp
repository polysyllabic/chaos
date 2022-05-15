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

#include "GameCommandTable.hpp"
#include "ControllerInputTable.hpp"

using namespace Chaos;

int GameCommandTable::buildCommandList(toml::table& config, ControllerInputTable& signals) {
  int parse_errors = 0;
  
  // Wipe everything if we're reloading a file
  if (command_map.size() > 0) {
    PLOG_VERBOSE << "Clearing existing GameCommand data.";
    command_map.clear();
  }

  // We should have an array of tables. Each table defines one command binding.
  toml::array* arr = config["command"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      if (toml::table* command = elem.as_table()) {
	      if (! command->contains("name")) {
	        PLOG_ERROR << "Command definition missing required 'name' field: " << command;          
          ++parse_errors;
          continue;
        }
        std::optional<std::string> item = config["binding"].value<std::string>();
        if (!item) {
          PLOG_ERROR << "Missing command binding.";
          ++parse_errors;
          continue;
        }
        std::shared_ptr<ControllerInput> bind = signals.getInput(*item);
        if (!bind) {
          PLOG_ERROR << "Specified command binding does not exist.";
          ++parse_errors;
          continue;
        }

        std::string cmd_name = command->get("name")->value_or("");
	      if (command_map.count(cmd_name) > 0) {
	        PLOG_ERROR << "Duplicate command definition for '" << cmd_name << "'. Later one will be ignored.";
          ++parse_errors;
          continue;
	      }
	      PLOG_VERBOSE << "Inserting '" << cmd_name << "' into game command list.";
	      try {
      	  auto [it, result] = command_map.try_emplace(cmd_name, std::make_shared<GameCommand>(cmd_name, bind));
          if (!result) {
            ++parse_errors;
            PLOG_ERROR << "Duplicate command definition ignored: " << cmd_name;
          }
	      }
	      catch (const std::runtime_error& e) {
          ++parse_errors;
	        PLOG_ERROR << "In definition for game command '" << cmd_name << "': " << e.what();
	      }
	    }
    }
  } else {
    ++parse_errors;
    PLOG_ERROR << "Command definitions should be an array of tables";
  }
  return parse_errors;
}
