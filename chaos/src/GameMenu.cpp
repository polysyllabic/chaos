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
#include <stack>
#include <plog/Log.h>

#include "GameMenu.hpp"
#include "TOMLUtils.hpp"
#include "Controller.hpp"
#include "Sequence.hpp"
#include "SequenceTable.hpp"
#include "MenuItem.hpp"

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
  disable_delay = usec(delay);
  PLOG_VERBOSE << "menu disable_delay = " << delay << " seconds (" << disable_delay.count() << " microseconds)";

  delay = TOMLUtils::getValue<double>(*menu_list, "select_delay", 0, 10, 0.05);
  select_delay = usec(delay);
  PLOG_VERBOSE << "menu select_delay = " << delay << " seconds (" << select_delay.count() << " microseconds)";

  remember_last = (*menu_list)["remember_last"].value_or(false);
  PLOG_VERBOSE << "menu remember_last = " << remember_last;

  hide_guarded = (*menu_list)["hide_guarded"].value_or(false);
  PLOG_VERBOSE << "menu hide_guarded = " << hide_guarded;

  if (! menu_list->contains("layout")) {
    PLOG_ERROR << "No menu layout found!";
    return 1;
  }
  return 0;
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


void GameMenu::setState(std::shared_ptr<MenuItem> item, unsigned int new_val, bool restore, Controller& controller) {
  PLOG_DEBUG << "Creating set menu sequence";
  Sequence seq{controller};

  defined_sequences->addSequence(seq, "disable all");
  seq.addDelay(disable_delay);
  defined_sequences->addSequence(seq, "open menu");

  // Create a stack of all the parent menus of this option
  std::stack<std::shared_ptr<MenuItem>> menu_stack;
  std::shared_ptr<MenuItem> s;
  for (s = item->getParent(); s != nullptr; s = s->getParent()) {
    PLOG_VERBOSE << "Push " << s->getName() << " on stack";
    menu_stack.push(s);
  }

  // Now create the navigation commands. Popping from the top of the stack starts us with the
  // top-level menu and works down to the terminal leaf.
  PLOG_DEBUG << "Navigation commands for " << item->getName();
  // Navigate through the parent menus
  while (!menu_stack.empty()) {
    s = menu_stack.top();
    s->selectItem(seq);
    menu_stack.pop();
  }
  // navigation through the final leaf
  item->selectItem(seq);  
  item->setState(seq, new_val, restore);
  PLOG_DEBUG << "Constructing reverse navigation";
  item->navigateBack(seq);
  // Back out, leaving all the parent menus in their default state
  for (s = item->getParent(); s != nullptr; s = s->getParent()) {
    s->navigateBack(seq);
  }
  seq.send();
}

void GameMenu::restoreState(std::shared_ptr<MenuItem> item, Controller& controller) {
  PLOG_DEBUG << "Creating restore menu sequence";
  setState(item, item->getDefault(), true, controller);
}

void GameMenu::correctOffset(std::shared_ptr<MenuItem> changed) {
  // direction of the correction depends on whether the changed item is now hidden or revealed, and
  // on whether the offset is positive or negative
  int adjustment = (changed->isHidden() ? -1 : 1) * (changed->getOffset() < 0 ? -1 : 1);
  PLOG_DEBUG << "Adjusting offset " << changed->getOffset() << " by " << adjustment;
  for (auto& [name, entry] : menu) {
    // Process all other items sharing the same parent and tab group
    if (entry->getParent() == changed->getParent() && entry->getptr() != changed->getptr() &&
        entry->getTab() == changed->getTab()) {
      if (abs(entry->getOffset()) > abs(changed->getOffset())) {
        entry->adjustOffset(adjustment);
        PLOG_DEBUG << " - adjustOffset: " << adjustment;
      }
    }
  }
}

void GameMenu::addSequence(Sequence& sequence, const std::string& name) {
  PLOG_DEBUG << "adding sequence " << name;
  defined_sequences->addSequence(sequence, name);
}

void GameMenu::addSelectDelay(Sequence& sequence) {
  PLOG_DEBUG << "adding select delay";
  defined_sequences->addDelay(sequence, select_delay);
}

bool GameMenu::insertMenuItem(std::string& name, std::shared_ptr<MenuItem> new_item) {
  auto [it, result] = menu.try_emplace(name, new_item);
  return result;
}