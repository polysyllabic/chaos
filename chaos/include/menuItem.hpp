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
   * \brief Action to take after a counter change
   * 
   * The following options are currently supported:
   * - NONE: Do nothing (_Default_)
   * - REVEAL: The menu item is shown when the counter is non-zero, hidden when it is zero.
   * - ZERO_RESET:  
   */
  enum class CounterAction { NONE, REVEAL, ZERO_RESET};

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
   * the complete series of button presses necessary to navigate to that option.
   * 
   * A menu item has the following parameters defined in the configuration file:
   * - name: A unique name for this menu item (_Required_)
   * - type: The category of menu item (_Required_). The available types are:
   *     - menu: Item is a submenu that appears in a standard list. These items serve as parents
   * to other menu items.
   *     - command: Item executes some command (e.g., restart encounter) rather than allowing
   * a choice from a number of values. This type is a terminal leaf in the menu tree and
   * declare it as a parent of another menu item will generate an error when the configuration
   * file is parsed.
   *     - option: Item selects an option from among a discrete number of values. The selected
   * option is represented as an unsigned integer, where 0 represents the minimum possible
   * value achieved by executing the #option_less sequence until the first item in the list is
   * reached. A maximum value is not currently specified. We rely on the configuration file to
   * have correct values. This type is a terminal leaf of the menu structure and declaring it as
   * a parent of another menu item will generate an error when the configuration file is parsed.
   *     - guard: Item is a boolean option that protects options that appear beneath it in the
   * same menu. The subordinate options are only active if the guard option is true. Note that
   * if the guard is actually a sub-menu that must be selected to reveal the options, you should
   * implement that with the "guarded" option in the "menu" type. See the SubMenu class.
   *     .
   * - parent: The parent menu for this item. If omitted, this item belongs to the root (main)
   * menu. Parents must be declared before any children that reference them, and only items of
   * the menu type can serve as parents. (_Optional_)
   *- offset: The number of actions required to reach this item from the first item in the list.
   * Must be an integer. Positive values navigate down, negative values navigate up. If the
   * menu contains potentially hidden items, the offset should be the value required to reach the
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
   * - confirm: Setting this option requires a yes/no confirmation after setting/selection.
   * (_Optional_)
   * - initialState: The starting value for the menu item, expressed as an unsigned integer. For
   * menu items of the option and guard type, this value indicates the number of #option_greater
   * actions required to reach this value from the minimum value. When used with a menu item, the
   * initialState is interpreted as the currently selected child that has this item as a parent.
   * If omitted, the default value is 0. (_Optional_)
   * - guard: This item is guarded by a guard item of this name.
   * - counter: references a menu item and increments the counter for that menu item when the
   * mod referencing this item is initialized, and decrements the counter when the mod referencing
   * this item is finished. 
   * - counterAction: Action to take after this menu option's counter changes. The currently
   * supported values are:
   *     - reveal: Menu option is shown when the counter is non-zero, hidden when zero
   *     - zeroReset: Restore the menu value to default only when the counter is zero.
   *     .

   * Parent menu items and guards must be declared before they are referenced, but the definition
   * order is otherwise unconstrained.
   *
   * \todo Allow shortcut commands to invoke menus
   */
  class MenuItem : std::enable_shared_from_this<MenuItem> {
  protected:
    /**
     * Vertical navigation from menu top to reach this item
     */
    int offset;
    /**
     * Horizontal tab group that this item belongs to
     */
    int tab_group;

    /**
     * \brief Correction of the offset to account for hidden items
     *
     * This value is updated actively when menu items are hidden or revealed.
     */
    int offset_correction;

    /**
     * \brief The default state of the menu item before any changes
     * 
     * We use this to restore the menu to its original state after any menu modifier
     */
    int default_state;

    /**
     * Is the menu item currently hidden?
     */
    bool hidden;

    /**
     * The parent to this menu item
     * 
     * If the #parent is NULL, this item is part of the root (main) menu.
     */
    std::shared_ptr<MenuItem> parent;

    /**
     * \brief The guard for this item
     * 
     * If non-NULL, the item can only be set/selected if the guard item is set to true.
     */
    std::shared_ptr<MenuItem> guard;

    /**
     * \brief Pointer to another menu item whose counter is tied to this item
     * 
     * When non-null, setting this option will increment the sibling's counter and restoring the
     * state will decrement it.
     */
    std::shared_ptr<MenuItem> sibling_counter;

    // Type of action to take when the counter changes value
    CounterAction counter_action;

    /**
     * \brief The current state of the menu item.
     * 
     * For options and groups, this value is the 
     */
    unsigned int current_state;

    /**
     * \brief Internal counter for this item.
     * 
     * Mods can increment or decrement the item to track particular states
     */
    int counter;


  public:
    /**
     * \brief Construct a new Menu Item object
     * 
     * \param ofst Vertical navigation from menu top to reach this item
     * \param tab Horizontal tab group that this item belongs to
     * \param initial The initial state of the menu item before any changes
     * \param hide Initialize the menu item as hidden?
     * \param par The parent menu item to this one
     * \param grd The guard for this item
     * \param cnt The subling counter for this item
     * \param action Type of action to take when the counter changes value
     */
    MenuItem(toml::table& config,
             std::shared_ptr<MenuItem> par,
             std::shared_ptr<MenuItem> grd,
             std::shared_ptr<MenuItem> cnt);

    /**
     * \brief Returns #this as a shared pointer
     * 
     * \return std::shared_ptr<MenuItem> 
     *
     * Since the pointer for this object is managed by std::shared_ptr, using #this
     * directly is dangerous. Use this function instead.
     */
    std::shared_ptr<MenuItem> getptr() { return shared_from_this(); }

    /**
     * \brief Get the pointer to the parent MenuItem of this one
     * 
     * \return std::shared_ptr<MenuItem> 
     * \return NULL if this item is in the top-level (main) menu
     */
    std::shared_ptr<MenuItem> getParent() { return parent; }

    /**
     * \brief Add commands necessary to navigate to this item from the top of main menu
     * 
     * \param seq Sequence to which the commands should be appended
     *
     * This only creates the commands to move to the menu item. It does not select or set it.
     */
    void moveTo(Sequence& seq);

    /**
     * \brief Add commands necessary to navigate back to the top of the menu after selecting
     * 
     * \param seq Sequence to which the commands should be appended
     *
     * For games that leave the menu in the last state the user left it, we need to navigate
     * everything back to the top of each menu so the menu system is in a known state.
     */
    void navigateBack(Sequence& seq);

    /**
     * \brief Adds commands to moves to item within a single menu
     * 
     * \param seq The sequence to which the necessary commands will be appended
     * \param delta The number of steps to scroll in the menu to reach the appropriate item
     *
     * If the item is a menu, it will be selected. If it is an option, the cursor will be positioned
     * at this item but not set.
     */
    void selectItem(Sequence& seq, int delta);

    /**
     * \brief Get the offset between this menu item and the top object
     * 
     * \return int The offset with corrections for any hidden items
     */
    int getOffset() { return offset + offset_correction; }
    int getDefault() { return default_state; }
    
    void adjustOffset(int delta) { offset_correction += delta; }

    int getTab() { return tab_group; }

    virtual int getState() { return current_state; }
    
    virtual void setState(Sequence& seq, unsigned int new_state) = 0;
    virtual void restoreState(Sequence& seq) = 0;

    virtual bool isOption() = 0;
    virtual bool isSelectable() = 0;

    void setHidden(bool hide) { hidden = hide; }
    bool isHidden() { return hidden; }

    void incrementCounter();
    void decrementCounter();

    void setCounter(int val) { counter = val; }
    int getCounter() { return counter; }

    bool isGuarded() { return (guard != nullptr); }
    std::shared_ptr<MenuItem> getGuard() { return guard; }

  };
};
