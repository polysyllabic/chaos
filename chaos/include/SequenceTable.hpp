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
#include<unordered_map>
#include <toml++/toml.h>
#include "Sequence.hpp"

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

  public:
    bool addDefinedSequence(const std::string& name, std::shared_ptr<Sequence> new_sequence);

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Sequence> getSequence(const std::string& name);

    /**
     * \brief Append a defined sequence to the end of another one by name
     * 
     * \param seq The sequence that is being created
     * \param name The name of the defined sequence as given in the TOML file
     */
    void addToSequence(Sequence& seq, const std::string& name);

    /**
     * \brief Append a delay to the end of a sequence
     * 
     * \param seq The sequence that is being created
     * \param name The name of the defined sequence as given in the TOML file
     */
    void addDelayToSequence(Sequence& sequence, double delay);
    
    /**
     * \brief Remove all existing defined sequences
     */
    void clearSequenceList();
  };
};