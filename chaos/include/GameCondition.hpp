/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
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
   * Game conditions are used to test the state of events coming from the controller. There are
   * two types of conditions: transient and persistent. A transient condition polls the current
   * state of the controller. In other words, it will be true or false depending on what the user
   * is doing at that moment, and as soon as the conditions stop, the condition will read false.
   * A persistent condition is set to true when a particular condition arrives and remains true
   * until a different condition arrives that clears the condition.
   * 
   * Transient conditions are defined with the `while` key and the condition will be true as long
   * as the current state of the controller exceeds the defined threshold value and false
   * otherwise. More than one command can appear in the while parameter, but how multiple
   * commands are processed depends on the threshold type.
   * 
   * Persistent events also use the `while` key to define the state that turns the condition true,
   * but once the condition is true, it remains so until the commands listed in `clear_on` are
   * true.
   * 
   * Both transient and persistent must define the `while` key. The `clear_on` key is only valid
   * for persistent conditions. If it is missing, the condition will be treated as transient.
   * 
   * The syntax for defining conditions in the TOML file is described in chaosCoonfigFiles.md
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

    std::vector<std::shared_ptr<ControllerInput>> clear_on;

    /**
     * The current state of a persistent trigger
     */
    bool persistent_state = false;

    short threshold = 1;

    ThresholdType threshold_type = ThresholdType::ABOVE;

    short clear_threshold = 1;

    ThresholdType clear_threshold_type = ThresholdType::ABOVE;

    bool testCondition(std::vector<std::shared_ptr<ControllerInput>> conditions, short thresh, ThresholdType type);

    /**
     * \brief Test if the value passes the threshold for this threshold type
     * 
     * \param value incoming value
     * \return W
     * 
     * The ThresholdType must be one of the ordinary comparisons to one value. It's a programming
     * error to call this function if the threshold type is DISTANCE.
     */
    bool thresholdComparison(short value, short thresh, ThresholdType type);

    bool distanceComparison(short x, short y, short thresh, ThresholdType type);

    short calculateThreshold(double proportion, std::vector<std::shared_ptr<ControllerInput>> conditions);

  public:
    /**
     * \brief Construct a new GameCondition object
     * 
     * \param name Name of the came condition
     */
    GameCondition(const std::string& name);

    /**
     * \brief Get the threshold value required for the while condition to be true
     * 
     * \return short 
     */
    short getThreshold() { return threshold; }

    /**
     * \brief Get the threshold value for the clear_on condition to be true
     * 
     * \return short 
     */
    short getClearThreshold() { return clear_threshold; }

    /**
     * \brief Translates the proportional threshold into an integer based on the type of signal that
     * the game command is currently mapped to and the threshold type.
     */
    void setThreshold(double proportion);

    /**
     * \brief Translates the proportional threshold into an integer based on the type of signal that
     * the game command is currently mapped to and the clear_threshold type.
     */
    void setClearThreshold(double proportion);

    /**
     * \brief Sets the rule for how to test the threshold value that triggers a condition.
     */
    void setThresholdType(ThresholdType new_type) { threshold_type = new_type; }

    /**
     * \brief Sets the rule for how to test the threshold value that triggers a condition.
     */
    void setClearThresholdType(ThresholdType new_type) { clear_threshold_type = new_type; }

    /**
     * \brief Add a command to the while condition list
     * 
     * \param command The game command to add
     * 
     * The GameCommand is translated to a ControllerInput object.
     */
    void addWhile(std::shared_ptr<GameCommand> command);

    /**
     * \brief Add a command to the clear_on condition list
     * 
     * \param command The game command to add
     * 
     * The GameCommand is translated to a ControllerInput object.
     */
    void addClearOn(std::shared_ptr<GameCommand> command);

    /**
     * \brief Tests if the condition's parameters have all been met.
     */
    bool inCondition();
    
    /**
     * \brief Check if this condition is transient or persistent
     * 
     * \return true This condition tests the live state of the controller (transient)
     * \return false This condition maintains a persistent state
     */
    bool isTransient() { return !clear_on.empty(); }

    int getNumWhile() { return while_conditions.size(); }

    /**
     * \brief Get the name of the condition as used in the TOML file
     * 
     * \return std::string& 
     */
    std::string& getName() { return name; }
  };

};