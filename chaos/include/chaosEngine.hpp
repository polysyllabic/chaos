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
#include <mogi/thread.h>
#include <memory>
#include <list>
#include <string>
#include <queue>
#include <json/json.h>

#include "chaosInterface.hpp"
#include "controller.hpp"
#include "modifier.hpp"
#include "game.hpp"

namespace Chaos {

  /**
   * \brief The engine that adds and removes modifiers at the appropraite time.
   * 
   * The chaos engine listens to the Python voting system using ZMQ with zmqpp. When a winning modifier
   * comes in, chaos engine adds the modifier to list of active modifiers. After a set amount of time,
   * the chaos engine will remove the modifier.
   */
  class ChaosEngine : public CommandObserver, public ControllerInjector, public Mogi::Thread {
  private:
    ChaosInterface chaosInterface;

    Controller& controller;
	
    Mogi::Math::Time time;

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

    // overridden from ControllerInjector
    bool sniffify(const DeviceEvent& input, DeviceEvent& output);
    // overridden from Mogi::Thread
    void doAction();

    Json::CharReaderBuilder jsonReaderBuilder;
    Json::CharReader* jsonReader;
    Json::StreamWriterBuilder jsonWriterBuilder;	

  public:
    ChaosEngine(Controller& c, const std::string& configfile);
    
/*    ChaosEngine(Controller& c, const std::string& gamefile) : controller{c} {
      game.loadConfigFile(gamefile);
    }*/

    //void setController(Controller& c) { controller = c; }

    void sendInterfaceMessage(const std::string& msg);
    

    void setGame(const std::string& name) {
      game.loadConfigFile(name);
    } 

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

    // override from CommandListenerObserver
    void newCommand(const std::string& command);

    bool isPaused() { return pause; }

    bool keepGoing() { return keep_going; }
  };

};
