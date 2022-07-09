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
  PLOG_DEBUG << "increment counter for " << name;
  ++counter;
  if (counter_action == CounterAction::REVEAL && counter == 1) {
    hidden = false;
    menu_items.correctOffset(getptr());
  }
}

void MenuItem::decrementCounter() {
  PLOG_DEBUG << "decrement counter for " << name;
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

void MenuItem::selectItem(Sequence& seq) {
    // navigate left or right through tab groups
    int delta = offset;
    PLOG_DEBUG << name << " menu offset = " << offset;
    for (int i = 0; i < tab_group; i++) {
      menu_items.addSequence(seq, "tab right");
    }
    for (int i = 0; i > tab_group; i++) {
      menu_items.addSequence(seq, "tab left");
    }

    // If this item is guarded, make sure the guard is enabled
    if (guard && guard->getState() == 0) {
      // navigate to the guard
      guard->selectItem(seq);
      // enable guard item
      guard->setState(seq, 1, false);
      // adjust delta for the steps we've already made to get to the guard
      delta -= guard->getOffset();
      PLOG_VERBOSE << " - delta to guard: " << guard->getOffset() << " new delta:: " << delta;
    }

    // navigate down for positive offsets
    for (int i = 0; i < delta; i++) {
      menu_items.addSequence(seq, "menu down");
    }

    // navigate up for negative offsets
    for (int i = 0; i > delta; i--) {
      menu_items.addSequence(seq, "menu up");
    }

    // Submenus require a button press
    if (! isOption()) {
      menu_items.addSequence(seq, "menu select");
      menu_items.addSelectDelay(seq);
    }
}

void MenuItem::navigateBack(Sequence& seq) {
  // starting from the selected option, we reverse to navigate to top of the menu and
  // back out
  PLOG_DEBUG << "Navigate back offset " << offset;
  for (int i = 0; i < offset; i++) {
    menu_items.addSequence(seq, "menu up");
  }
  for (int i = 0; i > offset; i--) {
    menu_items.addSequence(seq, "menu down");
  }
  for (int i = 0; i < tab_group; i++) {
    menu_items.addSequence(seq, "tab left");
  }
  for (int i = 0; i > tab_group; i++) {
    menu_items.addSequence(seq, "tab right");
  }
  menu_items.addSequence(seq, "menu exit");
}

void MenuItem::setState(Sequence& seq, unsigned int new_val, bool restore) {
  PLOG_DEBUG << "Set state of " << name << " to " << new_val;

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
    if (restore) {
      sibling_counter->decrementCounter();
      PLOG_DEBUG << "Decrementing sibling counter " << sibling_counter->getName();
    } else {
      sibling_counter->incrementCounter();
      PLOG_DEBUG << "Incrementing sibling counter " << sibling_counter->getName();
    }
  }
}

void MenuItem::setMenuOption(Sequence& seq, unsigned int new_val) {
  int difference = new_val - current_state;
  // scroll to the appropriate option
  PLOG_DEBUG << "Setting option: difference = " << difference;
  for (int i = 0; i < difference; i++) {
    menu_items.addSequence(seq, "option greater");
  }
  for (int i = 0; i > difference; i--) {
    menu_items.addSequence(seq, "option less");
  }
}
