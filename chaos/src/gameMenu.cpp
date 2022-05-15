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
#include "tomlUtils.hpp"
#include "menuOption.hpp"
#include "subMenu.hpp"
#include "menuSelect.hpp"
#include "game.hpp"
#include "controller.hpp"

using namespace Chaos;

int GameMenu::initialize(toml::table& config, Game* game) {
  int errors = 0;
  toml::table* menu_list = config["menu"].as_table();
  if (! config.contains("menu")) {
    ++errors;
    PLOG_ERROR << "No 'menu' table found in configuration file";
  }

  // timing parameters
  // A 10 second delay is absurdly long, but it's unclear to me where to cap it. This mostly
  // in case a user gets mixed up and tries to enter time in milliseconds or some other unit.
  double delay = TOMLUtils::getValue<double>(*menu_list, "disable_delay", 0, 10, 0.333333);
  disable_delay = (unsigned int) (delay * 1000000);
  PLOG_VERBOSE << "menu disable_delay = " << delay << " seconds (" << disable_delay << " microseconds)";

  delay = TOMLUtils::getValue<double>(*menu_list, "select_delay", 0, 10, 0.05);
  select_delay = (unsigned int) (delay * 1000000);
  PLOG_VERBOSE << "menu select_delay = " << delay << " seconds (" << select_delay << " microseconds)";

  remember_last = (*menu_list)["remember_last"].value_or(false);
  PLOG_VERBOSE << "menu remember_last = " << remember_last;

  hide_guarded = (*menu_list)["hide_guarded"].value_or(false);
  PLOG_VERBOSE << "menu hide_guarded = " << hide_guarded;

  // Sequences for basic actions
  std::optional<std::string> seq_name = (*menu_list)["disable all"].value<std::string>();
  disable_all = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["open"].value<std::string>();
  open = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["back"].value<std::string>();
  back = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["navigate_up"].value<std::string>();
  nav_up = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["navigate_down"].value<std::string>();
  nav_down = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["tab_left"].value<std::string>();
  tab_left = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["tab_right"].value<std::string>();
  tab_right = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["select"].value<std::string>();
  select = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["option_greater"].value<std::string>();
  option_greater = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["option_less"].value<std::string>();
  option_less = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  seq_name = (*menu_list)["confirm"].value<std::string>();
  confirm = (seq_name) ?  game->getSequence(*seq_name) : nullptr;

  // Menu layout
  toml::array* arr = (*menu_list)["layout"].as_array();
  if (! arr) {
    ++errors;
    throw std::runtime_error("Menu layout must be in an array.");
  }
  for (toml::node& elem : *arr) {
    toml::table* m = elem.as_table();
    if (m) {
      errors += addMenuItem(*m);
    } else {
      ++errors;
      PLOG_ERROR << "Each menu-item definition must be a table.";
    }
  }
  return errors;
}

int GameMenu::addMenuItem(toml::table& config) {
  int errors = 0;

  TOMLUtils::checkValid(config, std::vector<std::string>{"name", "type", "offset", "tab",
                        "initialState", "parent", "guard", "hidden", "counter", "counterAction"});

  std::optional<std::string> entry_name = config["name"].value<std::string>();
  if (! entry_name) {
    PLOG_ERROR << "Menu item missing required name field";
    return 1;
  }
  // Check for duplicate names
  if (getMenuItem(*entry_name)) {
    PLOG_ERROR << "Menu item with duplicate name: " << *entry_name;
    return 1;
  }

  std::optional<std::string> menu_type = config["type"].value<std::string>();
  if (! menu_type) {
    PLOG_ERROR << "Menu item definition lacks required 'type' parameter.";
    return 1;
  }

  std::shared_ptr<MenuItem> parent = getMenuItem(config, {"parent"}, errors);
  std::shared_ptr<MenuItem> guard = getMenuItem(config, {"guard"}, errors);
  std::shared_ptr<MenuItem> counter = getMenuItem(config, {"counter"}, errors);
  

  PLOG_VERBOSE << "Adding menu item '" << *entry_name << "' of type " << *menu_type;
  try {
    if (*menu_type == "option") {
   	  menu.insert({*entry_name, std::make_shared<MenuOption>(config, parent, guard, counter)});
    } else if (*menu_type == "select") {
      menu.insert({*entry_name, std::make_shared<MenuSelect>(config, parent, guard, counter)});
    } else if (*menu_type == "menu") {
      menu.insert({*entry_name, std::make_shared<SubMenu>(config, parent, guard, counter)});
    } else {
      PLOG_ERROR << "Menu type '" << *menu_type << "' not recognized.";
      return 1;
    }
  } catch (const std::runtime_error& e) {
    ++errors;
    PLOG_ERROR << "In definition for MenuItem '" << *entry_name << "': " << e.what(); 
  }
  return errors;
}

std::shared_ptr<MenuItem> GameMenu::getMenuItem(const std::string& name) {
 auto iter = menu.find(name);
  if (iter != menu.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<MenuItem> GameMenu::getMenuItem(toml::table& config, const std::string& key, int& errors) {
  std::optional<std::string> item_name = config[key].value<std::string>();
  std::shared_ptr<MenuItem> item = nullptr;
  if (item_name) {
    item = getMenuItem(*item_name);
    if (item) {
      PLOG_VERBOSE << "-- " << key << " = " << ((item_name) ? *item_name : "[NONE]");
    } else {
      ++errors;
      PLOG_ERROR << "Unknown " << key << " menu item: " << *item_name;
    }
  }
  return item;
}

void GameMenu::setState(std::shared_ptr<MenuItem> item, unsigned int new_val, Controller& controller) {
  Sequence seq;

  PLOG_VERBOSE << "Creating set menu sequence";

  // We build a sequence to open the main menu, navigate to the option, and perform the appropriate
  // action on the item.
  seq.addSequence(disable_all);
  seq.addDelay(disable_delay);
  seq.addSequence(open);

  item->setState(seq, new_val);

  // TO DO: if the menu system does not remember where we left things, there's likely a faster way to
  // exit the menu system, but it's pointless to try to implement this at this point.
  item->navigateBack(seq);
  seq.addSequence(menu_exit);

  // send the sequence
  seq.send(controller);
}

void GameMenu::restoreState(std::shared_ptr<MenuItem> item, Controller& controller) {
  Sequence seq;
  PLOG_VERBOSE << "Creating restore menu sequence";
  seq.addSequence(disable_all);
  seq.addDelay(disable_delay);
  seq.addSequence(open);
  item->restoreState(seq);
  item->navigateBack(seq);
  seq.addSequence(menu_exit);
  seq.send(controller);
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
