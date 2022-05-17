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

#include "MenuOption.hpp"
#include "TOMLUtils.hpp"
#include "MenuInterface.hpp"

using namespace Chaos;

MenuOption::MenuOption(toml::table& config, std::shared_ptr<MenuInterface> menu) : 
                       MenuItem(config, menu) {

 confirm = config["confirm"].value_or(false);

}

void MenuOption::setState(Sequence& seq, unsigned int new_val) {
  PLOG_VERBOSE << "setState to " << new_val;
  setMenuOption(seq, new_val);
  current_state = new_val;
  // Increment the counter, if set
  if (sibling_counter) {
    sibling_counter->incrementCounter();
    PLOG_VERBOSE << "Incrementing sibling counter";
  }
  navigateBack(seq);
}

void MenuOption::restoreState(Sequence& seq) {
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

void MenuOption::setMenuOption(Sequence& seq, unsigned int new_val) {
  moveTo(seq);
  int difference = new_val - current_state;
  // scroll to the appropriate option
  PLOG_VERBOSE << "Setting option: difference = " << difference;
  for (int i = 0; i < difference; i++) {
    menu_items->addSequence(seq, "option greater");
  }
  for (int i = 0; i > difference; i--) {
    menu_items->addSequence(seq, "option less");
  }
  if (confirm) {
    menu_items->addSequence(seq, "confirm");
  }
}
