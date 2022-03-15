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

// TODO: menu shortcuts
// TODO: override default menu actions

#include <stdexcept>
#include <plog/Log.h>

#include "gameMenu.hpp"
#include "submenu.hpp"

using namespace Chaos;

void GameMenu::GameMenu(toml::table& config) {

  toml::table* menu = config["menu"].as_table();
  if (! config.contains("menu")) {
    throw std::runtime_error("No 'menu' table found in configuration file");
  }

  remember_last = menu["remember_last"].value_or(false);

  // timing parameters
  disable_delay = (unsigned int) menu["disable_delay"].value_or(0.333333) * 1000000;
  select_delay = (unsigned int) menu["select_delay"].value_or(0.05) * 1000000;

  // sequences corresponding to basic menu actions
  open = getDefinedSequence(menu, "open");
  back = getDefinedSequence(menu, "back");
  nav_up = getDefinedSequence(menu, "navigate_up");
  nav_down = getDefinedSequence(menu, "navigate_down");
  tab_left = getDefinedSequence(menu, "tab_left");
  tab_right = getDefinedSequence(menu, "tab_right");
  select = getDefinedSequence(menu, "select");
  option_greater = getDefinedSequence(menu, "option_greater");
  option_less = getDefinedSequence(menu, "option_less");
  confirm = getDefinedSequence(menu, "confirm");

  // menu layout
  toml::array* arr = config["layout"].as_array();
  if (! arr) {
    throw std::runtime_error("Menu layout must be in an array.");
  }
  for (toml::node& elem : *arr) {
    toml::table* m = elem.as_table();
    if (m) {
      std::optional<std::string> cmd_name = (*m)["name"].value<std::string>();
      if (! cmd_name) {
        PLOG_ERROR << "Menu item missing required name field: " << *m << std::endl;
        continue;
      }
      // Check for duplicate names
      if (getMenuItem(*cmd_name)) {
        PLOG_ERROR << "Menu item with duplicate name '" << *cmd_name << "'" << std::endl;
        continue;
      }
      std::optional<std::string_view> menu_type = (*m)["type"].value<std::string_view>();
      if (! menu_type) {
        throw std::runtime_error("Menu item definition lacks required 'type' parameter.");
      }
      try {
        PLOG_VERBOSE << "Adding Menu Item '" << *cmd_name << "'" << std::endl;
        if (*menu_type == "option") {
       	  menu.insert({*cmd_name, std::make_shared<MenuOption>(*m)});
        } else if (*menu_type == "select") {
       	  menu.insert({*cmd_name, std::make_shared<SelectOption>(*m)});
        } else if (*menu_type == "menu") {
       	  menu.insert({*cmd_name, std::make_shared<SubMenu>(*m)});
        } else if (*menu_type == "tab") {
       	  menu.insert({*cmd_name, std::make_shared<TabMenu>(*m)});
        } else if (*menu_type == "xgroup") {
       	  menu.insert({*cmd_name, std::make_shared<XGroupMenu>(*m)});
        } else {
          PLOG_ERROR << "Menu type '" << *menu_type << "' not recognized." << std::endl;
        }
      }
      catch (const std::runtime_error& e) {
        PLOG_ERROR << "In definition for MenuItem '" << cmd_name << "': " << e.what() << std::endl; 
      }
    } else {
      PLOG_ERROR << "Each menu-item definition must be a table.\n";
    }
  }
}

std::shared_ptr<Sequence> GameMenu::getDefinedSequence(toml::table& config, std::string& name) {
  std::optional<std::string> seq_name = config[name];
  if (seq_name) {
    return Sequence::get(*seq_name);
  }
  return NULL;
}

std::shared_ptr<MenuItem> MenuItem::getMenuItem(const std::string& name) {
 auto iter = menu.find(name);
  if (iter != menu.end()) {
    return iter->second;
  }
  return NULL;
}

