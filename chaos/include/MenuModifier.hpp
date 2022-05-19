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
#include <unordered_map>

#include <toml++/toml.h>

#include "Modifier.hpp"
#include "MenuItem.hpp"

namespace Chaos {

  /**
   * \brief A modifier that executes a particular commands in the game's menu system.
   *
   * Mods of this type are defined in the TOML configuration file with the following keys in
   * adition to the required name and description:
   */
  class MenuModifier : public Modifier::Registrar<MenuModifier> {
  private:
    /**
     * \brief Map of menu items and values
     * 
     * A mod can perform multiple actions in the menu, so we store the value for each menu item
     * in a map.
     */
    std::unordered_map<std::shared_ptr<MenuItem>,int> menu_items;

    /**
     * Should the menu option be restored to its previous value when the mod finishes?
     */
    bool reset_on_finish;

  public:
    static const std::string mod_type;
    
    MenuModifier(toml::table& config, EngineInterface* e);
    
    void begin();
    void finish();
    bool tweak(DeviceEvent* event);

    
  };
};
