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
   * These sequences can be set to repeat unconditionally or to begin only when triggered by a
   * particular input condition.
   *
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "repeat" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - appliesTo: A commands affected by the mod. (_Required_)
   * - forceOn: A boolean flag that, if true, sets any incoming signals in the appliesTo list to
   * - timeOn: Time in seconds to apply the command. (_Required_)
   * - timeOff: Time in seconds to turn the command off. (_Required_)
   * - disableOnStart: A boolean flag indicating whether to zero the commands listed in appliesTo
   * during the begin() routine. (_Optional; default = false_)
   * - disableOnFinish: A boolean flag indicating whether to zero the commands listed in appliesTo
   * during the finish() routine. (_Optional; default = false_)
   * - repeat: Number of times to repeat the on/off event before starting a new cycle. (_Optional_)
   * - cycleDelay: Time in seconds to delay after one iteration of a cycle (all the repeats) before
   * starting the next cycle. (_Optional; default = 0_)
   * - blockWhileBusy: List of commands to block while the sequence is on.
   * - startSequence: A sequence of commands to execute once in the begin() routine. (_Optional_)
   * - finishSequence: A sequence of commands to execute once in the finish() routine. (_Optional_)
   *
   * Sequences are build up of individual commands. Each command is defined in an inline table. See
   * the SequenceModifier documentation for the format of commands.
   *
   */
  class RepeatModifier : public Modifier::Registrar<RepeatModifier> {

  protected:
    
    dseconds press_time;
    bool is_on;
    /**
     * Time in seconds to keep the command on.
     */
    dseconds time_on;

    /**
     * Time in seconds to leave the command off.
     */

    dseconds time_off;

    int repeat_count;
    int num_cycles = 1;
    /**
     * Time in seconds to wait after one cycle ends before repeating the sequence.
     */
    dseconds cycle_delay;

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

