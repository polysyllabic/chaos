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
   * Conditions are used to test check the state of events coming from the controller. There are
   * two types of conditions: transient and persistent.
   * 
   * A transient condition polls the current state of the controller and compares the current
   * state of a list of controller signals held in #while_conditions to a defined threshold value.
   * More than one command can appear in the while parameter, but the way multiple commands are
   * processed depends on the threshold type.
   * 
   * A persistent condition is set to true when a particular condition arrives and remains true
   * until a different condition arrives that clears the condition. Persistent events define one
   * command that sets the state and a different command that clears it. Only one command at a
   * time can be checked for setting and clearing a persistent state.
   * 
   * Transient and persistent are mutually exclusive states, so a condition must define either the
   * while parameter or the set_on and clear_on parameters, but not both.
   * 
   * The syntax for defining conditions in the TOML file is described in chaosCoonfigFiles.md
   *
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

    std::shared_ptr<ControllerInput> set_on;
    std::shared_ptr<ControllerInput> clear_on;
    bool persistent_state = false;

    short threshold;
    ThresholdType threshold_type;

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

    /**
     * \brief Get the threshold value required for the condition to be true
     * 
     * \return short 
     */
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
     * \brief Set the command to check for setting a persistent condition
     * 
     * \param command The game command to add
     * 
     * The GameCommand is translated to a ControllerInput object.
     */
    void setSetOn(std::shared_ptr<GameCommand> command);
    
    /**
     * \brief Set the command to check for clearing a persistent condition
     * 
     * \param command The game command to add
     * 
     * The GameCommand is translated to a ControllerInput object.
     */
    void setClearOn(std::shared_ptr<GameCommand> command);

    /**
     * \brief Tests if the condition's parameters have all been met.
     * 
     * If this is a transient condition, it checks the current state of all items in the while
     * list. If this is a persistent condition, it returns the current state.
     */
    bool inCondition();

    /**
     * \brief Sets a persistent condition if the incoming signal matches the set/clear states for
     * a persistent event.
     */
    void updateState(DeviceEvent& event);

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
     * \brief Check if this condition is transient or persistent
     * 
     * \return true This condition tests the live state of the controller (transient)
     * \return false This condition maintains a persistent state
     */
    bool isTransient() { return !while_conditions.empty(); }

    /**
     * \brief Get the name of the condition as used in the TOML file
     * 
     * \return std::string& 
     */
    std::string& getName() { return name; }
  };

};