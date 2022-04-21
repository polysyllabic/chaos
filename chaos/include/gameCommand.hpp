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
#include <string>
#include <memory>
#include <unordered_map>
#include <toml++/toml.h>

#include "deviceEvent.hpp"
#include "gamepadInput.hpp"

namespace Chaos {
  
  class GameCondition;
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
   * - binding: The name of a gamepad input. The legal names are defined in
   * ControllerCommand::buttonNames. (_Required_)
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
     * The binding of the actual button/axis the console expects for this command to the
     * potentially remapped signal we expect from the controller.
     */
    std::shared_ptr<GamepadInput> binding;

    /**
     * \brief Additional condition that must be true for the command to apply.
     *
     * This is a single condition (unlike the lists of conditions maintained in the modifiers)
     * and does not support recursion of conditions.
     */
    std::shared_ptr<GameCondition> condition;

    /**
     * Reverse the polarity of the condition test. If true, the command is applied *unless* the
     * gamepad is in the specified condition.
     */
    bool invert_condition;
    
    /**
     * \brief Map of defined commands identified by a string name.
     */
    static std::unordered_map<std::string, std::shared_ptr<GameCommand>> commands;
    
  public:
    /**
     * \brief The public constructor to define the controller input that corresponds
     * to a single game command.
     *
     * \param config A single table from the TOML file that defines this command.
     */
    GameCommand(toml::table& config);

    /**
     * \brief Initialize the global map to hold the command definitions without conditions.
     *
     * \param config The object containing the complete parsed TOML file
     *
     * Because commands can reference conditions and conditions are based on commands, we need some
     * extra steps to avoid initialization deadlocks and infinite recursion. We therefore use a
     * two-stage initialization process. First, this routine initializes all "direct" commands
     * (those without conditions). Next, we initialize all conditions. Finally, we initialize
     * those commands based on a condition with #buildCommandMapCondition().
     *
     * This routine is called once during the start-up phase.
     */
    static void buildCommandMapDirect(toml::table& config);

    /**
     * \brief Initialize the global map to hold the command definitions with conditions.
     *
     * \param config The object containing the complete parsed TOML file
     *
     * Because commands can reference conditions and conditions are based on commands, we need some
     * extra steps to avoid initialization deadlocks and infinite recursion. We therefore use a
     * two-stage initialization process. First, #buildCommandMapDirect() initializes all commands
     * without conditions. Next, we initialize all conditions. Finally, we initialize those
     * commands based on a condition in this routine.
     *
     * This routine is called once during the start-up phase.
     */
    static void buildCommandMapCondition(toml::table& config);

    /**
     * \brief Accessor to GameCommand pointer by command name.
     *
     * \param name Name by which the game command is identified in the TOML file.
     *
     * \return The GameCommand pointer for this command, or NULL if not found.
     */
    static std::shared_ptr<GameCommand> get(const std::string& name);

    /**
     * \brief Get the command as an gamepad input enumeration.
     * \param name The name of the command as defined in the TOML file
     * \return The GPInput signal associated with this command, or GPInput::NONE if the command
     * does not exist.
     *
     * Note that because of conditions, more than one command can return the same GPInput. This
     * should only be used when you really want the signal and don't care about testing for the
     * context of a particular game command.
     */
    //static GPInput getInput(const std::string& name);

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
     * \brief Accessor for the gamepad input object bound to this command.
     * \return std::shared_ptr to the GamepadInput object.
     */
    std::shared_ptr<GamepadInput> getInput() { 
      return binding; 
      }
    
    /**
     * \brief Get the pointer to the gamepad input object that is the remapped signal.
     * 
     * \return std::shared_ptr<GamepadInput> 
     */
    std::shared_ptr<GamepadInput> getRemapped();

    /**
     * \brief Accessor for a condition that must also be true for this command to apply.
     * \return The condition 
     */
    std::shared_ptr<GameCondition> getCondition() { 
      return condition; 
      }

    /**
     * If true, test for the condition being false rather than true.
     */
    bool conditionInverted() { 
      return invert_condition; 
      }

    /**
     * \brief Get the current state of the controller for this command
     * 
     * \return short 
     *
     * When checking the current state, we look at the remapped signal. That is, we case about what
     * the user is actually doing, not what signal is going to the console.
     */
    short int getState() { return binding->getState(); }

    /**
     * \brief Get the name of the command as defined in the TOML file
     * 
     * \return std::string& 
     *
     * This command is provided largely for debugging
     */
    const std::string& getName();
  };
};

