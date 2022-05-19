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
#include <unordered_map>

#include "GameMenu.hpp"
#include "ControllerInputTable.hpp"
#include "GameCommandTable.hpp"
#include "GameConditionTable.hpp"
#include "SequenceTable.hpp"
#include "ModifierTable.hpp"
#include "EngineInterface.hpp"

namespace Chaos {

  // Some of these may not need to be forward references, but I want to force classes to include
  // the header files explicitly.
  class Modifier;
  class Controller;
  /**
   * \brief Main container for the information needed to run a game
   * 
   * The Game class is the facade for all the various classes containing information we need to
   * inject chaos into a game. It also contains the logic to translate the TOML configuration file
   * into the actual objects we use during operation.
   */
  class Game {
  public:
    Game(Controller& c);

    /**
     * \brief Load game configuration file
     * 
     * \param configfile Name of the configuration file
     * \return true If loaded with non-fatal errors
     * \return false If had a fatal error while loading
     * 
     * If we're reloading a new game, an early fatal error will leave the old game's data intact
     */
    bool loadConfigFile(const std::string& configfile, EngineInterface* engine);

    /**
     * \brief Get the name of the game defined in the TOML file
     * 
     * \return std::string
     *
     * The game name is defined with the "game" key in the TOML configuration file.
     */
    const std::string& getName() { return name; }

    int getNumActiveMods() { return active_modifiers; }

    double getTimePerModifier() { return time_per_modifier; }

    /**
     * \brief Get the number of errors encountered loading the game-configuration file
     * 
     * \return int Error count
     */
    int getErrors() { return parse_errors; }

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Modifier> getModifier(const std::string& name) { return modifiers.getModifier(name); }

    std::string getModList() { return modifiers.getModList(); }

    GameCommandTable& getGameCommandTable() { return game_commands; }
    GameConditionTable& getGameConditionTable() { return game_conditions; }
    ControllerInputTable& getSignalTable() { return signal_table; }
    std::shared_ptr<SequenceTable> getSequenceTable() { return sequences; }
    /**
     * \brief Alters the incomming event to the value expected by the console
     * \param[in,out] event The signal coming from the controller
     * \return Validity of the signal. False signals are dropped from processing by the mods and not sent to
     * the console.
     *
     * This remapping is invoked from the chaos engine's sniffify routine before the list of regular
     * mods is traversed. That means that mods can operate on the signals associated with particular
     * commands without worrying about the state of the remapping.
     */
    //bool remapEvent(DeviceEvent& event) { return signal_table.remapEvent(event); }
    //bool remapEvent(DeviceEvent& event);

    /**
     * \brief Tests if an event matches this signal
     * \param event The incomming event from the controller
     * \param to_match The input signal we're testing for
     * \return Whether event id and type match the definition for this controller input.
     *
     * Remapping should be complete before the ordinary (non-remapping) mods see the event, so
     * it is safe to test for the actual events that the console expects.
     */
    bool matchesID(const DeviceEvent& event, ControllerSignal to_match) {
      return signal_table.matchesID(event, to_match);
    }

    GameMenu& getMenu() { return menu; }

  private:
    /**
     * The name of this game
     */
    std::string name;

    /**
     * Running count of total errors encountered in initializing the configuraiton file
     */
    int parse_errors;

    /**
     * \brief Do we use the game's menu system
     * 
     * If false, menu modifiers are disabled
     */
    bool use_menu;

    Controller& controller;

    /**
     * Defines the structure of the game's menu system
     */
    GameMenu menu;
    
    /**
     * Container for defined sequences
     */
    std::shared_ptr<SequenceTable> sequences;

    /**
     * Container for defined game commands
     */
    GameCommandTable game_commands;

    /**
     * Container for defined game conditions
     */
    GameConditionTable game_conditions;

    /**
     * Container for defined modifiers
     */
    ModifierTable modifiers;

    /**
     * \brief Number of modifiers simultaneously active
     * 
     * This count does not include child mods.
     */
    int active_modifiers;

    /**
    * Time in seconds modifiers last before they are removed from the queue.
    */
    double time_per_modifier;

    /**
     * Controller signal status, including remapping
     */
    ControllerInputTable signal_table;

    void makeMenu(toml::table& config);
    void addMenuItem(toml::table& config);

  };

};