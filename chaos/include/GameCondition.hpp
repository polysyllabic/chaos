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
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <toml++/toml.h>

#include "enumerations.hpp"

namespace Chaos {

  class GameCommand;

  /**
   * \brief Class to encapsulate a test for a controller condition that can be used by a modifier
   * to make a decision.
   * 
   * Conditions can be transient, reflecting the real-time state of the controller, or persistent.
   * Persistent conditions remember previous controller actions, such as selecting a weapon, that
   * may not be reflected in the current state of the controller.
   * 
   * Conditions are defined in the TOML file, and the following keys are allowed:
   *
   * - name: The name by which the condition is identified in the TOML file (_Required_)
   * - persistent: Boolean value to indicate whether the condition's state persists after being
   * triggered. (_Optional, default = false_)
   * - trueOn: An array of commands that will be tested to determine if the condition is true. For
   * persistent conditions, detecting this condition sets the persistent state flag to true.
   * (_Required_)
   * - trueOff: An array of commands that will be tested to determine when a persistent condition
   * should be set to false. Not used for transient conditions. (_Optional_)
   * - threshold: The threshold that a signal value must reach to trigger the condition, expressed
   * as a proportion of the maximum value for the particular signal type.
   * - thresholdType: The test applied to the threshold. The following keys correspond to the types
   # defined in ThresholdType.
   *     - "greater" or ">": ThresholdType::GREATER
   *     - "greater_equal" or ">=": ThresholdType::GREATER_EQUAL
   *     - "less" or "<": ThresholdType::LESS
   *     - "less_equal" or "<=" ThresholdType::LESS_EQUAL
   *     - "magnitude": ThresholdType::MAGNITUDE (_Default_)
   *     - "distance": ThresholdType::DISTANCE
   *     .
   * - testType: For a #thresholdType other than DISTANCE, what sort of logical relationship is
   * required for the individual conditions. The possible values are:
   *     - "all": All commands listed in #commands must be past the threshold. (_Default_)
   *     - "any": One or more of the commands in #commands must be past the threshold.
   *     - "none": None of the commands in #commands must be past the threshold.
   *
   * The threshold must be a number in the range $0 \le x \le 1$ that reflects the proportion of
   * the maximum value. It should therefore always be positive even when the sign of the incoming
   * signal is negative. So if you want, for example, to trigger the condition when the left Dpad
   * is pressed (-1 on the DX axis) but not when the right Dpad is pressed, you would set the
   * threshold to 1 and the thresholdType to BELOW_EQUAL.
   *
   * If there is more than one command defined in #conditions, they must have the same maximum axis
   * value. The same threshold check is applied to every command in the list, and if you attempt
   * to mix different types, you will likely get incorrect behavior.
   *
   * \todo Allow different thresholds for each command in the condition array.
   * \todo Support turning off a persistent condition after a fixed time.
   *
   * The default threshold and #thresholdType settings have the effect of returning a true
   * condition on any non-zero signal from the specified game commands.
   *
   * Example TOML configuation settings:
   *
   *    [[condition]]
   *    name = "aiming"
   *    trueOn = [ "aiming" ]
   *
   *    [[condition]]
   *    name = "movement"
   *    trueOn = [ "move forward/back", "move sideways" ]	
   *    threshold = 0.2
   *    thresholdType = "DISTANCE"
   *
   *    [[condition]]
   *    name = "gun selected"
   *    persistent = true
   *    trueOn = [ "select weapon" ]
   *    falseOn = [ "select consumable" ]
   *
   * You can test a condition by calling the member function #inCondition().
   */
  class GameCondition {
  protected:
    std::string name;
    /**
     * \brief Commands to check whether the condition is on.
     *
     * A vector of game commands that should be checked to determine if the game is in some
     * condition that the mod cares about.
     */
    std::vector<std::shared_ptr<GameCommand>> true_on;

    /**
     * \brief Commands to check that turn the condition off.
     *
     * A vector of game commands that should be checked to determine if the game has left some
     * persistent state. Ignored if #persistent is false.
     */
    std::vector<std::shared_ptr<GameCommand>> true_off;

    /**
     * \brief Does this condition track a persistent game state?
     */
    bool persistent;
    
    /**
     * \brief The current state of a persistent-state condition.
     */
    bool state;

    /**
     * \brief The rule for how to test the threshold value that triggers a condition.
     */
    ThresholdType threshold_type;

    /**
     * \brief What type of test to apply to the commands are in the #trueOn and #trueOff vectors.
     */
    ConditionCheck condition_type;
    
    /**
     * \brief The proportion that the threshold rule uses to test.
     * 
     * Although it would be faster to pre-convert this proportion to an integer, we need to account
     * for the possibility that a signal is remapped from one signal type to another with a
     * different maximum value.
     */
    double threshold;

    /**
     * \brief Translates the proportional threshold into an integer based on the type of signal that
     * the game command is currently mapped to and the threshold type.
     * 
     * \return short 
     */
    short getSignalThreshold(std::shared_ptr<GameCommand> signal);

    /**
     * \brief Test if the signal exceeds the threshold for this type
     * 
     * \param signal 
     * \return true 
     * \return false 
     */
    bool pastThreshold(std::shared_ptr<GameCommand> signal);

    bool testConditions(std::vector<std::shared_ptr<GameCommand>> command_list);

    void addToVector(const toml::table& config, const std::string& key,
                     std::vector<std::shared_ptr<GameCommand>>& vec,
                     GameCommandTable& commands);

public:
  /**
   * \brief Construct a new GameCondition object
   * 
   * \param config Parsed TOML table containing the list of parameters defining the condition.
   * \param commands Container object for previously initialized game commands
   */
  GameCondition(toml::table& config, GameCommandTable& commands);

  /**
   * \brief Tests if the current state of the controller meets the defined condition(s)
   */
  bool inCondition();

  /**
   * \brief Sets state if an incoming signal matches the start/stop states for a persistent event.
   * 
   * Conditions can be defined to be persistent, meaning that we track their state after the input
   * that caused them is no longer coming from the controller. For example, selecting a weapon is
   * an event that we want to remember, since the singificance of other actions can change depending
   * on what is selected. Persistent conditions are defined 
   */
  void updateState();

  std::string& getName() { return name; }
  };

};