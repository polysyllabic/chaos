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
#include <json/json.h>
#include <timer.hpp>

#include "GameMenu.hpp"
#include "ControllerInputTable.hpp"
//#include "GameCommandTable.hpp"
//#include "GameConditionTable.hpp"
//#include "ConditionTrigger.hpp"
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
   * inject chaos into a game. It also contains most of the logic to translate the TOML
   * configuration file into the actual objects we use during operation.
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
     * If we're reloading a new game, an early fatal error will leave the old game's data intact.
     * A count of the number of parsing errors encountered is kept in #parse_errors so that it can
     * be reported to the interface.
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

    /**
     * \brief Get the maximum number of active primary mods to run
     * 
     * \return int Number of modifiers
     * 
     * This number does not include the children of parent modifiers.
     */
    int getNumActiveMods() { return active_modifiers; }

    /**
     * \brief Set the maximum number of active primary mods to run
     * 
     * \param new_mods Number of modifiers
     * 
     * This number does not include the children of parent modifiers.
     */
    void setNumActiveMods(int new_mods) { active_modifiers = new_mods; }


    /**
     * \brief Get the default time that a modifier remains active before it is removed.
     * 
     * \return double 
     */
    double getTimePerModifier() { return time_per_modifier; }

    /**
     * \brief Get the number of errors encountered loading the game-configuration file
     * 
     * \return int Error count
     */
    int getErrors() { return parse_errors; }

    /**
     * \brief Get the modifier's pointer base don its name
     * 
     * \param name The name by which this modifier is identified in the TOML file
     * \return std::shared_ptr<Sequence> Pointer to the Modifier object.
     * 
     * This name is case sensitive and is used in communication with the interface.
     */
    std::shared_ptr<Modifier> getModifier(const std::string& name) { return modifiers.getModifier(name); }

    int getNumModifiers() { return modifiers.getNumModifiers(); } 

    std::unordered_map<std::string, std::shared_ptr<Modifier>>& getModifierMap() { return modifiers.getModMap();}

    Json::Value getModList() { return modifiers.getModList(); }

    //GameCommandTable& getGameCommandTable() { return game_commands; }
    //GameConditionTable& getGameConditionTable() { return game_conditions; }
    ControllerInputTable& getSignalTable() { return signal_table; }
    //std::shared_ptr<SequenceTable> getSequenceTable() { return sequences; }
    
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

    std::shared_ptr<GameCommand> getCommand(const std::string& name);
    std::shared_ptr<GameCommand> getCommand(const toml::table& config, const std::string& key,
                                            bool required);

    void addGameCommands(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<GameCommand>>& vec);
    
    /**
     * \brief Translates a list of game commands from the TOML file into the equivalent controller
     * input signals
     * 
     * \param config 
     * \param key 
     * \param vec 
     */
    void addGameCommands(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<ControllerInput>>& vec);

    std::shared_ptr<GameCondition> getCondition(const std::string& name);

    void addGameConditions(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<GameCondition>>& vec);

    
    short getSignalThreshold(std::shared_ptr<GameCommand> command, double proportion);

    /**
     * \brief Append sequence to the end of the current one by name
     * 
     * \param seq The sequence that is being created
     * \param name The name of the defined sequence as given in the TOML file
     */
    void addSequence(Sequence& seq, const std::string& name);

    /**
     * \brief Construct a sequence from a TOML file definition
     * 
     * \param config The parsed TOML table object
     * \param key The name of the 
     * \param required 
     * \return std::shared_ptr<Sequence> 
     */
    std::shared_ptr<Sequence> makeSequence(toml::table& config, 
                                           const std::string& key,
                                           bool required);


    
  private:
    /**
     * The name of this game
     */
    std::string name;

    /**
     * \brief Running count of total errors encountered in initializing the configuration file
     */
    int parse_errors;

    /**
     * \brief Running count of total warnings encountered in initializing the configuration file
     */
    int parse_warnings;

    /**
     * \brief Do we use the game's menu system?
     * 
     * If false, menu modifiers are disabled.
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
    //std::unordered_map<std::string, std::shared_ptr<Sequence>> sequences;

    /**
     * Container for defined game commands
     */
    //GameCommandTable game_commands;
    std::unordered_map<std::string, std::shared_ptr<GameCommand>> game_commands;

    /**
     * Container for defined game conditions
     */
    // GameConditionTable game_conditions;
    std::unordered_map<std::string, std::shared_ptr<GameCondition>> game_conditions;

    //std::unordered_map<std::string, std::shared_ptr<ConditionTrigger>> condition_triggers;

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
    * \brief Default time in seconds that modifiers last before they are removed from the active
    * modifier list.
    */
    double time_per_modifier;

    /**
     * Controller signal definitions
     */
    ControllerInputTable signal_table;
    //std::unordered_map<std::string, std::shared_ptr<ControllerInput>> controller_signals;

    void buildCommandList(toml::table& config);
    void buildConditionList(toml::table& config);
    //void buildTriggerList(toml::table& config);
    void buildSequenceList(toml::table& config);

    std::shared_ptr<GameCondition> makeCondition(toml::table& config);
    //std::shared_ptr<ConditionTrigger> makeTrigger(toml::table& config);

    void makeMenu(toml::table& config);
    void addMenuItem(toml::table& config);

  };

};