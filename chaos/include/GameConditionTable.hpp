/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file at the top-level directory of this distribution for details of the
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
#include <memory>
#include <string>

#include "GameCondition.hpp"
#include "GameCommandTable.hpp"

namespace Chaos {

  class GameConditionTable {

    /**
     * The map of game conditions identified by their names in the TOML file.
     */
    std::unordered_map<std::string, std::shared_ptr<GameCondition>> condition_map;


  public:
    /**
     * \brief Given the GameCondition name, get the object
     * 
     * \param name The name by which this GameCondition is identified in the TOML file
     * \return std::shared_ptr<GameCondition> Pointer to the GameCondition object.
     *
     * Conditions refer to game commands and game command can, optionally, refer to conditions. To
     * avoid infinite recursion, we prohibit conditions from referencing game commands that have
     * conditions themselves.
     */
    std::shared_ptr<GameCondition> getCondition(const std::string& name);

    /**
     * \brief Initializes the list of conditions from the TOML file.
     * 
     * \param config The object holding the parsed TOML file
     */
    int buildConditionList(toml::table& config, GameCommandTable& commands);


  };
};
