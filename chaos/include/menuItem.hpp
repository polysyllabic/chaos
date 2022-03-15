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

#include "sequence.hpp"

namespace Chaos {

  /**
   * \brief An object to hold a single entry in the game's menu system
   *
   * This is a virtual parent class. Different types of menu items are implemented as child
   * classes.
   *
   * In order to support modifiers that alter game play by changing basic game settings, we need
   * to track the structure of the menu system. We do this by defining the a menu as a series of
   * MenuItem objects. In the configuration file, these are described as an array of inline
   * tables.
   *
   * Menu modifiers reference the menu option they want to invoke and the engine will calculate
   * the complete series of button presses necessary to navigate to that option automatically.
   * 
   * A menu item has the following parameters defined in the configuration file:
   * - name: A unique name for this menu item (_Required_)
   * - type: The category of menu item (_Required_). The available types are:
   *     - menu: Item is an ordinary submenu that appears in a standard list. Selecting it will
   * go to that submenu.
   *     - tab: Item is a "tab" submenu, using a different navigation method from the ordinary
   * scrolling up and down. Tabs are parents for individual menu items.
   *     - xgroup: Item is an "exclusive group." This is an invisible parent for menu options
   * that are exclusive. In other words, there is no special line in the menu interface for
   * the xgroup. The items within the xgroup are part of the list in the parent menu, but they
   * are binary items that can only be set at a time, though all items in the group may be
   * off. The xgroup iself should have an offset equal to the number of menu presses required
   * to navigate to the first members of that xgroup. That first member should have an offset
   * of 0, and all other members of the xgroup should have their offset set relative to that
   * first member of the xgroup, not to the parent menu.
   *     - option: Item is an option.
   *     .
   * - parent: The parent menu for this item. If omitted, this item belongs to the root (main)
   * menu. (_Optional_)
   *- offset: The number of actions required to reach this item from the first item in the list.
   * Must be an integer. Positive values navigate down (or right for tab items), negative values
   * navigate up (or left for tab items). The offset should be the value required to reach the
   * item when all menu items are unhidden. The engine will adjust automatically for any hidden
   * items. (_Required_)
   * - tab: An integer indicating a group of items within a parent menu. Items in this group are
   * displayed together and you must navigate from tab to tab with a different command from the
   * normal menu up/down items. The value of the integer indicates the number of tab button
   * presses required to reach this group from the default tab. Positive values mean tab right,
   * negative values mean tab left. (_Optional_)
   * - hidden: A boolean value which, if true, indicates that the menu item is not displayed in
   * the list currently. Menu items can be hidden or revealed as a result of other actions in
   * the game. The default value is false. (_Optional_)
   * - confirm: For options, selecting this option requires a yes/no confirmation. (_Optional_)
   * - initialState: For options, the starting value for the menu item, expressed as an integer
   * indicating the number of button presses away from the minimum value required to reach this
   * state. (_Optional_)
   * - childState: For items of type menu or tab, setting this value to true indicates that we
   * should save the offset value of the last child item selected. This is used when the children
   * of a menu item constitute a list where only one can be selected at a time. (_Optional_)
   *
   * Parent menu items must be declared before child items, but the definition order is otherwise
   * unconstrained.
   *
   * \todo Allow shortcut commands to invoke menus
   *
   */
  class MenuItem {
  private:
    /**
     * \brief Vertical navigation from menu top to reach this item
     */
    int offset;

    /**
     * \brief Correction of the offset to account for hidden items
     */
    int offset_correction;

    /**
     * \brief The current state of the menu item.
     * 
     * The interpretation of this value depends on the type of the menu item:
     *
     * For options, it is the currently selected 
     */
    int state;

    /**
     * \brief The default state of the menu item before any changes
     * 
     */
    int default_state;

    /**
     * \brief Horizontal tab group that this item belongs to
     */
    int tab_group;

    /**
     * \brief Is the menu item currently hidden?
     */
    bool hidden;

    /**
     * \brief Which child option is selected (for groups)
     */
    int child_state;

    std::shared_ptr<MenuItem> parent;

    /**
     * \brief Confirm yes after selection
     */
    bool confirm;

    

  public:
    
    MenuItem(toml::table& config);
    

    /**
     * \brief Get the pointer to this MenuItem
     * 
     * \return std::shared_ptr<MenuItem> 
     */
    std::shared_ptr<MenuItem> get();

    /**
     * \brief Get the pointer to the parent MenuItem of this one
     * 
     * \return std::shared_ptr<MenuItem> 
     * \return NULL if this item is in the top-level (main) menu
     */
    std::shared_ptr<MenuItem> getParent();

    /**
     * \brief Get the offset between this menu item and the top object
     * 
     * \return int 
     */
    int getOffset() { return offset + offset_correction; }
    int getState() { return current_state; }
    void setState(int new_state) { current_state = new_state; }
    int getDefault() { return default_state; }
    void setDefault(int new_state) { default_state = new_state; }

    bool isOption() { return type == MenuItemType::OPTION; }
    bool toConfirm() { return confirm; }
    void hideItem();
    void revealItem();
    void calculateCorrection();
  };
};
