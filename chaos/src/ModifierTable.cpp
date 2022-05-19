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

#include "ModifierTable.hpp"
#include "EngineInterface.hpp"

using namespace Chaos;

// Handles the static initialization. We construct the list of mods from their TOML-file
// definitions.
int ModifierTable::buildModList(toml::table& config, EngineInterface* engine,
                                bool use_menu) {
  int parse_errors = 0;
  PLOG_VERBOSE << "Building modifier list";
  
  if (mod_map.size() > 0) {
    PLOG_VERBOSE << "Clearing existing Modifier data.";
    mod_map.clear();
  }

  // Should have an array of tables, each one defining an individual modifier.
  toml::array* arr = config["modifier"].as_array();
  
  if (arr) {
    int i = 0;
    // Each node in the array should contain the definition for a modifier.
    // If there's a parsing error at this point, we skip that mod and keep going.
    for (toml::node& elem : *arr) {
      toml::table* modifier = elem.as_table();
      if (! modifier) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier definition must be a table";
      	continue;
      }
      assert (modifier);
      PLOG_VERBOSE << "Processing mod #" << i;
      std::optional<std::string> mod_name = (*modifier)["name"].value<std::string>();
      if (! mod_name) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier missing required 'name' field: ";
	      continue;
      }
      std::optional<std::string> mod_type = (*modifier)["type"].value<std::string>();
      if (!mod_type) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier '" << *mod_name << " does not specify a type";
	      continue;
      }
      if (! Modifier::hasType(*mod_type)) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier '" << *mod_name << "' has unknown type '" << *mod_type << "'";
	      continue;
      }
      // skip menu mods if the menu system is disabled
      if (*mod_type == "menu" && ! use_menu) {
        continue;
      }
      // Now we can create the mod. The constructors handle the rest of the configuration.
      try {
	      PLOG_VERBOSE << "Adding modifier '" << *mod_name << "' of type " << *mod_type;
	      std::shared_ptr<Modifier> m = Modifier::create(*mod_type, *modifier, engine);
        assert(m);
        auto [it, result] = mod_map.try_emplace(*mod_name, m);
        if (! result) {
          ++parse_errors;
          PLOG_ERROR << "Duplicate modifier name: " << *mod_name;
        }
      }
      catch (std::runtime_error e) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier '" << *mod_name << "' not created: " << e.what();
      }
      ++i;
    }
  }
  if (mod_map.size() == 0) {
    ++parse_errors;
    PLOG_ERROR << "No modifiers were defined.";
    throw std::runtime_error("No modifiers defined");
  }
  return parse_errors;
}

std::string ModifierTable::getModList() {
  Json::Value root;
  Json::Value& data = root["mods"];
  for (auto const& [key, val] : mod_map) {
    // Mods that are marked unlisted are not reported to the chatbot
    if (! val->isUnlisted()) {
      data.append(val->toJsonObject());
    }
  }
	
  Json::StreamWriterBuilder builder;	
  return Json::writeString(builder, root);
}

std::shared_ptr<Modifier> ModifierTable::getModifier(const std::string& name) {
  auto iter = mod_map.find(name);
  if (iter != mod_map.end()) {
    return iter->second;
  }
  return nullptr;
}
