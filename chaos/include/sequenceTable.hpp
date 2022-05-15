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

#include "sequence.hpp"

namespace Chaos {

  /**
   * \brief Container for pre-defined sequences and global parameters relating to sequences
   * 
   */
  class SequenceTable {
  public:
    int buildSequenceList(toml::table& config);

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Sequence> getSequence(const std::string& name);

    unsigned int timePerPress() { return press_time; }

    /**
     * \brief Set the time to hold button down in a press sequence step
     * 
     * \param time Press time in seconds
     */
    void setPressTime(double time);
   
    unsigned int timePerRelease() { return release_time; }

    /**
     * \brief Set the time to release button before moving on to the next step
     * 
     * \param time Press time in seconds
     */
    void setReleaseTime(double time);

    std::shared_ptr<Sequence> makeSequence(toml::table& config, const std::string& key);
  private:
    /**
     * Time in microseconds to hold down a signal for a button press
     */
    unsigned int press_time;

    /**
     * Time in microseconds to release a signal for a button press before going on to the
     * next command.
     */
    unsigned int release_time;

    /**
     * The map of pre-defined sequences identified by their names in the TOML file.
     */
    std::unordered_map<std::string, std::shared_ptr<Sequence>> sequence_map;

    
  };
};