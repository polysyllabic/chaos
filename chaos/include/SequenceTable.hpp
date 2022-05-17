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
#include <toml++/toml.h>

#include "Sequence.hpp"
#include "GameCommandTable.hpp"

namespace Chaos {
  class Controller;

  /**
   * Container for pre-defined sequences
   */
  class SequenceTable {
    /**
     * The map of pre-defined sequences identified by their names in the TOML file.
     */
    std::unordered_map<std::string, std::shared_ptr<Sequence>> sequence_map;

    std::shared_ptr<Sequence> makeSequence(toml::array* event_list, GameCommandTable& commands,
                                           Controller& controller);

  public:
    int buildSequenceList(toml::table& config, GameCommandTable& commands,
                          Controller& controller);

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Sequence> getSequence(const std::string& name);

    std::shared_ptr<Sequence> makeSequence(toml::table& config, 
                                           const std::string& key,
                                           GameCommandTable& commands,
                                           Controller& controller,
                                           bool required);

    /**
     * \brief Append sequence to the end of the current one by name
     * 
     * \param seq The sequence that is being created
     * \param name The name of the defined sequence as given in the TOML file
     */
    void addSequence(Sequence& seq, const std::string& name);

    void addDelay(Sequence& sequence, unsigned int delay) { sequence.addDelay(delay); }
    
  };
};