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
#include <mogi/thread.h>
#include <memory>
#include <list>
#include <string>
#include <queue>

#include "chaosInterface.hpp"	// for CommandListener
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
  class ChaosEngine : public CommandListenerObserver, public ControllerInjector, public Mogi::Thread {
  private:
    ChaosInterface chaosInterface;
    Controller* controller;
	
    Mogi::Math::Time time;
    double timePerModifier;
    /**
     * The list of currently active modifiers
     */
    std::list<std::shared_ptr<Modifier>> modifiers;
    /**
     * List of modifiers that have been selected but not yet initialized.
     */
    std::queue<std::shared_ptr<Modifier>> modifiersThatNeedToStart;
	
    bool pause = true;
    bool pausePrimer = false;
    bool pausedPrior = false;

    // overridden from ControllerInjector
    bool sniffify(const DeviceEvent* input, DeviceEvent* output);
    // overridden from Mogi::Thread
    void doAction();
	
  public:
    ChaosEngine(Controller* controller);
	
    void setInterfaceReply(const std::string& reply);
    
    inline void setTimePerModifier(double time) { timePerModifier = time; }
    
    void fakePipelinedEvent(DeviceEvent* fakeEvent, std::shared_ptr<Modifier> modifierThatSentTheFakeEvent);

    void newCommand(const std::string& command);	// override from CommandListenerObserver

    inline bool isPaused() { return pause; }

  };

};

