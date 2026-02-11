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

namespace Chaos {
  class ControllerInput;

  /**
   * \brief Class to hold a mapping between a command defined for a game and the controller's
   * button/axis presses.
   *
   * This gives a game-specific semantic interpretation to the signals that the game expects to
   * receive. For example, you can associate "interact" with TRIANGLE. During initialization, we
   * parse a TOML file to produce a map of these command bindings, which in turn is used to
   * initialize the individual modifiers.
   * 
   * Note that these commands are currently always simple. They associate one button press or axis
   * event with one command. They do not currently support associating simultaneous presses of
   * controls with one command. Currently, those states are modeled by checking for a condition
   * when a particular signal comes in. For example, "shoot" in TLOU2 is equivalent to
   * "reload/toss" with the condition "aiming".
   *
   * A command is defined in a TOML file by defining a name and binding. The following keys
   * are required:
   * - name: The label used to refer to the action in the elsewhere in this configuration file.
   * Command names must be unique, but you can define multiple names to point to the same signal.
   * This allows for aliases, in case you want to use different command names in different
   * contexts.
   * - binding: The controller signal (button, axis, etc.) attached to this command. The
   * information about these signals are stored in ControllerInput objects, and the particular
   * labels ("L1", "X", etc.) are hard-coded.
   * 
   * Example TOML defintition:
   *
   *     [[command]]
   *     name = "aiming"
   *     binding = "L2"
   * 
   */
  class GameCommand : public std::enable_shared_from_this<GameCommand> {
  private:
    /**
     * \brief The name of the command as used in the TOML file
     */
    std::string name;

    /**
     * \brief The button/axis the game expects for this command.
     */
    std::shared_ptr<ControllerInput> binding;

  public:
    /**
     * \brief The public constructor to define the controller input that corresponds
     * to a single game command.
     *
     * \param cmd The name of the command
     * \param bind The actual signal the command is officially bound to by the game
     */
    GameCommand(const std::string& cmd, std::shared_ptr<ControllerInput> bind);

    /**
     * \brief Returns shared pointer
     * 
     * \return std::shared_ptr<GameCommand> 
     *
     * Since the pointer for this object is managed by std::shared_ptr, using 'this'
     * directly is dangerous. Use this function instead.
     */
    std::shared_ptr<GameCommand> getptr() { return shared_from_this(); }

    /**
     * \brief Accessor for the CongrollerInput object bound to this command.
     * \return std::shared_ptr to the ControllerInput object.
     */
    std::shared_ptr<ControllerInput> getInput() { return binding; }
    
    /**
     * \brief Get the current state of the controller for this command
     * \param hybrid_axis Return the axis component if this is a hybrid control
     * \return short Live controller state
     *
     * This examines the current state of the controller. If the signal is a HYBRID type and
     * hybrid_axis is false, the button value will be polled. If it is true, the axis value
     * will be polled. This value is ignored for other types.
     */
    short getState(bool hybrid_axis);

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

