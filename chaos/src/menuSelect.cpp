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

#include "menuSelect.hpp"
#include "configuration.hpp"
#include "gameMenu.hpp"

using namespace Chaos;

MenuSelect::MenuSelect(const toml::table& config) : MenuItem(config) {
  Configuration::checkValid(config, std::vector<std::string>{
      "name", "type", "parent", "offset", "counter", "counterAction", "guard",
      "hidden", "confirm", "tab"});

  std::optional<std::string> item_name = config["counter"].value<std::string>();
  if (item_name) {
    sibling_counter = GameMenu::instance().getMenuItem(*item_name);
    if (! sibling_counter) {
      // an error, but not a fatal one, so don't throw
      PLOG_ERROR << "Unknown counter '" << *item_name << "' for menu item " << config["name"];
    } else {
      PLOG_DEBUG << "Counter = " << *item_name;
    }
  }
  confirm = config["confirm"].value_or(false);
}

void MenuSelect::setState(Sequence& seq, unsigned int new_val) {
  PLOG_VERBOSE << "setState ";
  moveTo(seq);
  seq.addSequence(GameMenu::instance().getSelect());
  if (confirm) {
    seq.addSequence(GameMenu::instance().getConfirm());
  }
  // Increment the counter, if set
  if (sibling_counter) {
    sibling_counter->incrementCounter();
    PLOG_VERBOSE << "Incrementing group counter";
  }
  navigateBack(seq);
}

// Restore state only makes sense for select options when they are a series. This means that all
// items that use the same counter should be exclusive options (selecting one automatically
// deselects another). We don't need to do anything until we remove the final mod that changes
// something in this group and we need to restore the original state. That original state should be
// stored as the default_state (initialState in the TOML file) of the counter item.
void MenuSelect::restoreState(Sequence& seq) {
  PLOG_VERBOSE << "restoreState ";
  if (sibling_counter) {
    sibling_counter->decrementCounter();
    PLOG_VERBOSE << "Decrementing gorup counter";
    if (sibling_counter->getCounter() == 0) {
      // select the parent item (a submenu) and move to the initial state setting stored by the
      // counter item.
      parent->moveTo(seq);
      selectItem(seq, sibling_counter->getDefault());
      navigateBack(seq);
    }
  }
}

