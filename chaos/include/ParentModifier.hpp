/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
#include <string>
#include <toml++/toml.h>

#include "Modifier.hpp"
#include "EngineInterface.hpp"

namespace Chaos {
  class Game;
  /**
   * \brief A modifier that enables child modifiers, either from a specific list or randomly selected.
   * 
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "parent" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - children: A list of specific child modifiers
   * - random: A boolean. If true, selects value number of child mods at random
   * 
   * Notes:
   * During the lifetime of the mod, child modifiers listed in _children_ will be processed in the order
   * that they are specified in the TOML file. If both specific child mods and random mods are specified,
   * the random mods will be processed after the explicit child mods.
   */
  class ParentModifier : public Modifier::Registrar<ParentModifier> {
  protected:
    std::vector<std::shared_ptr<Modifier>> fixed_children;
    std::vector<std::shared_ptr<Modifier>> random_children;
    
    bool random_selection;
    short num_randos;

    void buildRandomList();
  public:
    ParentModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    bool randomSelection() { return random_selection; }
    
    void begin();
    void update();
    void finish();
    bool tweak(DeviceEvent& event);
  };
};
