/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
#include <vector>
#include <toml++/toml.h>

#include "menuItem.hpp"

namespace Chaos {

  class MenuAction {
  private:
    std::shared_ptr<MenuItem> menu_item;
    int value;

    /**
     * \brief Recalculate the correction, if any, to the menu offset, based on any hidden menu items
     * that might appear before this item in the list.
     *
     * This only needs to be recalculated any time we hide or reveal aa menu item
     */
    void calculateOffsetCorrection();

  protected:
    /**
     * \brief Add the button presses necessary to select a menu option to a sequence
     */
    void addMenuSelect(Sequence& seq);

    /**
     * \brief Add the button presses necessary to restore the menu structure to the state before
     * the item was selected.
     * 
     * \param menu_item The name of the menu item defined in the configuration file
     */
    void addMenuRestore(Sequence& seq);


  public:
    MenuAction(toml::table& config);

    virtual void doAction();

  };

};