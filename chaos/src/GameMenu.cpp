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

#include "GameMenu.hpp"
#include "TOMLUtils.hpp"
#include "MenuOption.hpp"
#include "SubMenu.hpp"
#include "MenuSelect.hpp"
#include "Controller.hpp"
#include "Sequence.hpp"
#include "SequenceTable.hpp"

using namespace Chaos;

int GameMenu::initialize(toml::table& config, std::shared_ptr<SequenceTable> sequences) {
  PLOG_VERBOSE << "Initializing menu";
  defined_sequences = sequences;
  assert(defined_sequences);

  int errors = 0;
  toml::table* menu_list = config["menu"].as_table();
  if (! config.contains("menu")) {
    PLOG_ERROR << "No 'menu' table found in configuration file";
    return 1;
  }

  // Timing parameters
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

  if (! menu_list->contains("layout")) {
    PLOG_ERROR << "No menu layout found!";
    return 1;
  }
  // Menu layout
  toml::array* arr = (*menu_list)["layout"].as_array();
  if (! arr) {
    ++errors;
    PLOG_ERROR << "Menu layout must be in an array.";
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

  std::optional<std::string> menu_type = config["type"].value<std::string>();
  if (! menu_type) {
    PLOG_ERROR << "Menu item definition lacks required 'type' parameter.";
    return 1;
  }

  if (*menu_type != "option" && *menu_type == "select" && *menu_type != "menu") {
    PLOG_ERROR << "Menu type '" << *menu_type << "' not recognized.";
    return 1;
  }

  bool opt = (*menu_type == "option" || *menu_type == "select");
  bool sel = (*menu_type == "select" || *menu_type == "menu");

  PLOG_VERBOSE << "Adding menu item '" << *entry_name << "' of type " << *menu_type;

  if (! config.contains("offset")) {
    PLOG_WARNING << "Menu item '" << config["name"] << "' missing offset. Set to 0.";
  }
  short off = config["offset"].value_or(0);
  short tab = config["tab"].value_or(0);
  short initial = config["initialState"].value_or(0);
  bool hide = config["hidden"].value_or(false);

  PLOG_VERBOSE << "-- offset = " << off << "; tab = " << tab <<
      "; initial_state = " << initial << "; hidden = " << hide;

  std::shared_ptr<MenuItem> parent;
  try {
    PLOG_VERBOSE << "checking parent";
    parent = getMenuItem(config, "parent");
  } catch (const std::runtime_error& e) {
    ++errors;
    PLOG_ERROR << e.what();
  }

  std::shared_ptr<MenuItem> guard;
  try {
    PLOG_VERBOSE << "checking guard";
    guard = getMenuItem(config, "guard");
  } catch (const std::runtime_error& e) {
    ++errors;
    PLOG_ERROR << e.what();
  }

  std::shared_ptr<MenuItem> counter;
  try {
    PLOG_VERBOSE << "checking sibling";
    counter = getMenuItem(config, "counter");
  } catch (const std::runtime_error& e) {
    ++errors;
    PLOG_ERROR << e.what();
  }

  // CounterAction ignored for now
/* 
  CounterAction action = CounterAction::NONE;
  std::optional<std::string> action_name = config["counterAction"].value<std::string>();  
  if (action_name) {
    if (*action_name == "reveal") {
      action = CounterAction::REVEAL;
    } else if (*action_name == "zeroReset") {
      action = CounterAction::ZERO_RESET;
    } else if (*action_name != "none") {
      throw std::runtime_error("Unknown counterAction type: " + *action_name);
    }
  } */

  bool confirm = config["confirm"].value_or(false);

  PLOG_VERBOSE << "constructing menu item";
  assert(getptr());
  try {
    std::shared_ptr<MenuItem> m = std::make_shared<MenuItem>(getptr(), *entry_name,
      off, tab, initial, hide, opt, sel, confirm, parent, guard, counter);
 	  menu.try_emplace(*entry_name, m);
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

std::shared_ptr<MenuItem> GameMenu::getMenuItem(toml::table& config, const std::string& key) {
  std::optional<std::string> item_name = config[key].value<std::string>();
  std::shared_ptr<MenuItem> item = nullptr;
  if (item_name) {
    item = getMenuItem(*item_name);
    if (item) {
      PLOG_VERBOSE << "-- " << key << " = " << ((item_name) ? *item_name : "[NONE]");
    } else {
      throw std::runtime_error("Unknown " + key + " menu item: " + *item_name);
    }
  }
  return item;
}

void GameMenu::setState(std::shared_ptr<MenuItem> item, unsigned int new_val, Controller& controller) {
  PLOG_VERBOSE << "Creating set menu sequence";
  Sequence seq{controller};

  // We build a sequence to open the main menu, navigate to the option, and perform the appropriate
  // action on the item.
  defined_sequences->addSequence(seq, "disable all");
  seq.addDelay(disable_delay);
  defined_sequences->addSequence(seq, "open menu");
  item->setState(seq, new_val);
  item->navigateBack(seq);
  defined_sequences->addSequence(seq, "menu exit");
  seq.send();
}

void GameMenu::restoreState(std::shared_ptr<MenuItem> item, Controller& controller) {
  PLOG_VERBOSE << "Creating restore menu sequence";
  Sequence seq(controller);
  defined_sequences->addSequence(seq, "disable all");
  seq.addDelay(disable_delay);
  defined_sequences->addSequence(seq, "open menu");
  item->restoreState(seq);
  item->navigateBack(seq);
  defined_sequences->addSequence(seq, "menu exit");
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

void GameMenu::addSequence(Sequence& sequence, const std::string& name) {
  defined_sequences->addSequence(sequence, name);
}

void GameMenu::addSelectDelay(Sequence& sequence) {
  defined_sequences->addDelay(sequence, select_delay);
}

