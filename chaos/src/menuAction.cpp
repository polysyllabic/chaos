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
#include <algorithm>

#include <plog/Log.h>

#include <tomlReader.hpp>
#include <config.hpp>

#include "menuAction.hpp"

using namespace Chaos;

MenuAction::MenuAction(toml::table& config) {
  TOMLReader::checkValid(config, std::vector<std::string>{
      "event", "command", "value"});
  
  std::optional<std::string> event_type = config["event"].value<std::string>();
  if (!event_type) {
    throw std::runtime_error("Missing required 'event' for menu action.");
  }
  if (*event_type == "set") {
    type = MenuActionType::SET;
  } else if (*event_type == "restore") {
    type = MenuActionType::RESTORE;
  } else {
    throw std::runtime_error("Unrecognized event type for menu action: " + *event_type);
  }

  std::optional<std::string> command_name = config["command"].value<std::string>();
  if (! command_name) {
    throw std::runtime_error("Missing required 'command' for menu action.");
  }
  menu_item = MenuItem.get(*command_name);
  if (! menu_item) {
    throw std::runtime_error("Undefined command type for menu action: " + *command_name);
  }

  value = config["value"].value_or(0);


}


void MenuAction::doAction() {
  Sequence seq;
  // open the main menu
  seq.addSequence(menu_open);
  switch (type) {
    case MenuActionType::SET:
      addMenuSelect(seq);
      break;
    case MenuActionType::RESTORE;
      addMenuRestore(seq);
      break
  }
  // put menu back at top

  seq.send();
}

void MenuAction::addSetMenuOption(Sequence& seq) {
  assert(menu_item->isOption());
  int difference = value - item->getState();

  if (!menu_option_less) {
    throw std::runtime_error("No sequence defined for 'menu_option_less'");
  }
  if (!menu_option_greater) {
    throw std::runtime_error("No sequence defined for 'menu_option_greater'");
  }
  // scroll to the appropriate option
  for (int i = 0; i < difference; i++) {
    seq.addSequence(menu_option_greater);
  }
  for (int i = 0; i > difference; i--) {
    seq.addSequence(menu_option_less);
  }
  if (item->toConfirm()) {
    seq.addSequence(menu_confirm);
  }
}

// TODO: implement xgroups and child selection value
void MenuAction::addMenuSelect(Sequence& seq) {
  std::stack<std::shared_ptr<MenuItem>> menu_stack;

  for (auto s =  menu_item; s->parent; s = menu_item->parent) {
    menu_stack.push(s);
  }
  while (!menu_stack.empty()) {
    auto item = menu_stack.top();
    // navigate down or up in the menu list
    if (!menu_down) {
      throw std::runtime_error("No sequence defined for 'menu_down'");
    }
    if (!menu_up) {
      throw std::runtime_error("No sequence defined for 'menu_up'");
    }
    // navigate down for positive offsets
    for (int i = 0; i < item->getOffset(); i++) {
      seq.addSequence(menu_down);
    }
    for (int i = 0; i > item->getOffset(); i--) {
      seq.addSequence(menu_up);
    }
    if (item->isOption()) {
      // we're at a leaf of the tree
      addSetMenuOption(seq);
    } else {
      
      // For ordinary, submenu items, just select it and go to the next child menu
      seq.addSequence(menu_select);
      addDelay(menu_select_delay);
      if (item->toConfirm()) {
        seq.addSequence(menu_confirm);
      }
    }
    menu_stack.pop();
  }
  // reverse to navigate to top of each submenu
  auto s =  menu_item;
  do {
    // navigate down for positive offsets
    for (int i = 0; i < s->getOffset(); i++) {
      seq.addSequence(menu_up);
    }
    for (int i = 0; i > s->getOffset(); i--) {
      seq.addSequence(menu_down);
    }
    seq.addSequence(menu_back);
    s = menu_item->parent;
  } while (s->parent);

}

MenuAction::calculateOffsetCorrection() {

}
