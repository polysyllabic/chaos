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

#include "modifier.hpp"

namespace Chaos {

  enum class DisableFilter { ALL, ABOVE_THRESHOLD, BELOW_THRESHOLD };

  /**
   * \brief A modifier that disables a particullar list of commands from being passed to the
   * controller.
   *
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "disable" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - appliesTo: An array of commands affected by the mod. (_Required_)
   * - filter: The portion of the values to block. (_Optional_) The following values are legal:
   *     - all: All non-zero values are blocked, i.e., set to zero. (_Default_)
   *     - below: Any values less than the threshold are blocked
   *     - above: Any values greater than the threshold are blocked
   *     .
   * The default is 'all'.
   * - filterThreshold: The threshold state above or below which signals will be filtered.
   * The default is 0. (_Optional_)
   * - condition: Specifies a game command other than those in the appliesTo list. The appliesTo
   * commands will be blocked only if if the input signal of condition is equal to, above (for
   * positive
   *   thresholds) or below (for negative thresholds) the conditionThreshold.
   * - conditionThreshold: The default is 1.
   * - unless: Equivalent to 'condition', but the commands are disabled if the condition is NOT true.
   * - gamestate: Only take action if the specified gamestate is true.
   * - disableOnBegin: Game commands to disable (set to zero) when the mod is initialized.
   *  The value can be either an array of game commands or the single string "ALL". The latter
   *  is a shortcut to disable all defined game commands. (_Optional_)
   * - beginSequence: A sequence of commands to execute during the begin() routine. See 
   * SequenceModifier for an explanation of the TOML format for sequences. (_Optional_)
   * - finishSequence: A sequence of commands to execute during the finish() routine. See 
   * SequenceModifier for an explanation of the TOML format for sequences. (_Optional_)
   */
  class DisableModifier : public Modifier::Registrar<DisableModifier> {
  protected:
    /**
     * Limit above or below which values will be disabled
     */
    int filterThreshold;
    /**
     * What type of filtering to apply to the signal
     */
    DisableFilter filter;

  public:
    static const std::string name;
    
    /**
     * \brief The public constructor
     * \param config A TOML modifier-table object. If the constructor is properly dispatched, this
     * object will contain the key/value pair 'type=disable'.
     */
    DisableModifier(toml::table& config);

    // virtual routines we need to override
    void begin();
    void finish();
    bool tweak(DeviceEvent& event);

  };
};
