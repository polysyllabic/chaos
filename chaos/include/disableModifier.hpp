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
   * Mods of this type are defined in the TOML configuration file with the following keys in
   * adition to the required name and description:
   *
   * - type = "disable"
   * - appliesTo (required): An array listing the commands to be disabled. At least one command must
   *   be specified. If there is only one command, it should still be enclosed in array brackets.
   * - disableOnStart: boolean (optional). If true, sends a disable signal in the begin routine to
   *   any commands in the appliesTo list.
   * - filter (optional): Accepted values: all, below, above. Default = all. The filter defines
   *   what portion of the values to block. If this key is omitted, all non-zero signals will be
   *   blocked. If it is set to 'below' or 'above', only signals below or above the threshold value,
   *   respectively, will be blocked.
   * - filterThreshold (optional): The threshold state above or below which signals will be filtered.
   *   The default is 0.
   * - condition: Specifies a game command other than those in the appliesTo list. The appliesTo commands
   *   will be blocked only if if the input signal of condition is equal to, above (for positive
   *   thresholds) or below (for negative thresholds) the conditionThreshold.
   * - conditionThreshold: The default is 1.
   * - unless: Equivalent to 'condition', but the commands are disabled if the condition is NOT true.
   */
  class DisableModifier : public Modifier::Registrar<DisableModifier> {
  protected:
    /**
     * If true, sends events to zero out the button on initialization.
     */
    bool disableOnStart;
    /**
     * A condition to test. The event will only be tested if condition_threshold <> 0.
     */
    GPInput condition;
    GPInput condition_remap;
    int condition_threshold;
    /**
     * If true, we invert the polarity of the condition (i.e., it must be false to disable the signal)
     */
    bool invertCondition;
    /**
     * Limit above or below which values will be disabled
     */
    int filter_threshold;
    /**
     * What type of filtering to apply to the signal
     */
    DisableFilter filter;
  public:
    static const std::string name;
    
    /**
     * \brief The public constructor
     * \param config A TOML modifier table object. If the constructor is properly dispatched, this object
     * will contain the key/value pair 'type=disable'.
     */
    DisableModifier(const toml::table& config);
    /**
     * \brief The routine called each time the mod is started up.
     */
    void begin();
    /**
     * \brief The routine called to check each event coming from the controller for possible action.
     * \param event A structure containing the id and value of the device event.
     */
    bool tweak(DeviceEvent* event);

  };
};
