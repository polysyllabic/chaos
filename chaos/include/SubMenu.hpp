/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
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
#include "MenuInterface.hpp"

namespace Chaos {

  /**
   * \brief Implements a menu item that selects a subordinate set of menu items.
   * 
   * 
   */
  class SubMenu : public MenuItem {
  protected:
    /**
     * \brief Value of the last child item selected
     */
    int last_child_selected;

  public:
    SubMenu(std::shared_ptr<MenuInterface> menu);

    void setState(Sequence& seq, unsigned int new_val);
    void restoreState(Sequence& seq);

    bool isOption() { return false; }
    bool isSelectable() { return true; }
  };

};