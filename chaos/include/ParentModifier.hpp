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
   * \brief A modifier that contains child modifiers
   * 
   * A parent modifier is a modifier that executes one or more other modifiers (child modifiers).
   * The children can be drawn from a specific list or selected at random. Working from a specified
   * list lets you create modifiers by chaining together multiple modifiers of different types to
   * achieve effects that would not be possible within a single modifier class.
   * 
   * The TOML syntax defining menu items is described in chaosConfigFiles.md
   * 
   * Child modifiers in #fixed_children will be processed in the same order that they are specified
   * in the TOML file. If both named child mods and random mods are specified, the random mods
   * will be processed after all the mods specified in #fixed_children.
   */
  class ParentModifier : public Modifier::Registrar<ParentModifier> {
  protected:
    /**
     * Vector of mods that were explicitly identified in the configuration file
     */
    std::vector<std::shared_ptr<Modifier>> fixed_children;

    /**
     * Vector of mods chosen at random
     */
    std::vector<std::shared_ptr<Modifier>> random_children;

    /**
     * \brief Number of random modifiers chosen
     * 
     */
    short num_randos;

    void buildRandomList();
  public:
    ParentModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    void begin();
    void update();
    void finish();
    bool tweak(DeviceEvent& event);
  };
};
