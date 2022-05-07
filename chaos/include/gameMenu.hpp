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

#include "menuItem.hpp"
#include "sequence.hpp"

namespace Chaos {

  /**
   * \brief Defines the game's menu system.
   * 
   * This class provides a mechanism to select anything defined by the game menu. It is the central
   * container for sub-menus and menu items.
   */
  class GameMenu {
  protected:
    std::unordered_map<std::string, std::shared_ptr<MenuItem>> menu;

    /**
     * \brief Does the game menu remember the last position that the user left the menu in?
     *
     * If true, we must reset each menu we enter to the top item (offset 0) before exiting, so that
     * we are in a known position when we open the menu the next time. Note: we do it this way rather
     * than remembering the position ourselves because the user may pause the game and use menu options
     * themselves. In that circumstance, it's more practical for the user to remember to leave everything
     * at the top rather than to try to remember and restore whatever position the menus were in when the
     * pause occurred.
     */
    bool remember_last;

    /**
     * \brief Should we treat items protected by a guard as hidden when the guard is off?
     * 
     * The values of menu options protected by a guard cannot be changed unless the guard is set to true.
     * If this flag is true, those protected items are also hidden. That is, the user cannot navigate to them,
     * and scrolling down from the guard will skip over all the guarded items. Note that "hide" doesn't necessarily
     * mean that the items are invisible to the user, just that they can't be navigated to.
     */
    bool hide_guarded;

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
     * \brief Actions necessary to exit the menu immediately
     */
    std::shared_ptr<Sequence> menu_exit;

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

    GameMenu() {}

    static GameMenu& instance() {
      static GameMenu menu{};
      return menu;
    }

    /**
     * \brief Initialize the menu from the TOML file
     * 
     * \param config The main table for the complete parsed TOML file
     */
    void initialize(toml::table& config);

    /**
     * \brief Get a pre-defined sequence by name
     * 
     * \param config 
     * \param name 
     * \return std::shared_ptr<Sequence> 
     */
    std::shared_ptr<Sequence> getDefinedSequence(const toml::table& config, const std::string& name);

    /**
     * \brief Look up the defined MenuItem by name
     * 
     * \param name Name by which this menu item is referenced in the configuration file
     * \return std::shared_ptr<MenuItem> 
     * \return NULL if no item has been defined for this name.
     */
    std::shared_ptr<MenuItem> getMenuItem(const std::string& name);

    std::shared_ptr<Sequence> getBack() {
      assert (!back->empty());
      return back;
    }

    std::shared_ptr<Sequence> getTabLeft() {
      assert (!tab_right->empty());
      return tab_right;
    }

    std::shared_ptr<Sequence> getTabRight() {
      assert (!tab_left->empty());
      return tab_left;
    }

    std::shared_ptr<Sequence> getNavDown() {
      assert (!nav_down->empty());
      return nav_down;
    }

    std::shared_ptr<Sequence> getNavUp() {
      assert (!nav_up->empty());
      return nav_up;
    }

    std::shared_ptr<Sequence> getOptionGreater() {
      assert (!option_greater->empty());
      return option_greater;
    }

    std::shared_ptr<Sequence> getOptionLess() {
      assert (!option_less->empty());
      return option_less;
    }

    std::shared_ptr<Sequence> getSelect() {
      assert (!select->empty());
      return select;
    }

    std::shared_ptr<Sequence> getConfirm() {
      assert (!confirm->empty());
      return confirm;
    }

    unsigned int getSelectDelay() { return select_delay; }

    /**
     * \brief Sets a menu item to the specified value
     * \param item The menu item to change
     * \param new_val The new value of the item
     *
     * The menu item must be settable (i.e., not a submenu)
     */
    void setState(std::shared_ptr<MenuItem> item, unsigned int new_val);

    /**
     * \brief Restores a menu to its default state
     * \param item The menu item to restore
     *
     * The menu item must be settable (i.e., not a submenu)
     */
    void restoreState(std::shared_ptr<MenuItem> item);

    /**
     * \brief Update the offset correction when menu items are hidden or revealed
     * \param The item whose visibility has changed
     *
     * This function is called by the menu item that has been hidden or revealed, so we alter the
     * offset correction for all other items that share the same parent and tab group.
     */
    void correctOffset(std::shared_ptr<MenuItem> changed);
};
};