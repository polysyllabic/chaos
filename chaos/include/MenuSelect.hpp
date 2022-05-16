/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <toml++/toml.h>

#include "MenuItem.hpp"

namespace Chaos {

  /**
   * \brief Implements a menu item that is triggered by selecting.
   * 
   * Select options are teminal nodes of the menu system, but they do not maintain a multivalued
   * state. Executing a select option triggers the game to do something, such as restart a
   * checkpoint. If a select option has an associated counter, it is interpreted as being one
   * of a set of mutually exclusive options 
   * 
   * \todo Add support for max value checking
   */
  class MenuSelect : public MenuItem {
  protected:
  
    /**
     * \brief Confirm yes after selection
     *
     * True if selecting this item requires a confirmation prompt to activate.
     */
    bool confirm;

  public:
    MenuSelect(toml::table& config, std::shared_ptr<MenuInterface> menu);

    void setState(Sequence& seq, unsigned int new_state);
    
    void restoreState(Sequence& seq);

    bool isOption() { return true; }
    bool isSelectable() { return true; }
  };

};