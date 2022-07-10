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
   * Example TOML defintition:
   *
   *     [[command]]
   *     name = "aiming"
   *     binding = "L2"
   *
   * Note that command names must be unique, but you can define multiple names to point to the same
   * signal. This allows for aliases, in case you want to use different command names in different
   * contexts.
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

