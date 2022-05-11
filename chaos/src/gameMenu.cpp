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
#include <cmath>
#include <plog/Log.h>

#include "gameMenu.hpp"
#include "configuration.hpp"
#include "menuOption.hpp"
#include "subMenu.hpp"
#include "menuSelect.hpp"

using namespace Chaos;

GameMenu::GameMenu(toml::table& config) {

  toml::table* menu_list = config["menu"].as_table();
  if (! config.contains("menu")) {
    throw std::runtime_error("No 'menu' table found in configuration file");
  }

  // timing parameters
  // A 10 second delay is absurdly long, but it's unclear to me where to cap it. This mostly
  // in case a user gets mixed up and tries to enter time in milliseconds or some other unit.
  double delay = Configuration::getValue<double>(*menu_list, "disable_delay", 0, 10, 0.333333);
  disable_delay = (unsigned int) (delay * 1000000);
  PLOG_VERBOSE << "menu disable_delay = " << delay << " seconds (" << disable_delay << " microseconds)";

  delay = Configuration::getValue<double>(*menu_list, "select_delay", 0, 10, 0.05);
  select_delay = (unsigned int) (delay * 1000000);
  PLOG_VERBOSE << "menu select_delay = " << delay << " seconds (" << select_delay << " microseconds)";

  remember_last = (*menu_list)["remember_last"].value_or(false);
  PLOG_VERBOSE << "menu remember_last = " << remember_last;

  hide_guarded = (*menu_list)["hide_guarded"].value_or(false);

  // sequences corresponding to basic menu actions
  open = getDefinedSequence(*menu_list, "open");
  back = getDefinedSequence(*menu_list, "back");
  nav_up = getDefinedSequence(*menu_list, "navigate_up");
  nav_down = getDefinedSequence(*menu_list, "navigate_down");
  tab_left = getDefinedSequence(*menu_list, "tab_left");
  tab_right = getDefinedSequence(*menu_list, "tab_right");
  select = getDefinedSequence(*menu_list, "select");
  option_greater = getDefinedSequence(*menu_list, "option_greater");
  option_less = getDefinedSequence(*menu_list, "option_less");
  confirm = getDefinedSequence(*menu_list, "confirm");

  // menu layout
  toml::array* arr = (*menu_list)["layout"].as_array();
  if (! arr) {
    throw std::runtime_error("Menu layout must be in an array.");
  }
  for (toml::node& elem : *arr) {
    toml::table* m = elem.as_table();
    if (m) {
      std::optional<std::string> cmd_name = (*m)["name"].value<std::string>();
      if (! cmd_name) {
        PLOG_ERROR << "Menu item missing required name field: " << m;
        continue;
      }
      // Check for duplicate names
      if (getMenuItem(*cmd_name)) {
        PLOG_ERROR << "Menu item with duplicate name: " << *cmd_name;
        continue;
      }
      std::optional<std::string_view> menu_type = (*m)["type"].value<std::string_view>();
      if (! menu_type) {
        throw std::runtime_error("Menu item definition lacks required 'type' parameter.");
      }
      try {
        PLOG_VERBOSE << "Adding Menu Item '" << *cmd_name << "'";
        if (*menu_type == "option") {
       	  menu.insert({*cmd_name, std::make_shared<MenuOption>(*m)});
        } else if (*menu_type == "select") {
       	  menu.insert({*cmd_name, std::make_shared<MenuSelect>(*m)});
        } else if (*menu_type == "menu") {
       	  menu.insert({*cmd_name, std::make_shared<SubMenu>(*m)});
        } else {
          PLOG_ERROR << "Menu type '" << *menu_type << "' not recognized.";
        }
      }
      catch (const std::runtime_error& e) {
        PLOG_ERROR << "In definition for MenuItem '" << *cmd_name << "': " << e.what(); 
      }
    } else {
      PLOG_ERROR << "Each menu-item definition must be a table.";
    }
  }
}

std::shared_ptr<Sequence> GameMenu::getDefinedSequence(const toml::table& config, const std::string& name) {
  std::optional<std::string> seq_name = config[name].value<std::string>();
  if (seq_name) {
    return Sequence::get(*seq_name);
  }
  return nullptr;
}

std::shared_ptr<MenuItem> GameMenu::getMenuItem(const std::string& name) {
 auto iter = menu.find(name);
  if (iter != menu.end()) {
    return iter->second;
  }
  return nullptr;
}

void GameMenu::setState(std::shared_ptr<MenuItem> item, unsigned int new_val) {
  PLOG_VERBOSE << "GameMenu::setState: ";

  Sequence seq;
  // We build a sequence to open the main menu, navigate to the option, and perform the appropriate
  // action on the item.
  seq.disableControls();
  seq.addDelay(disable_delay);
  seq.addSequence(open);

  item->setState(seq, new_val);

  // TO DO: if the menu system does not remember where we left things, there's likely a faster way to
  // exit the menu system, but it's pointless to try to implement this at this point.
  item->navigateBack(seq);
  seq.addSequence(menu_exit);

  // send the sequence
  seq.send();
}

void GameMenu::restoreState(std::shared_ptr<MenuItem> item) {
  Sequence seq;
  seq.disableControls();
  seq.addDelay(disable_delay);
  seq.addSequence(open);
  item->restoreState(seq);
  item->navigateBack(seq);
  seq.addSequence(menu_exit);
  seq.send();
}


void GameMenu::correctOffset(std::shared_ptr<MenuItem> changed) {
  // direction of the correction depends on whether the changed item is now hidden or revealed, and
  // on whether the offset is positive or negative
  int adjustment = (changed->isHidden() ? -1 : 1) * (changed->getOffset() < 0 ? -1 : 1);
  PLOG_VERBOSE << "Adjusting offset " << changed->getOffset() << " by " << adjustment;
  for (auto& [name, entry] : menu) {
    // Process all other items sharing the same parent and tab group
    if (entry->getParent() == changed->getParent() && entry->getptr() != changed->getptr() &&
        entry->getTab() == changed->getTab()) {
      if (abs(entry->getOffset()) > abs(changed->getOffset())) {
        entry->adjustOffset(adjustment);
        PLOG_VERBOSE << " - adjustOffset: " << adjustment;
      }
    }
  }
}
