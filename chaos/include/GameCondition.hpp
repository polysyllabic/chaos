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
#include <unordered_map>
#include <vector>
#include <string>
#include <toml++/toml.h>
#include "DeviceEvent.hpp"
#include "enumerations.hpp"

namespace Chaos {

  class GameCommand;
  class GameCommandTable;
  class ControllerInput;
  /**
   * \brief Class to encapsulate a test for a controller condition that can be used by a modifier
   * to make a decision.
   * 
   * Conditions are used to test the state of the game for particular controller states. For
   * example, aiming a weapon is a common condition that occurs with other events and changes the
   * meaning of an incomming signal, e.g., you shoot the weapon when aiming but reload it when not
   * aiming.
   * 
   * Conditions are defined in the TOML file, and the following keys are allowed:
   *
   * - name: The name by which the condition is identified in the TOML file (_Required_)
   * - persistent: Boolean value to indicate whether the condition's state persists after being
   * triggered. (_Optional, default = false_)
   * - trigger_on: An array of commands that will be tested to determine if a condition state will
   * be set to true. Once set, the state persists as true until either the clear_on conditions are
   * met or the state is manually reset. If more than one command is in the trigger_on list, all
   * incomming events must be received before the event triggers, and a partially triggered
   * condition will be canceled if new incomming events undo the earlier partial trigger. If
   * _trigger_on_ is not defined, _while_ must be, and vice versa. (_Optional_)
   * - clear_on: An array of commands that will be tested to determine when a persistent state
   * should be set to false. Not used for transient conditions. (_Optional_)
   * - while: A list of commands that must be true according to the real-time state of the
   * controller, potentially in addition to the conditions defined in _trigger_on_. If _trigger_on_
   * is not defined, _while_ or _while_not_ must be, and vice versa. If both _while/while_not_ and
   * _trigger_on_ are defined, the while conditions are tested at the point where all conditions in
   * _trigger_on_ are met. (_Optional_)
   * - while_not: A list of commands that must be *false* according to the real-time status of the
   * controller. This can be used together with or independently of _while_. (Optional)
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
   * - test_type: For a #thresholdType other than DISTANCE, what sort of logical relationship is
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
   *    while = [ "aiming" ]
   * 
   *    [[condition]]
   *    name = "shot taken"
   *    trigger_on = [ "shoot" ]
   *    while = [ "aiming" ]
   *    
   *    [[condition]]
   *    name = "movement"
   *    trigger_on = [ "move forward/back", "move sideways" ]  
   *    threshold = 0.2
   *    threshold_type = "distance"
   *
   * You can test a condition by calling the member function #inCondition().
   */
  class GameCondition {
  protected:
    std::string name;

    /**
     * \brief Commands to test if their real-time state is true
     */
    std::vector<std::shared_ptr<GameCommand>> while_conditions;

    /**
     * \brief Commands to test if their real-time state is false
     */
    std::vector<std::shared_ptr<GameCommand>> while_not_conditions;

    /**
     * \brief Commands to check whether the condition is on.
     *
     * A vector of game commands that should be checked to determine if the game has reached 
     * a state that we care about.
     */
    std::unordered_map<std::shared_ptr<GameCommand>, bool> trigger_on;

    /**
     * \brief Commands to check that turn the condition off.
     *
     * A vector of game commands that should be checked to determine if the game has left some
     * persistent state.
     */
    std::unordered_map<std::shared_ptr<GameCommand>, bool> clear_on;

    /**
     * \brief The current state of the condition.
     */
    bool persistent_state;

    /**
     * \brief The rule for how to test the threshold value that triggers a condition.
     */
    ThresholdType threshold_type;

    /**
     * \brief The proportion that the threshold rule uses to test.
     * 
     * Although it would be faster to pre-convert this proportion to an integer, we need to account
     * for the possibility that a signal is remapped from one signal type to another with a
     * different maximum value.
     */
    double threshold;

    /**
     * \brief Translates the proportional threshold into an integer based on the type of signal.
     * This check is done _after_ any signal remapping. Call the overloaded version with a
     * GameCommand pointer to get the thresholled with remapping.
     * 
     * \return short 
     */
    short getSignalThreshold(std::shared_ptr<ControllerInput> input);

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

    bool thresholdComparison(short value, short threshold);
    /**
     * \brief Test the real-time state of the controller
     * 
     * \param command_list 
     * \param unless 
     * \return true 
     * \return false 
     */
    bool testConditions(std::vector<std::shared_ptr<GameCommand>> command_list, bool unless);

public:
  /**
   * \brief Construct a new GameCondition object
   * 
   * \param config Parsed TOML table containing the list of parameters defining the condition.
   * \param commands Container object for previously initialized game commands
   */
  GameCondition(toml::table& config, GameCommandTable& commands);

  /**
   * \brief Resets the condition to appropriate initial conditions
   */
  void reset();

  /**
   * \brief Tests if the condition's parameters have all been met.
   * 
   */
  bool inCondition();

  /**
   * \brief Updates the test state with the incomming event and checks if it fulfils the conditions
   * of the test.
   */
  bool conditionTriggered(DeviceEvent& event);

  /**
   * \brief Sets state if an incoming signal matches the trigger/clear states for a persistent event.
   * 
   * Conditions can be defined to be persistent, meaning that we track their state after the input
   * that caused them is no longer coming from the controller. For example, selecting a weapon is
   * an event that we want to remember, since the singificance of other actions can change depending
   * on what is selected. Persistent conditions are defined 
   */
  void updateState(DeviceEvent& event);

  std::string& getName() { return name; }
  };

};