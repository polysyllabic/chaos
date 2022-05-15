/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
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
#include <memory>
#include <toml++/toml.h>

#include "DeviceEvent.hpp"

namespace Chaos {
  class Game;
  class GameCondition;
  class ControllerInput;
  class Game;

  /**
   * \brief Class to hold a mapping between a command defined for a game and the controller's
   * button/axis presses.
   *
   * During initialization, we parse a TOML file to produce a map of these command bindings,
   * which in turn is used to initialize the individual modifiers. Each command also maintains a
   * record of the current remapping state, so the inputs for particular commands can be swapped
   * on a per-command basis.
   *
   * Accepted TOML entries:
   * - name: A string to refer to this command elsewhere in the TOML file. (_Required_)
   * - binding: The name of a controller input signal. The legal names are defined by
   * ControllerCommand. (_Required_)
   * - condition: The name of a defined condition whose state must be true for the command to
   * apply. Mutually exclusive with "unless". (_Optional_)
   * - unless: The inverse of "condition": the name of a defined condition whose state must be
   * _false_ for the command to apply. Mutually exclusive with "condition".
   *
   * Example TOML defintitions:
   *
   *     [[command]]
   *     name = "aiming"
   *     binding = "L2"
   *
   *     [[command]]
   *     name = "shoot/throw"
   *     binding = "R2"
   *     condition = "aiming"
   *
   */
  class GameCommand : std::enable_shared_from_this<GameCommand> {
  private:
    /**
     * \brief The name of the command as used in the TOML file
     */
    std::string name;

    /**
     * \brief The button/axis the game expects for this command.
     */
    std::shared_ptr<ControllerInput> binding;

    /**
     * \brief Additional condition that must be true for the command to apply.
     *
     * This is a single condition (unlike the lists of conditions maintained in the modifiers)
     * and does not support recursion of conditions.
     */
    std::shared_ptr<GameCondition> condition;

    /**
     * Reverse the polarity of the condition test. If true, the command is applied *unless* the
     * controller is in the specified condition.
     */
    bool invert_condition;
    
  public:
    /**
     * \brief The public constructor to define the controller input that corresponds
     * to a single game command.
     *
     * \param cmd The name of the command
     * \param bind The actual signal the command is officially bound to by the game
     * \param condition A game condition that must be true when the signal comes in for the command to apply
     * \param invert If true, inverts the polarity of the game condition (= unless condition)
     */
    GameCommand(const std::string& cmd, std::shared_ptr<ControllerInput> bind,
                std::shared_ptr<GameCondition> condition, bool invert);

    /**
     * \brief Returns #this as a shared pointer
     * 
     * \return std::shared_ptr<GameCommand> 
     *
     * Since the pointer for this object is managed by std::shared_ptr, using #this
     * directly is dangerous. Use this function instead.
     */
    std::shared_ptr<GameCommand> getptr() { return shared_from_this(); }

    /**
     * \brief Accessor for the CongrollerInput object bound to this command.
     * \return std::shared_ptr to the ControllerInput object.
     */
    std::shared_ptr<ControllerInput> getInput() { return binding; }
    
    /**
     * \brief Get the pointer to the ControllerInput object that is the remapped signal.
     * 
     * \return std::shared_ptr<ControllerInput> 
     */
    std::shared_ptr<ControllerInput> getRemappedSignal();

    /**
     * \brief Accessor for a condition that must also be true for this command to apply.
     * \return The condition 
     */
    std::shared_ptr<GameCondition> getCondition() { return condition; }

    /**
     * If true, test for the condition being false rather than true.
     */
    bool conditionInverted() { return invert_condition; }

    /**
     * \brief Get the current state of the controller for this command
     * 
     * \return short Current state
     *
     * When checking the current state, we look at the remapped signal. That is, we case about what
     * the user is actually doing, not what signal is going to the console. Remapping between axes
     * and buttons, though creates a problem, since we need to check to two signals simultaneously.
     * for the purpose of checking thresholds, etc.
     */
    short getState();

    /**
     * \brief Get the name of the command as defined in the TOML file
     * 
     * \return std::string& 
     *
     * This command is provided largely for debugging
     */
    const std::string& getName() { return name; }
  };
};

