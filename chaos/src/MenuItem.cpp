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
#include "MenuInterface.hpp"

using namespace Chaos;

MenuItem::MenuItem(MenuInterface& menu, std::string mname, short off, short tab,
                   short initial, bool hide, bool opt, bool sel, bool conf,
                   std::shared_ptr<MenuItem> par,
                   std::shared_ptr<MenuItem> grd,
                   std::shared_ptr<MenuItem> cnt) : menu_items{menu},
                   name{mname}, 
                   offset{off}, tab_group{tab}, default_state{initial}, 
                   hidden{hide}, is_option{opt}, is_selectable{sel}, confirm{conf},
                   parent{par}, guard{grd}, sibling_counter{cnt} {
  current_state = default_state;
}

void MenuItem::incrementCounter() {
  ++counter;
  if (counter_action == CounterAction::REVEAL && counter == 1) {
    hidden = false;
    menu_items.correctOffset(getptr());
  }
}

void MenuItem::decrementCounter() {
  if (counter) {
    --counter;
    if (counter == 0) {
      if (counter_action == CounterAction::REVEAL) {
        hidden = true;
        menu_items.correctOffset(getptr());
      }
    }
  }
}

void MenuItem::moveTo(Sequence& seq) {
  PLOG_VERBOSE << "Building moveTo sequence";

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

void MenuItem::selectItem(Sequence& seq, short delta) {
    // navigate left or right through tab groups
    for (int i = 0; i < tab_group; i++) {
      PLOG_VERBOSE << "tab right";
      menu_items.addSequence(seq, "tab right");
    }
    for (int i = 0; i > tab_group; i++) {
      PLOG_VERBOSE << "tab left";
      menu_items.addSequence(seq, "tab left");
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
      menu_items.addSequence(seq, "menu down");
    }

    // navigate up for negative offsets
    for (int i = 0; i > delta; i--) {
      PLOG_VERBOSE << "up ";
      menu_items.addSequence(seq, "menu up");
    }

    // Submenus and select option items items require a button press
    if (isSelectable()) {
      PLOG_VERBOSE << "select";
      menu_items.addSequence(seq, "menu select");
      menu_items.addSelectDelay(seq);
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
      menu_items.addSequence(seq, "menu up");
    }
    for (int i = 0; i > item->getOffset(); i--) {
      PLOG_VERBOSE << " down ";
      menu_items.addSequence(seq, "menu down");
    }
    for (int i = 0; i < item->getTab(); i++) {
      PLOG_VERBOSE << " tab left ";
      menu_items.addSequence(seq, "tab left");
    }
    for (int i = 0; i > item->getTab(); i++) {
      PLOG_VERBOSE << " tab right ";
      menu_items.addSequence(seq, "tab right");
    }
    menu_items.addSequence(seq, "menu exit");
    item = parent;
  } while (item != nullptr);

}

void MenuItem::setState(Sequence& seq, unsigned int new_val) {
  PLOG_VERBOSE << "setState to " << new_val;
  moveTo(seq);

  if (is_option) {
    setMenuOption(seq, new_val);
    current_state = new_val;
  }
  if (is_selectable) {
    menu_items.addSequence(seq, "menu select");
  }
  if (confirm) {
    menu_items.addSequence(seq, "confirm");
  }

  // Increment the counter, if set
  if (sibling_counter) {
    sibling_counter->incrementCounter();
    PLOG_VERBOSE << "Incrementing sibling counter";
  }
  navigateBack(seq);
}

void MenuItem::restoreState(Sequence& seq) {
  PLOG_VERBOSE << "restoreState ";
  setMenuOption(seq, default_state);
  current_state = default_state;
  // Decrement the counter, if set
  if (sibling_counter) {
    sibling_counter->decrementCounter();
    PLOG_VERBOSE << "Decrementing sibling counter";
  }
  navigateBack(seq);
}

void MenuItem::setMenuOption(Sequence& seq, unsigned int new_val) {
  int difference = new_val - current_state;
  // scroll to the appropriate option
  PLOG_VERBOSE << "Setting option: difference = " << difference;
  for (int i = 0; i < difference; i++) {
    menu_items.addSequence(seq, "option greater");
  }
  for (int i = 0; i > difference; i--) {
    menu_items.addSequence(seq, "option less");
  }
  if (confirm) {
    menu_items.addSequence(seq, "confirm");
  }
}
