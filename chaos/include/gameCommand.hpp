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

#include "gameCommand.hpp"
#include "deviceTypes.hpp"
#include "gamepadInput.hpp"

namespace Chaos {
  
  /**
   * \brief Class to hold a mapping between a command defined for a game and the controller's
   * button/axis presses.
   *
   * During initialization, we parse a TOML file to produce a map of these command bindings,
   * which in turn is used to initialize the individual modifiers. Each command also maintains a
   * record of the current remapping state, so the inputs for particular commands can be swapped
   * on a per-command, rather than per input, basis.
   *
   * Accepted TOML entries:
   * - name (required): a string to refer to this command elsewhere in the TOML file.
   * - binding (required): the name of a gamepad input. The legal names are defined in
   *   ControllerCommand::buttonNames.
   * - condition (optional): the name of another command whose state must also be true for the
   *   command to apply. Mutually exclusive with "unless".
   * - unless (optional): the inverse of "condition": the name of another command whose state
   *   must be *false* for the command to apply. Mutually exclusive with "condition".
   * - threshold (optional): set a specific integer value that the input must exceed for the
   *   condition to be true. The default threshold is 1, which is the state we expect for ordinary
   *   buttons when pressed. If a condition is defined without an explicit threshold, it will be
   *   set to 1 automatically, so this key is only necessary if you need a custom value for an
   *   axis. A threshold is 0 signifies that there is no condition to test. If you want to test for
   *   a specific button *not* being pressed, use "unless" instead of "condition".
   *
   * Note that commands referenced in condition or unless lines must be defined earlier in the TOML
   * file.
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
  class GameCommand {
  private:
    /**
     * The real button/axis we send to the console for this command.
     */
    GPInput binding;
    /**
     * Additional controller state that must be true for the command to apply.
     */
    GPInput condition;
    /**
     * Input value threshold that must be exceeded for the condition to be true.
     */
    int threshold;
    /**
     * Reverse the polarity of the condition test. If true, the command is applied *unless* the
     * gamepad is in the specified condition.
     */
    bool invertCondition;
    
    /**
     * \brief Map of defined commands identified by a string name.
     */
    static std::unordered_map<std::string, std::shared_ptr<GameCommand>> commands;
    
  public:
    /**
     * \brief The public constructor to define the controller input that corresponds
     * to a single game command.
     *
     * The constructor expects a single table from the TOML file which must contain
     * a binding of this command to a particular button/axis on the controller.
     * The valid names for gamepad inputs are defined in ControllerCommand::buttonNames.
     *
     * The command definition may optionally contain a condition, which must be the name
     * of another command definition that is also defined in the TOML file. Any conditions
     * referenced in a command definition must be defined *before* they are referenced.
     *
     */
    GameCommand(toml::table& config);
    /**
     * Initialize the global map to hold the command definitions. This is called once
     * during the start-up phase.
     */
    static void initialize(toml::table& config);

    /**
     * \brief Accessor to GameCommand pointer by command name.
     * \param name Name by which the game command is identified in the TOML file.
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
    static GPInput getInput(const std::string& name);

    /**
     * \brief Accessor for the gamepad input bound to this command.
     */
    inline std::shared_ptr<GamepadInput> getInput() {
      return GamepadInput::get(binding);
    }
    
    /**
     * \brief Accessor for the signal bound to this command.
     * \return The input command the console expects.
     */
    inline GPInput getBinding() { return binding; }

    /**
     * \brief Accessor for a condition that must also be true for this command to apply.
     * \return The condition 
     */
    inline GPInput getCondition() { return condition; }
    /**
     * \brief Accessor for a the threshold value that the condition must meet or exceed for the
     * condition to be true.
     * \return The threshold
     *
     * The threshold should be a positive value. The absolute value of the signal will be tested
     * against this.
     */    
    inline int getThreshold() { return threshold; }
    /**
     * If true, test for the condition being false rather than true.
     */
    inline bool conditionInverted() { return invertCondition; }
  };
};

