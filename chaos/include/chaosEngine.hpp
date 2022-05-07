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

    // Name of the game we're playing
    std::string game;

    /**
     * \brief Number of modifiers simultaneously active
     * 
     * This count does not include child mods.
     */
    int activeModifiers;

    /**
    * Time in seconds modifiers last before they are removed from the queue.
    */
    double timePerModifier;

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

  protected:	
    ChaosEngine();

  public:
    static ChaosEngine& instance() {
      static ChaosEngine engine{};
      return engine;
    }

    void sendInterfaceMessage(const std::string& msg);
    
    void setTimePerModifier(double time) {
      assert(time > 0);  
      timePerModifier = time; 
      }
    
    void setActiveMods(int nmods) {
      assert(nmods > 0);
      activeModifiers = nmods;
    }

    void setGame(const std::string& name) {
      game = name;
    } 

    void fakePipelinedEvent(DeviceEvent& fakeEvent, std::shared_ptr<Modifier> modifierThatSentTheFakeEvent);

    void newCommand(const std::string& command);	// override from CommandListenerObserver

    bool isPaused() { return pause; }

    bool keepGoing() { return keep_going; }
  };

};
