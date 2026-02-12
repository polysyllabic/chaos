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
#include <queue>
#include <toml++/toml.h>
#include <timer.hpp>
#include "Modifier.hpp"
#include "EngineInterface.hpp"

namespace Chaos {

  /** 
   * \brief Subclass of modifiers that cycles a command repeatedly throughout the lifetime of the
   * modifier.
   *
   * This is effectively a specialized sequence. It can be set to repeat unconditionally or to
   * begin only when triggered by a particular input condition.
   *
   * The TOML syntax defining menu items is described in chaosConfigFiles.md
   *
   * \todo Support cycling between an on-sequence and an of-sequence (defined in the config file)
   */
  class RepeatModifier : public Modifier::Registrar<RepeatModifier> {

  protected:
    
    double press_time;
    bool is_on;
    /**
     * Time in seconds to keep the command on.
     */
    double time_on;

    /**
     * Time in seconds to leave the command off.
     */

    double time_off;

    /**
     * Running count of how many times we've repeated the cycle
     */
    int repeat_count;

    /**
     * Total number of times to repeat the cycle
     */
    int num_cycles = 1;
    
    /**
     * Time in seconds to wait after one cycle ends before repeating the sequence.
     */
    double cycle_delay;

    /**
     * The values that we set the commands to while we're in the on state
     */
    std::vector<short> force_on;

    /**
     * The values that we set the commands to while we're in the off state
     */
    std::vector<short> force_off;

    /**
     * \brief Any commands in this list are blocked while the mod is active.
     *
     * \todo Add the ability to block only while command is in on state.
     */
    std::vector<std::shared_ptr<GameCommand>> block_while;
    
  public:

    RepeatModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    void begin();
    void update();
    bool tweak(DeviceEvent& event);
  };
};

