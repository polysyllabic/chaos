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

#include "menuOption.hpp"
#include "configuration.hpp"
#include "gameMenu.hpp"

using namespace Chaos;

MenuOption::MenuOption(const toml::table& config)  : MenuItem(config) {
  Configuration::checkValid(config, std::vector<std::string>{
      "name", "type", "parent", "offset", "initialState", "hidden", "confirm", "tab", "guard",
      "counter", "counterAction"});

  std::optional<std::string> item_name = config["counter"].value<std::string>();
  if (item_name) {
    sibling_counter = GameMenu::instance().getMenuItem(*item_name);
    if (! sibling_counter) {
      // an error, but not a fatal one, so don't throw
      PLOG_ERROR << "Unknown counter '" << *item_name << "' for menu item " << config["name"];
    } else {
      PLOG_DEBUG << "    counter = " << *item_name;
    }
  }

 confirm = config["confirm"].value_or(false);

}

void MenuOption::setState(Sequence& seq, unsigned int new_val) {
  PLOG_VERBOSE << "setState ";
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
    seq.addSequence(GameMenu::instance().getOptionGreater());
  }
  for (int i = 0; i > difference; i--) {
    seq.addSequence(GameMenu::instance().getOptionLess());
  }
  if (confirm) {
    seq.addSequence(GameMenu::instance().getConfirm());
  }
}
