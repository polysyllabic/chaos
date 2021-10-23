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
#include "controllerCommand.hpp"
#include "deviceTypes.hpp"

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
   * TODO: Allow commands to be conditional on a persistant game state.
   */
  class GameCommand {
  private:
    /**
     * The real button/axis we send to the console for this command.
     */
    GPInput binding;
    /**
     * The remappped command that we accept from the player.
     */
    GPInput binding_remap;
    /**
     * Additional controller state that must be true for the command to apply.
     */
    GPInput condition;
    /**
     * The remapped condition that we look for from the player.
     */
    GPInput condition_remap;
    /**
     * Input value threshold that must be exceeded for the condition to be true.
     */
    int threshold;
    /**
     * Reverse the polarity of the condition test. If true, the command is applied *unless* the gamepad is
     * in the specified condition.
     */
    bool invertCondition;
    /** 
     * Vector to hold the button type and id value(s)
     */
    static std::vector<ControllerCommand> buttonInfo;
    
  public:
    /**
     * Global map for defined commands.
     */
    static std::unordered_map<std::string, std::shared_ptr<GameCommand>> bindingMap;
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
     * Return the actual input command the console expects.
     */
    inline GPInput getReal() { return binding; }
    /**
     * Return the remapped command that the player needs to produce.
     */
    inline GPInput getRemap() { return binding_remap; }
    /**
     * Set the remap to a new value
     */
    inline void setRemap(GPInput new_id) { binding_remap = new_id; }
    /**
     * Restore the command to the default control mapping
     */
    inline void resetRemap() { binding_remap = binding; }

  };
};

