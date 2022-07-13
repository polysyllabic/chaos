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
   * \brief Defines a single test of a game condition
   * 
   * A GameCondition is used to test particular controller states, for example, aiming a weapon.
   * Modifiers use conditions to define various behavior. For example, a modifier may only work
   * if a particular condition is true or false. Conditions are also used as the building blocks
   * for a ConditionTrigger.
   * 
   * Game conditions can be used in two modes: polling and queued. Polling mode looks at the
   * current state of the controller. Queued takes a device event passed through the tweak()
   * meachanism. Polled conditions can have multiple commands in the while vector. Queued
   * conditions are limited to checking one command at a time.
   * 
   * Conditions are defined in the TOML file, and the following keys are allowed:
   *
   * - name: The name by which the condition is identified in the TOML file (_Required_)
   * - while: A list of commands that must be true according to the real-time state of the
   * controller (_Required_)
   * - threshold: For axes, the threshold that a signal value must reach to trigger the condition,
   * expressed as a proportion of the maximum value. (_Optional. Default = 1_)
   * - thresholdType: The test applied to the threshold. The following keys correspond to the types
   # defined in ThresholdType.
   *     - "greater" or ">": ThresholdType::GREATER
   *     - "greater_equal" or ">=": ThresholdType::GREATER_EQUAL
   *     - "less" or "<": ThresholdType::LESS
   *     - "less_equal" or "<=" ThresholdType::LESS_EQUAL
   *     - "magnitude": ThresholdType::MAGNITUDE (_Default_)
   *     - "distance": ThresholdType::DISTANCE
   *
   * The threshold must be a number in the range $0 \le x \le 1$ that reflects the proportion of
   * the maximum value. It should therefore always be positive even when the sign of the incoming
   * signal is negative. The threshold is ignored for buttons and will always be 1.
   *
   * If more than one command is defined in #while, the same threshold applies to all. This means
   * the test will only work if each command has the same maximum value. If you have different
   * categories of signal or different thresholds, you should create separate conditions and
   * chain them together in a ConditionTrigger.
   *
   * The 'distance' threshold type checks the Pythagorean distance 
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
   *    name = "movement"
   *    while = [ "move forward/back", "move sideways" ]  
   *    threshold = 0.2
   *    threshold_type = "distance"
   *
   * You can test a condition in two ways. Calling the member function inCondition() polls
   * the real-time state of the controller for the condition. The function inCondition(event) 
   * tests if the passed event matches the condition without checking the live state of the
   * controller. Note that this latter function only makes sense when used on simple conditions.
   */
  class GameCondition {
  private:
    std::string name;
    std::vector<std::shared_ptr<ControllerInput>> while_conditions;

    ThresholdType threshold_type;

    /**
     * \brief The integer value that the threshold rule uses to test.
     * 
     */
    short threshold;

    /**
     * \brief Translates the proportional threshold into an integer based on the type of signal.
     * 
     * \return short 
     */
    short getSignalThreshold(std::shared_ptr<ControllerInput> input);

    /**
     * \brief Test if the value passes the threshold for this threshold type
     * 
     * \param value incoming value
     * \return W
     * 
     * The ThresholdType must be one of the ordinary comparisons to one value. It's a programming
     * error to call this function if the threshold type is DISTANCE.
     */
    bool thresholdComparison(short value);

  public:
    /**
     * \brief Construct a new GameCondition object
     * 
     * \param name Name of the came condition
     */
    GameCondition(const std::string& name);

    short getThreshold() { return threshold; }

    /**
     * \brief Translates the proportional threshold into an integer based on the type of signal that
     * the game command is currently mapped to and the threshold type.
     */
    void setThreshold(double proportion);

    /**
     * \brief Sets the rule for how to test the threshold value that triggers a condition.
     */
    void setThresholdType(ThresholdType new_type) { threshold_type = new_type; }

    /**
     * \brief Add a command to the game condition list
     * 
     * \param command The game command to add
     * 
     * The GameCommand is translated to a ControllerInput object.
     */
    void addCondition(std::shared_ptr<GameCommand> command);

    /**
     * \brief Tests if the condition's parameters have all been met.
     * 
     */
    bool inCondition();

    /**
     * \brief Check if the incomming event matches the first command in the while list.
     * A match returns true if the event matches the command in the while list regardless of
     * the event value.
     */
    bool matchEvent(DeviceEvent& event);

    /**
     * \brief Check if the incomming event value passes the threshold.
     * 
     * Note that this routine _does not_ check that the event id matches. Use matchEvent for that.
     */
    bool pastThreshold(DeviceEvent& event);

    /**
     * \brief Get the name of the condition as used in the TOML file
     * 
     * \return std::string& 
     */
    std::string& getName() { return name; }
  };

};