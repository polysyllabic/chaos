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
#include "GameCommand.hpp"
#include "ControllerInputTable.hpp"

namespace Chaos {

  class GameCommandTable {

   /**
     * The map of game commands identified by their names in the TOML file
     */
    std::unordered_map<std::string, std::shared_ptr<GameCommand>> command_map;

  public:

    /**
     * \brief Accessor to GameCommand pointer by command name.
     *
     * \param name Name by which the game command is identified in the TOML file.
     * \return The GameCommand pointer for this command, or NULL if not found.
     */
    std::shared_ptr<GameCommand> getCommand(const std::string& name);

    /**
     * \brief Initialize the global map to hold the command definitions
     *
     * \param config The object containing the complete parsed TOML file
     * \param signals Reference to 
     * This routine is called while parsing the game configuration file.
     */
    int buildCommandList(toml::table& config, ControllerInputTable& signals);

  };
};