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

#include "MenuInterface.hpp"
#include "Sequence.hpp"

namespace Chaos {

  /**
   * \brief Action to take after a counter change
   * 
   * The following options are currently supported:
   * - NONE: Do nothing (_Default_)
   * - REVEAL: The menu item is shown when the counter is non-zero, hidden when it is zero.
   * - ZERO_RESET: When the counter reaches zero, reset this item to its default state
   */
  enum class CounterAction { NONE, REVEAL, ZERO_RESET};

  /**
   * \brief An object to hold a single entry in the game's menu system
   *
   * In order to support modifiers that alter game play by changing basic game settings, we need
   * to track the structure of the menu system. We do this by defining the a menu as a series of
   * MenuItem objects. In the configuration file, these are described as an array of inline
   * tables.
   *
   * Menu modifiers reference the menu option they want to invoke and the engine will calculate
   * the complete series of button presses necessary to navigate to that option.
   * 
   * The TOML syntax defining menu items is described in chaosConfigFiles.md
   *
   */
  class MenuItem : public std::enable_shared_from_this<MenuItem> {
  protected:
    MenuInterface& menu_items;

    std::string name;

    /**
     * Vertical navigation from menu top to reach this item
     */
    short offset;

    /**
     * Horizontal tab group that this item belongs to
     */
    short tab_group;

    /**
     * \brief Correction of the offset to account for hidden items
     *
     * This value is updated actively when menu items are hidden or revealed.
     */
    short offset_correction;

    /**
     * \brief The default state of the menu item before any changes
     * 
     * We use this to restore the menu to its original state after any menu modifier
     */
    short default_state;

    /**
     * Is the menu item currently hidden?
     */
    bool hidden;

    /**
     * \brief The parent to this menu item
     * 
     * If the parent is NULL, this item is part of the root (main) menu.
     */
    std::shared_ptr<MenuItem> parent{nullptr};

    /**
     * \brief The guard for this item
     * 
     * If non-NULL, the item can only be set/selected if the guard item is set to true.
     */
    std::shared_ptr<MenuItem> guard{nullptr};

    /**
     * \brief Pointer to another menu item whose counter is tied to this item
     * 
     * When non-null, setting this option will increment the sibling's counter and restoring the
     * state will decrement it.
     */
    std::shared_ptr<MenuItem> sibling_counter{nullptr};

    /**
     * Type of action to take when the counter changes value
     */
    CounterAction counter_action;

    /**
     * The current state of the menu item
     * 
     */
    short current_state;

    /**
     * \brief Internal counter for this item
     * 
     * Mods can increment or decrement the item to track particular states.
     */
    short counter;

    /**
     * \brief Confirm yes after selection
     *
     * True if selecting this item requires a confirmation prompt to activate.
     */
    bool confirm;

    /**
     * Is the menu item an option type?
     */
    bool is_option;

    /**
     * Will selecting the item currently have any effect?
     */
    bool is_selectable;


    void setMenuOption(Sequence& seq, unsigned int new_val);

  public:
    /**
     * \brief Construct a new Menu Item object
     * 
     * \param menu Reference to the menu table
     * \param name Name of this menu entry
     * \param off Offset
     * \param tab Tab group
     * \param initial Initial value
     * \param hide Start hidden
     * \param opt Is an option
     * \param sel Is selectable
     * \param conf Confirm on selection
     * \param par Parent of item
     * \param grd Guard for item
     * \param cnt Associated counter for item
     * \param act Action on counter change
     */
    MenuItem(MenuInterface& menu, std::string name, short off, short tab,
             short initial, bool hide, bool opt, bool sel, bool conf,
             std::shared_ptr<MenuItem> par,
             std::shared_ptr<MenuItem> grd,
             std::shared_ptr<MenuItem> cnt,
             CounterAction act);

    /**
     * \brief Returns 'this' as a shared pointer
     * 
     * \return std::shared_ptr<MenuItem> 
     *
     * Since the pointer for this object is managed by std::shared_ptr, using 'this'
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
     * \brief Creates the sequence of commands necessary to navigate to the top of the menu after
     * selecting it.
     * 
     * \param seq Sequence to which the commands should be appended
     *
     * For games that leave the menu in the last state the user left it, we need to navigate
     * everything back to the top of each menu so the menu system is in a known state. If a game
     * always starts you in the same place with the menus, this is unnecessary.
     */
    void navigateBack(Sequence& seq);

    /**
     * \brief Adds commands to moves to item within a single menu
     * 
     * \param seq The sequence to which the necessary commands will be appended
     *
     * If the item is a menu, it will be selected. If it is an option, the cursor will be positioned
     * at this item but not set.
     */
    void selectItem(Sequence& seq);

    /**
     * \brief Get the name of this menu item by which it is referenced in the TOML file
     * 
     * \return std::string& 
     */
    std::string& getName() { return name; }
    
    /**
     * \brief Get the offset between this menu item and the top object
     * 
     * \return int The offset with corrections for any hidden items
     */
    short getOffset() { return offset + offset_correction; }

    /**
     * \brief Get the default state of the menu item
     * 
     * This lets us return the game settings to their earlier state when we tear down the mod.
     * 
     * \return short 
     */
    short getDefault() { return default_state; }
    
    void adjustOffset(int delta) { offset_correction += delta; }

    /**
     * \brief Get the tab group to which this menu item belongs
     * 
     * \return short 
     */
    short getTab() { return tab_group; }

    /**
     * \brief Get the the current state of the menu item
     * 
     * \return short 
     */
    short getState() { return current_state; }
    
    /**
     * \brief Set the current state of the State object
     * 
     * \param seq The sequence to issue in order to change the state
     * \param new_state 
     * \param restore 
     */
    void setState(Sequence& seq, unsigned int new_state, bool restore);

    
    bool isOption() { return is_option; }

    bool isSelectable() { return is_selectable; }

    void setHidden(bool hide) { hidden = hide; }
    bool isHidden() { return hidden; }

    /**
     * \brief Increase the value of the counter by one
     * 
     * If this item has the counter action REVEAL associated with it, it will be unhidden when
     * the counter goes from 0 to 1.
     */
    void incrementCounter();

    /**
     * \brief Decrease the value of the counter by one
     * 
     * If this item has the counter action REVEAL associated with it, it will be hidden when the
     * counter goes from 1 to 0.
     */
    void decrementCounter();

    /**
     * \brief Set the counter to an arbitrary value
     * 
     * \param val New value for the counter
     * 
     * If this item has the counter action REVEAL associated with it, it will be unhidden when the
     * counter goes from 0 to a non-zero value and hidden when the counter goes to 0 from a non-zero
     * value.
     */
    void setCounter(int val);

    /**
     * \brief Get the current state of the counter for this item
     * 
     * \return int 
     */
    int getCounter() { return counter; }

    /**
     * \brief Is this menu item guarded by another menu item
     */
    bool isGuarded() { return (guard != nullptr); }

    /**
     * \brief Get the menu item that is guarding this one
     * 
     * \return std::shared_ptr<MenuItem> 
     */
    std::shared_ptr<MenuItem> getGuard() { return guard; }

  };
};
