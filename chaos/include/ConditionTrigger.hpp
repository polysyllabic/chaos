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
#include <string>
#include <vector>
#include <memory>
#include "DeviceEvent.hpp"
#include "GameCondition.hpp"

namespace Chaos {

  /**
   * \brief Defines a trigger based on one or more conditions.
   * 
   * A trigger is a game state that occurs as the result of an incomming event (command)
   * that optionally occurs while one or more game conditions are true. A trigger can be
   * transient or persistent.
   * 
   * Triggers are defined in the TOML file, and the following keys are allowed:
   *
   * - _name_: The name by which the trigger is identified in the TOML file (_Required_)
   * - _trigger_on_: A game condition that is tested against an incomming signal
   * - _clear_on_: A game condition that is tested to determine when a persistent state
   * should be set to false. (_Optional_)
   * - _while_: A list of game conditions that must be *true* at the time the trigger_on command is
   * received in order to set the trigger state. At least one of while or unless must be
   * defined. (_Optional_)
   * - _unless_: A list of game conditions that must be *false* at the time the trigger_on
   * command is received. At least one of while or unless must be defined. (_Optional_)
   *
   * \todo Allow multiple triggers/clear
   * \todo Support turning off a persistent condition after a fixed time.
   * \todo While/until conditions for clear_on
   * 
   * Example TOML configuation settings:
   *
   *    [[trigger]]
   *    name = "shot taken"
   *    trigger_on = [ "shoot" ]
   *    while = [ "aiming" ]
   *    
   * If #clear_on is set, the state persists as true until either the clear_on condition is
   * met or the state is manually reset.
   * 
   */
  class ConditionTrigger {

  private:

    std::string name;

    std::vector<std::shared_ptr<GameCondition>> while_conditions;
    std::vector<std::shared_ptr<GameCondition>> unless_conditions;

    std::shared_ptr<GameCondition> trigger_on = nullptr;
    std::shared_ptr<GameCondition> clear_on = nullptr;

    bool trigger_state = false;

    bool testConditions(std::vector<std::shared_ptr<GameCommand>> command_list, bool unless);

  public:
    /**
     * \brief Construct a new ConditionTrigger object
     */
    ConditionTrigger(const std::string& name);

    /**
     * \brief Resets the condition to appropriate initial conditions
     */
    void reset();

    /**
     * \brief Reports if the trigger is currently set
     */
    bool isTriggered() { return trigger_state; }

    /**
     * \brief Sets state if an incoming signal matches the trigger/clear states for a persistent event.
     * 
     * Conditions can be defined to be persistent, meaning that we track their state after the input
     * that caused them is no longer coming from the controller. For example, selecting a weapon is
     * an event that we want to remember, since the singificance of other actions can change depending
     * on what is selected. Persistent conditions are defined 
     */
    void updateState(DeviceEvent& event);

    /**
     * \brief Get the name of the trigger
     * 
     * \return std::string& 
     */
    std::string& getName() { return name; }

    /**
     * \brief Add a condition to the while condition list
     * 
     * \param condition The GameCondition to add
     */
    void addWhileCondition(std::shared_ptr<GameCondition> condition);

    /**
     * \brief Add a condition to the unless condition list
     * 
     * \param condition The GameCondition to add
     */
    void addUnlessCondition(std::shared_ptr<GameCondition> condition);

    /**
     * \brief Set the trigger on condition
     * 
     * \param condition The GameCondition that will serve as a trigger
     */
    void setTriggerOn(std::shared_ptr<GameCondition> condition);

    /**
     * \brief Set the clear on condition
     * 
     * \param condition The GameCondition that will clear the trigger
     */
    void setClearOn(std::shared_ptr<GameCondition> condition);
  };

};