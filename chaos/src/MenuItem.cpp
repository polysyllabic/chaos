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
#include <stack>
#include <toml++/toml.h>
#include <plog/Log.h>

#include "MenuItem.hpp"
#include "Configuration.hpp"
#include "GameMenu.hpp"

using namespace Chaos;

MenuItem::MenuItem(toml::table& config, std::shared_ptr<MenuItem> par,
                   std::shared_ptr<MenuItem> grd,
                   std::shared_ptr<MenuItem> cnt) :
                   parent{par}, guard{grd}, sibling_counter{cnt} {

  if (! config.contains("offset")) {
    PLOG_WARNING << "Menu item '" << config["name"] << "' missing offset. Set to 0.";
  }
  offset = config["offset"].value_or(0);
  tab_group = config["tab"].value_or(0);
  default_state = config["initialState"].value_or(0);
  current_state = default_state;
  hidden = config["hidden"].value_or(false);

  counter_action = CounterAction::NONE;
  std::optional<std::string> action_name = config["counterAction"].value<std::string>();
  if (action_name) {
    if (*action_name == "reveal") {
      counter_action = CounterAction::REVEAL;
    } else if (*action_name == "zeroReset") {
      counter_action = CounterAction::ZERO_RESET;
    } else if (*action_name != "none") {
      throw std::runtime_error("Unknown counterAction type: " + *action_name);
    }
  }
  PLOG_VERBOSE << "-- offset = " << offset << "; tab_group = " << tab_group <<
      "; initial_state = " << default_state << "; hidden = " << hidden;
}

void MenuItem::incrementCounter() {
  ++counter;
  if (counter_action == CounterAction::REVEAL && counter == 1) {
    hidden = false;
    GameMenu::instance().correctOffset(getptr());
  }
}

void MenuItem::decrementCounter() {
  if (counter) {
    --counter;
    if (counter == 0) {
      if (counter_action == CounterAction::REVEAL) {
        hidden = true;
        GameMenu::instance().correctOffset(getptr());
      }
    }
  }
}

void MenuItem::moveTo(Sequence& seq) {
  PLOG_VERBOSE << "moveTo: ";

  // Create a stack of all the parent menus of this option
  std::stack<std::shared_ptr<MenuItem>> menu_stack;
  for (auto s =  getptr(); s->parent != nullptr; s = s->parent) {
    menu_stack.push(s);
  }

  // Now create the navigation commands. Popping from the top of the stack starts us with the
  // top-level menu and works down to the terminal leaf.
  std::shared_ptr<MenuItem> item;
  while (!menu_stack.empty()) {
    item = menu_stack.top();
    item->selectItem(seq, offset);
    menu_stack.pop();
  }
}

void MenuItem::selectItem(Sequence& seq, int delta) {
    // navigate left or right through tab groups
    for (int i = 0; i < tab_group; i++) {
      PLOG_VERBOSE << "tab right";
      seq.addSequence(GameMenu::instance().getTabRight());
    }
    for (int i = 0; i > tab_group; i++) {
      PLOG_VERBOSE << "tab left";
      seq.addSequence(GameMenu::instance().getTabLeft());
    }

    // If this item is guarded, make sure the guard is enabled
    PLOG_VERBOSE << "- scroll " << delta;
    if (guard && guard->getState() == 0) {
      guard->setState(seq, 1);
      // adjust delta for the steps we've already made to get to the guard
      delta -= guard->getOffset();
      PLOG_VERBOSE << " - delta to guard: " << guard->getOffset() << " new delta:: " << delta;
    }

    // navigate down for positive offsets
    for (int i = 0; i < delta; i++) {
      PLOG_VERBOSE << "down";
      seq.addSequence(GameMenu::instance().getNavDown());
    }

    // navigate up for negative offsets
    for (int i = 0; i > delta; i--) {
      PLOG_VERBOSE << "up ";
      seq.addSequence(GameMenu::instance().getNavUp());
    }

    // Submenus and select option items items require a button press
    if (isSelectable()) {
      PLOG_VERBOSE << "select";
      seq.addSequence(GameMenu::instance().getSelect());
      seq.addDelay(GameMenu::instance().getSelectDelay());
    }
}

void MenuItem::navigateBack(Sequence& seq) {
  // starting from the selected option, we reverse to navigate to top of each submenu and leave it
  // in a consistent state for the next time we open the menu
  PLOG_VERBOSE << "navigateBack: ";
  std::shared_ptr<MenuItem> item = getptr();
  do {
    for (int i = 0; i < item->getOffset(); i++) {
      PLOG_VERBOSE << " up ";
      seq.addSequence(GameMenu::instance().getNavUp());
    }
    for (int i = 0; i > item->getOffset(); i--) {
      PLOG_VERBOSE << " down ";
      seq.addSequence(GameMenu::instance().getNavDown());
    }
    for (int i = 0; i < item->getTab(); i++) {
      PLOG_VERBOSE << " tab-left ";
      seq.addSequence(GameMenu::instance().getTabLeft());
    }
    for (int i = 0; i > item->getTab(); i++) {
      PLOG_VERBOSE << " tab-right ";
      seq.addSequence(GameMenu::instance().getTabRight());
    }
    seq.addSequence(GameMenu::instance().getBack());
    item = parent;
  } while (item != nullptr);

}

