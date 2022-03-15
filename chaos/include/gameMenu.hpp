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

#include "menuItem.hpp"
#include "sequence.hpp"

namespace Chaos {

  /**
   * \brief Defines the game's menu system.
   * 
   */
  class GameMenu {
  protected:

    std::unordered_map<std::string, std::shared_ptr<MenuItem>> menu;

    /**
     * \brief Does the game menu remember the last position that the user left the menu in?
     */
    bool remember_last;

    /**
     * \brief Time to wait, in microseconds, after disabling all control inputs
     */
    unsigned int disable_delay;

    /**
     * \brief Time to wait, in microseconds after selecting a menu item before issuing the next comand
     */
    unsigned int select_delay;

    /**
     * \brief Actions necessary to open the menu
     */
    std::shared_ptr<Sequence> open;

    /**
     * \brief Actions necessary to open go back one level in the menu
     */
    std::shared_ptr<Sequence> back;

    /**
     * \brief Actions necessary to move up one item in a list of menu options
     */
    std::shared_ptr<Sequence> nav_up;

    /**
     * \brief Actions necessary to move down one item in a list of menu options
     */
    std::shared_ptr<Sequence> nav_down;

    /**
     * \brief Actions necessary to move left between menu tabs (i.e., submenus that are selected in
     * a different way than the ordinary submenus are.
     */
    std::shared_ptr<Sequence> tab_left;

    /**
     * \brief Actions necessary to move right between menu tabs (i.e., submenus that are selected in
     * a different way than the ordinary submenus are.
     */
    std::shared_ptr<Sequence> tab_right;

    /**
     * \brief Actions necessary to select an individual menu item
     */
    std::shared_ptr<Sequence> select;

    /**
     * \brief Actions necessary to scroll through a multi-value menu option to increase its value
     * by one unit
     */
    std::shared_ptr<Sequence> option_greater;

    /**
     * \brief Actions necessary to scroll through a multi-value menu option to increase its value
     * by one unit
     */
    std::shared_ptr<Sequence> option_less;

    /**
     * \brief Actions necessary to confirm a menu option
     */
    std::shared_ptr<Sequence> confirm;

  public:
    /**
     * \brief Get the Defined Sequence object
     * 
     * \param config 
     * \param name 
     * \return std::shared_ptr<Sequence> 
     */
    std::shared_ptr<Sequence> getDefinedSequence(toml::table& config, std::string& name);

    /**
     * \brief Look up the defined MenuItem by name
     * 
     * \param name Name by which this menu item is referenced in the configuration file
     * \return std::shared_ptr<MenuItem> 
     * \return NULL if no item has been defined for this name.
     */
    std::shared_ptr<MenuItem> getMenuItem(const std::string& name);

  };
};