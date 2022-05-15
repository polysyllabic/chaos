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

#include "gameMenu.hpp"
#include "signalTable.hpp"
#include "sequenceTable.hpp"

namespace Chaos {

  // Forward references
  class GameCommand;
  class GameCondition;
  class Modifier;

  /**
   * \brief Main container for the information needed to run a game
   * 
   * The Game class is the facade for all the various classes containing information we need to
   * inject chaos into a game.
   */
  class Game {
  public:
    Game() {}

    /**
     * \brief Load game configuration file
     * 
     * \param configfile Name of the configuration file
     * \return true If loaded with non-fatal errors
     * \return false If had a fatal error while loading
     * 
     * If we're reloading a new game, an early fatal error will leave the old game's data intact
     */
    bool loadConfigFile(const std::string& configfile);

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
     * \brief Accessor to GameCommand pointer by command name.
     *
     * \param name Name by which the game command is identified in the TOML file.
     * \return The GameCommand pointer for this command, or NULL if not found.
     */
    std::shared_ptr<GameCommand> getCommand(const std::string& name);

    /**
     * \brief Given the GameCondition name, get the object
     * 
     * \param name The name by which this GameCondition is identified in the TOML file
     * \return std::shared_ptr<GameCondition> Pointer to the GameCondition object.
     *
     * Conditions refer to game commands and game command can, optionally, refer to conditions. To
     * avoid infinite recursion, we prohibit conditions from referencing game commands that have
     * conditions themselves.
     */
    std::shared_ptr<GameCondition> getCondition(const std::string& name);

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Modifier> getModifier(const std::string& name);

    /**
     * Return list of modifiers for the chat bot.
     */
    std::string getModList();

    SignalTable& getSignalTable() { return signal_table; }

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
    bool matchesID(const DeviceEvent& event, ControllerSignal to_match);

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
     * Defines the structure of the game's menu system
     */
    GameMenu menu;
    
    /**
     * Container for defined sequences
     */
    SequenceTable sequences;

   /**
     * The map of game commands identified by their names in the TOML file
     */
    std::unordered_map<std::string, std::shared_ptr<GameCommand>> command_map;

    /**
     * The map of game conditions identified by their names in the TOML file.
     */
    std::unordered_map<std::string, std::shared_ptr<GameCondition>> condition_map;

    /**
     * The map of pre-defined sequences identified by their names in the TOML file.
     */
    std::unordered_map<std::string, std::shared_ptr<Sequence>> sequence_map;

    /**
     * The map of all the mods defined through the TOML file
     */
    std::unordered_map<std::string, std::shared_ptr<Modifier>> mod_map;

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
    SignalTable signal_table;

    /**
     * \brief Initialize the global map to hold the command definitions without conditions.
     *
     * \param config The object containing the complete parsed TOML file
     * \param no_conditions If true, only processes commands that have no conditions
     *
     * Because commands can reference conditions and conditions are based on commands, we need some
     * extra steps to avoid initialization deadlocks and infinite recursion. We therefore use a
     * two-stage initialization process. First, this routine initializes all "direct" commands
     * (those without conditions). Next, we initialize all conditions. Finally, we initialize
     * those commands based on a condition with #buildCommandMapCondition().
     *
     * This routine is called while parsing the game configuration file.
     */
    void buildCommandList(toml::table& config, bool conditions);

    /**
     * \brief Initializes the list of conditions from the TOML file.
     * 
     * \param config The object holding the parsed TOML file
     */
    void buildConditionList(toml::table& config);

    /**
     * \brief Factory for pre-defined sequences
     * 
     * \param config Fully parsed TOML object of the configuration file.
     *
     * Initializing the pre-defined sequences that can be referenced from other sequences and
     * the global accessor functions, e.g., for menu operations.
     */
    void buildSequenceList(toml::table& config);

    /**
     * \brief Create the overall list of mods from the TOML file.
     * \param config The object containing the fully parsed TOML file
     */
    void buildModList(toml::table& config);

    void initializeRemap(const toml::table& config);

    // Template function to get a pointer from a map by name
    template <typename T>
    std::shared_ptr<T> getFromMap(std::unordered_map<std::string,std::shared_ptr<T>>& map,
                                  const std::string& name) {
      auto iter = command_map.find(name);
      if (iter != command_map.end()) {
        return iter->second;
      }
      return nullptr;
    }

  };

};