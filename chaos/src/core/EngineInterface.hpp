/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributors.
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
#include <vector>
#include <memory>
#include <list>
#include "DeviceEvent.hpp"

// This gathers all the various data into a single facade run through the engine
// Should we split this into multiple interfaces? Unless we need to speed things up, it's
// probably better only to keep a pointer to a single facade (the engine)
namespace Chaos {
  class Modifier;
  class GameCommand;
  class MenuItem;
  class ControllerInput;
  class SignalRemap;
  class GameCondition;
  class Sequence;
  
  class EngineInterface {
  public:
    // Engine
    /**
     * \brief Query whether engine processing is currently paused.
     */
    virtual bool isPaused() = 0;

    /**
     * \brief Inject a synthetic event back into the modifier pipeline.
     *
     * \param event Event to inject.
     * \param sourceMod Modifier that originated the event.
     */
    virtual void fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod) = 0;
  
    // These functions access the controller
    /**
     * \brief Read the current raw controller state by type/id.
     */
    virtual short getState(uint8_t id, uint8_t type) = 0;

    /**
     * \brief Test whether a raw event matches a game command in current conditions.
     */
    virtual bool eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) = 0;

    /**
     * \brief Set a game command to its "off" state.
     */
    virtual void setOff(std::shared_ptr<GameCommand> command) = 0;

    /**
     * \brief Set a game command to its "on" state.
     */
    virtual void setOn(std::shared_ptr<GameCommand> command) = 0 ;

    /**
     * \brief Set a game command to an explicit numeric value.
     */
    virtual void setValue(std::shared_ptr<GameCommand> command, short value) = 0 ;

    /**
     * \brief Apply an already-resolved raw event to controller output.
     */
    virtual void applyEvent(const DeviceEvent& event) = 0;
    
    // Functions to get modifier data
    /**
     * \brief Lookup a modifier by name.
     */
    virtual std::shared_ptr<Modifier> getModifier(const std::string& name) = 0;

    /**
     * \brief Access the modifier map keyed by configured name.
     */
    virtual std::unordered_map<std::string, std::shared_ptr<Modifier>>& getModifierMap() = 0;

    /**
     * \brief Access the currently active modifier list.
     */
    virtual std::list<std::shared_ptr<Modifier>>& getActiveMods() = 0;

    // These functions access the menu system
    /**
     * \brief Lookup a menu item by configured name.
     */
    virtual std::shared_ptr<MenuItem> getMenuItem(const std::string& name) = 0;

    /**
     * \brief Set a menu item to a specific option value.
     */
    virtual void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) = 0;

    /**
     * \brief Restore a menu item to its default state.
     */
    virtual void restoreMenuState(std::shared_ptr<MenuItem> item) = 0;

    // These functions access the controller inputs (signals)
    /**
     * \brief Lookup a controller input by configured name.
     */
    virtual std::shared_ptr<ControllerInput> getInput(const std::string& name) = 0;

    /**
     * \brief Resolve a raw event to its controller-input definition.
     */
    virtual std::shared_ptr<ControllerInput> getInput(const DeviceEvent& event) = 0;

    /**
     * \brief Resolve controller-input references from config into a vector.
     */
    virtual void addControllerInputs(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<ControllerInput>>& vec) = 0;

    /**
     * \brief Resolve the canonical input name for a raw event.
     */
    virtual std::string getEventName(const DeviceEvent&) = 0;
    
    // Game Commands
    /**
     * \brief Resolve game-command references from config into a command vector.
     */
    virtual void addGameCommands(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<GameCommand>>& vec) = 0;

    /**
     * \brief Resolve game-command references from config into controller inputs.
     */
    virtual void addGameCommands(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<ControllerInput>>& vec) = 0;
    // Game Conditions
    /**
     * \brief Resolve game-condition references from config into a vector.
     */
    virtual void addGameConditions(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<GameCondition>>& vec) = 0;
    // Sequences
    /**
     * \brief Build a sequence from configuration.
     *
     * \param config Configuration table.
     * \param key Sequence key/name in config.
     * \param required Whether absence is an error.
     */
    virtual std::shared_ptr<Sequence> createSequence(toml::table& config,
                                                     const std::string& key, bool required) = 0;
  };

};
