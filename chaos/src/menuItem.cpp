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
#include <toml++/toml.h>
#include <plog/Log.h>

#include "menuItem.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

std::unordered_map<std::string, std::shared_ptr<MenuItem>> MenuItem::menu;

MenuItem::MenuItem(toml::table& config) {
  assert(config.contains("name"));
  assert(config.contains("type"));

  TOMLReader::checkValid(config, std::vector<std::string>{
      "name", "type", "parent", "offset", "initialState", "hidden", "confirm", "tab", "childState"});

  std::optional<std::string_view> parent_name = config["parent"].value<std::string_view>();
  if (parent_name) {
    parent = get(*parent_name);
    if (!parent) {
      throw std:runtime_error("Unknown parent menu '" + *parent_name + "'. Parents must be declared before children.");
    }
  }

  hidden = config["hidden"].value_or(false);

  tab_group = config["tab"].value_or(0);

  std::optional<int> menu_offset = config["offset"].value<int>();
  if (menu_offset) {
    offset = *menu_offset;
  } else {
    PLOG_WARNING << "Menu item '" << *config["name"].value<std::string_view>() + "' missing explicit offset. Setting to 0.");
  }

  state = config["initialState"].value_or(0);


  keep_child_state = config["childState"].value_or(false);

  confirm = config["confirm"].value_or(false);
  
}


