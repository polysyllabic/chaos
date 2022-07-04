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
#include <unordered_map>
#include <string>
#include <json/json.h>
#include "Modifier.hpp"
#include "EngineInterface.hpp"

namespace Chaos {

  /**
   * \brief Container class for all available modifiers
   * 
   */
  class ModifierTable {

    /**
     * The map of all the mods defined through the TOML file
     */
    std::unordered_map<std::string, std::shared_ptr<Modifier>> mod_map;

  public:
    /**
     * \brief Create the overall list of mods from the TOML file.
     * \param config The object containing the fully parsed TOML file
     * \param engine Reference to the engine interface
     * \param use_menu If false, do not use menu modifiers
     */
    int buildModList(toml::table& config, EngineInterface* engine, bool use_menu);

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Modifier> getModifier(const std::string& name);

    std::unordered_map<std::string, std::shared_ptr<Modifier>>& getModMap() { return mod_map; }

    int getNumModifiers() { return mod_map.size(); }

    /**
     * Return list of modifiers for the chat bot.
     */
    Json::Value getModList();

  };
};