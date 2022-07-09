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
#include <thread.hpp>
#include <memory>
#include <list>
#include <string>
#include <queue>
#include <json/json.h>
#include <timer.hpp>

#include "ChaosInterface.hpp"
#include "Controller.hpp"
#include "Modifier.hpp"
#include "Game.hpp"

namespace Chaos {

  /**
   * \brief The engine that adds and removes modifiers at the appropraite time.
   * 
   * The chaos engine listens to the Python voting system using ZMQ with zmqpp. When a winning modifier
   * comes in, chaos engine adds the modifier to list of active modifiers. After a set amount of time,
   * the chaos engine will remove the modifier.
   */
  class ChaosEngine : public CommandObserver, public ControllerInjector,
                      public Thread, public EngineInterface {
  private:
    ChaosInterface chaosInterface;

    Controller& controller;	

    Timer time;

    // Data for the game we're playing
    Game game;

    /**
     * The list of currently active modifiers
     */
    std::list<std::shared_ptr<Modifier>> modifiers;

    /**
     * List of modifiers that have been selected but not yet initialized.
     */
    std::queue<std::shared_ptr<Modifier>> modifiersThatNeedToStart;
	
    bool keep_going = true;
    bool pause = true;
    bool pausePrimer = false;
    bool pausedPrior = false;
    int primary_mods = 0;
    
    // overridden from ControllerInjector
    bool sniffify(const DeviceEvent& input, DeviceEvent& output);

    // overridden from Thread
    void doAction();

    Json::CharReaderBuilder jsonReaderBuilder;
    Json::CharReader* jsonReader;
    Json::StreamWriterBuilder jsonWriterBuilder;	

    void reportGameStatus();

    void removeMod(std::shared_ptr<Modifier> mod);

  public:
    ChaosEngine(Controller& c, const std::string& listener_endpoint,
                const std::string& talker_endpoint);
    
    void loadConfigFile(const std::string& configfile, EngineInterface* engine);

    void sendInterfaceMessage(const std::string& msg);

    void setGame(const std::string& name) {
      game.loadConfigFile(name, this);
    } 

    /**
     * \brief Get the list of mods that are currently active
     * 
     * \return std::list<std::shared_ptr<Modifier>> 
     */
    std::list<std::shared_ptr<Modifier>>& getActiveMods() { return modifiers; }

    /**
     * \brief Insert a new event into the event queue
     * 
     * \param event The fake event to insert
     * \param sourceMod The modifier that inserted the event
     * 
     * This command is used to insert a new event in the event pipeline so that other mods in the
     * active list can also act on it.
     */
    void fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod);

    /**
     * \brief Remove mod with the least time remaining
     */
    void removeOldestMod();

    short getState(uint8_t id, uint8_t type) { return controller.getState(id, type); }

    bool eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command);

    void setOff(std::shared_ptr<GameCommand> command);
    
    void setOn(std::shared_ptr<GameCommand> command);

    void applyEvent(const DeviceEvent& event) { controller.applyEvent(event); }

    std::shared_ptr<Modifier> getModifier(const std::string& name) {
      return game.getModifier(name);
    }

    std::unordered_map<std::string, std::shared_ptr<Modifier>>& getModifierMap() {
      return game.getModifierMap();
    }

    std::shared_ptr<MenuItem> getMenuItem(const std::string& name) {
      return game.getMenu().getMenuItem(name);
    }

    /**
     * \brief Sets a menu item to the specified value
     * \param item The menu item to change
     * \param new_val The new value of the item
     * 
     * The menu item must be settable (i.e., not a submenu)
     */
    void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) {
      game.getMenu().setState(item, new_val, controller);
    }

    /**
     * \brief Restores a menu to its default state
     * \param item The menu item to restore
     *
     * The menu item must be settable (i.e., not a submenu)
     */
    void restoreMenuState(std::shared_ptr<MenuItem> item) {
      game.getMenu().restoreState(item, controller);
    }

    std::shared_ptr<ControllerInput> getInput(const std::string& name) {
      return game.getSignalTable().getInput(name);
    }

    std::shared_ptr<ControllerInput> getInput(const DeviceEvent& event) {
      return game.getSignalTable().getInput(event);
    }

    void addControllerInputs(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<ControllerInput>>& vec) {
      game.getSignalTable().addToVector(config, key, vec);
    }

    void addGameCommands(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<GameCommand>>& vec) {
      game.getGameCommandTable().addToVector(config, key, vec);
    }

    void addGameConditions(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<GameCondition>>& vec) {
      game.getGameConditionTable().addToVector(config, key, vec);
    }

    std::shared_ptr<Sequence> createSequence(toml::table& config, const std::string& key,
                                             bool required) {
      return game.getSequenceTable()->makeSequence(config, key, game.getGameCommandTable(),
                                                   controller, required);
}/**
     * \brief Is the event an instance of the specified input command?
     * 
     * This tests both that the event against the defined signal and that any defined condition
     * is also in effect.
     */
    bool matches(const DeviceEvent& event, std::shared_ptr<GameCommand> command);
 
    // override from CommandListenerObserver
    void newCommand(const std::string& command);

    bool isPaused() { return pause; }

    bool keepGoing() { return keep_going; }
  };

};
