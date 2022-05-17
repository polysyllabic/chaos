/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file at the top-level directory of this distribution for details of the
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

#include "GameConditionTable.hpp"

using namespace Chaos;

int GameConditionTable::buildConditionList(toml::table& config, GameCommandTable& commands) {
  int parse_errors = 0;
  
  // If we're loading a game after initialization, empty the previous data
  if (condition_map.size() > 0) {
    PLOG_VERBOSE << "Clearing existing GameCondition data.";
    condition_map.clear();
  }

  toml::array* arr = config["condition"].as_array();
  if (arr) {
    // Each node in the array should contain table defining one condition
    for (toml::node& elem : *arr) {
      toml::table* condition = elem.as_table();
      if (! condition) {
          ++parse_errors;
        PLOG_ERROR << "Condition definition must be a table";
        continue;
      }
      if (!condition->contains("name")) {
          ++parse_errors;
        PLOG_ERROR << "Condition missing required 'name' field: " << condition;
        continue;
      }
      std::optional<std::string> cond_name = condition->get("name")->value<std::string>();
      if (condition_map.count(*cond_name) == 1) {
          ++parse_errors;
        PLOG_ERROR << "The condition '" << *cond_name << "' has already been defined.";
      }
      try {
        PLOG_VERBOSE << "Adding condition '" << *cond_name << "' to static map.";
  	    auto [it, result] = condition_map.try_emplace(*cond_name, std::make_shared<GameCondition>(*condition, commands));
        if (! result) {
          ++parse_errors;
          PLOG_ERROR << "Duplicate condition name: " << *cond_name;
        }
	    }
	    catch (const std::runtime_error& e) {
        ++parse_errors;
	      PLOG_ERROR << "In definition for condition '" << *cond_name << "': " << e.what(); 
	    }
    }
  }
  return parse_errors;
}

std::shared_ptr<GameCondition> GameConditionTable::getCondition(const std::string& name) {
  auto iter = condition_map.find(name);
  if (iter != condition_map.end()) {
    return iter->second;
  }
  return nullptr;
}

void GameConditionTable::addToVector(const toml::table& config, const std::string& key,
                                   std::vector<std::shared_ptr<GameCondition>>& vec) {
      
  if (config.contains(key)) {
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error(key + " must be an array of strings");
   	}
	
    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
   	  std::shared_ptr<GameCondition> item = getCondition(*cmd);
      if (item) {
        vec.push_back(item);
        PLOG_VERBOSE << "Added '" + *cmd + "' to the " + key + " vector.";
      } else {
        throw std::runtime_error("Unrecognized condition: " + *cmd + " in " + key);
     	}
    }
  }      
}
