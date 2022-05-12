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
#include "gameCommand.hpp"
#include "gameCondition.hpp"

namespace Chaos {

  /**
   * \brief Main container for the information needed to run a game
   * 
   * The game data is constructed by parsing a TOML configuration file
   */
  class Game {
  public:
    Game();
    Game(const std::string& configfile);

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

    int getParseErrors() { return parse_errors; }
    
    /**
     * \brief Accessor to GameCommand pointer by command name.
     *
     * \param name Name by which the game command is identified in the TOML file.
     * \return The GameCommand pointer for this command, or NULL if not found.
     */
    std::shared_ptr<GameCommand> getCommand(const std::string& name) {
      return getFromMap<GameCommand>(command_map, name);
    }

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
    std::shared_ptr<GameCondition> getCondition(const std::string& name) {
      return getFromMap<GameCondition>(condition_map, name);
    }

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Sequence> getSequence(const std::string& name) {
      return getFromMap<Sequence>(sequence_map, name);
    }

    /**
     * \brief Given the sequence name, get the object
     * 
     * \param name The name by which this sequence is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Sequence object.
     */
    std::shared_ptr<Modifier> getModifier(const std::string& name) {
      return getFromMap<Modifier>(mod_map, name);
    }

    /**
     * Return list of modifiers for the chat bot.
     */
    std::string getModList();


  private:
    std::string name;    
    int parse_errors;

    /**
     * Description of the game's menu system
     */
    GameMenu menu;
    
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
    void buildCommandMap(toml::table& config, bool conditions);

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