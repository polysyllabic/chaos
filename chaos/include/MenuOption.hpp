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
#include "Sequence.hpp"
#include "MenuInterface.hpp"

namespace Chaos {

  /**
   * \brief Implements a menu item that sets a specific option.
   * 
   * The state of an option is stored as an unsigned integer, where 0 is the minimum value (or off),
   * and the maximum value depends on the number of options available.
   * 
   * \todo Add support for max value checking
   */
  class MenuOption : public MenuItem {
  protected:
  
    /**
     * \brief Confirm yes after selection
     *
     * True if selecting this item requires a confirmation prompt to activate.
     */
    bool confirm;

    void setMenuOption(Sequence& seq, unsigned int new_val);

  public:
    MenuOption(toml::table& config, std::shared_ptr<MenuInterface> menu);
    
    void setState(Sequence& seq, unsigned int new_state);
    void restoreState(Sequence& seq);

    bool isOption() { return true; }
    bool isSelectable() { return false; }
  };

};