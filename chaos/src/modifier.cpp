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
#include <unistd.h>
#include <cstdlib>
#include <string_view>
#include <stdexcept>

#include <plog/Log.h>

#include "modifier.hpp"

using namespace Chaos;

const std::string Modifier::name = "modifier";

void Modifier::initialize(const toml::table& config) {
  timer.initialize();
  me = shared_from_this();
  pauseTimeAccumulator = 0;
  totalLifespan = -1;    // An erroneous value that if set should be positive

  description = config["description"].value_or("Description not available");
  
  if (config.contains("appliesTo")) {
    const toml::array* cmd_list = config.get("appliesTo")->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error("'appliesTo' must be an array of strings.");
    }
    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      if (cmd && GameCommand::bindingMap.count(*cmd) == 1) {
	commands.push_back(GameCommand::bindingMap[*cmd]);
      } else {
	throw std::runtime_error("unrecognized command '" + *cmd + "' in appliesTo");
      }
    }
  }
}

// Static function to construct the list of mods from their TOML-file definitions
void Modifier::buildModList(toml::table& config) {
  
  // Should have an array of tables, each one defining an individual modifier.
  toml::array* arr = config["modifier"].as_array();
  
  if (arr) {
    // Each node in the array should contain the definition for a modifier.
    // If there's a parsing error at this point, we skip that mod and keep going.
    for (toml::node& elem : *arr) {
      toml::table* modifier = elem.as_table();
      if (! modifier) {
	PLOG_ERROR << "Modifier definition must be a table\n";
	continue;
      }
      if (! modifier->contains("name")) {
	PLOG_ERROR << "Modifier missing required 'name' field: " << *modifier << "\n";
	continue;
      }
      std::optional<std::string> mod_name = modifier->get("name")->value<std::string>();
      if (mod_list.count(*mod_name) == 1) {
	PLOG_ERROR << "The modifier '" << *mod_name << " has already been defined\n";
	continue;
      } 
      if (! modifier->contains("type")) {
	PLOG_ERROR << "Modifier '" << *mod_name << " does not specify a type\n";
	continue;
      }
      std::optional<std::string> mod_type = modifier->get("type")->value<std::string>();
      if (! hasType(*mod_type)) {
	PLOG_ERROR << "Modifier '" << *mod_name << "' has unknown type '" << *mod_type << "'\n";
	continue;
      }
      // Now we can finally create the mod. The constructors handle the rest of the configuration.
      try {
	mod_list[*mod_name] = Modifier::create(*mod_type, config);
      }
      catch (std::runtime_error e) {
	PLOG_ERROR << "Modifier '" << *mod_name << "' not created: " << e.what() << std::endl;
      }
    }
  }
  if (mod_list.size() == 0) {
    PLOG_FATAL << "No modifiers were defined.\n";
    throw std::runtime_error("No modifiers defined");
  }
}

void Modifier::_update(bool isPaused) {
  timer.update();
  if (isPaused) {
    pauseTimeAccumulator += timer.dTime();
  }
  update();
}


// Default implementations of virtual functions do nothing
void Modifier::begin() {}

void Modifier::update() {}

void Modifier::finish() {}

bool Modifier::tweak( DeviceEvent* event ) {
  return true;
  }

Json::Value Modifier::toJsonObject(const std::string& name) {
  Json::Value result;
  result["name"] = name;
  result["desc"] = getDescription();
  result["lifespan"] = lifespan();
  return result;
}

std::string Modifier::getModList() {
  Json::Value root;
  Json::Value& data = root["mods"];
  int i = 0;
  for (auto const& [key, val] : mod_list) {
    data[i++] = val->toJsonObject(key);
  }
	
  Json::StreamWriterBuilder builder;
	
  return Json::writeString(builder, root);
}

void Modifier::setController(Controller* contr) {
  controller = contr;
}

void Modifier::setEngine(ChaosEngine* eng) {
  engine = eng;
}


