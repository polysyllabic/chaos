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
#include <iostream>
#include <memory>
#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <plog/Log.h>
#include <toml++/toml.h>

#include "ChaosEngine.hpp"
#include <ControllerInput.hpp>
#include "Sequence.hpp"

using namespace Chaos;

ChaosEngine::ChaosEngine(Controller& c, const std::string& listener_endpoint, const std::string& talker_endpoint) : 
  controller{c}, game{c}, pause{true}
{
  time.initialize();
  jsonReader = jsonReaderBuilder.newCharReader();
  controller.addInjector(this);
  chaosInterface.setObserver(this);
  chaosInterface.setupInterface(listener_endpoint, talker_endpoint);
}

void ChaosEngine::newCommand(const std::string& command) {
  PLOG_DEBUG << "Received command: " << command;
	
  Json::Value root;
  std::string errs;
  bool parsingSuccessful = jsonReader->parse(command.c_str(), command.c_str() + command.length(), &root, &errs);
	
  if (!parsingSuccessful) {
    PLOG_ERROR << "Json parsing failed: " << errs << "; command = " << command;
  }
	
  if (root.isMember("winner")) {
    std::shared_ptr<Modifier> mod = game.getModifier(root["winner"].asString());
    if (mod != nullptr) {
      lock();
      PLOG_INFO << "Adding Modifier: " << mod->getName();
      modifiersThatNeedToStart.push(mod);
      //mod->_begin();
      unlock();
    } else {
      PLOG_ERROR << "ERROR: Modifier not found: " << command;
    }
  }
  
  // Manually remove a mod
  if (root.isMember("remove")) {
    std::shared_ptr<Modifier> mod = game.getModifier(root["remove"].asString());
    if (mod != nullptr) {
      PLOG_INFO << "Manually removing modifier '" << mod->getName();
      removeMod(mod);
    } else {
      PLOG_ERROR << "ERROR: Modifier not found: " << command;
    }
  }

  // Remove all mods
  if (root.isMember("reset")) {
    while (! modifiers.empty()) {
      removeOldestMod();
    }
  }

  if (root.isMember("game")) {
    reportGameStatus();
  }

  if (root.isMember("newgame")) {
    lock();
    pause = true;
    // Load a new game file
    game.loadConfigFile(root["newgame"].asString(), this);
    unlock();
    reportGameStatus();
  }

  // TODO: report available games
  
  if (root.isMember("exit")) {
    keep_going = false;
  }
}

// Tell the interface about the game we're playing
void ChaosEngine::reportGameStatus() {
  PLOG_DEBUG << "Sending game information for " << game.getName() << " to the interface.";
  Json::Value msg;
  msg["game"] = game.getName();
  msg["errors"] = game.getErrors();
  msg["nmods"] = game.getNumActiveMods();
  double t = std::chrono::duration<double>(game.getTimePerModifier()).count();
  msg["modtime"] = t;
  msg["mods"] = game.getModList();
  chaosInterface.sendMessage(Json::writeString(jsonWriterBuilder, msg));
}

void ChaosEngine::doAction() {
  usleep(500);	// sleep .5 milliseconds
	
  // Update timers/states of modifiers
  if (pause) {
    pausedPrior = true;
    return;    
  }
  if (pausedPrior) {
    PLOG_DEBUG << "Resuming after pause";
  }
  // Initialize the mods that are waiting
  lock();  
  while(!modifiersThatNeedToStart.empty()) {
    std::shared_ptr<Modifier> mod = modifiersThatNeedToStart.front();
    assert(mod);
    PLOG_DEBUG << "Initializing modifier " << mod->getName();
    modifiers.push_back(mod);
    modifiersThatNeedToStart.pop();
    mod->_begin();
  }
  unlock();
	
  lock();
  for (auto& mod : modifiers) {
    assert (mod);
    PLOG_DEBUG << "invoking _update for " << mod->getName();
    mod->_update(pausedPrior);
  }
  pausedPrior = false;
  unlock();
	
  // Check front element for expiration. 
  if (modifiers.size() > 0) {
    std::shared_ptr<Modifier> front = modifiers.front();
    assert(front);
    PLOG_DEBUG << "front modifier " << front->getName() << " lifespan =" << front->lifespan() << ", lifetime = " << front->lifetime();
    if ((front->lifespan() >= 0 && front->lifetime() > front->lifespan()) ||
	      (front->lifespan() <  0 && front->lifetime() > game.getTimePerModifier())) {
      removeMod(front);
    }
  } if (modifiers.size() > game.getNumActiveMods()) {
    // If we have too many mods as the result of a manual apply, remove the oldest one
    PLOG_DEBUG << " Have " << modifiers.size() << " mods; limit is " << game.getNumActiveMods();
    removeOldestMod();
  }

}

// Remove oldest mod whether or not it's expired. This keeps manually inserted mods from
// going beyond the specified modifier count.
void ChaosEngine::removeOldestMod() {
  PLOG_DEBUG << "Looking for oldest mod";
  if (modifiers.size() > 0) {
    std::shared_ptr<Modifier> oldest = nullptr;
    for (auto& mod : modifiers) {
      if (!oldest || oldest->lifetime() > mod->lifetime()) {
        oldest = mod;
      }
    }
    assert(oldest);
    removeMod(oldest);
  }
}

void ChaosEngine::removeMod(std::shared_ptr<Modifier> to_remove) {
  assert(to_remove);
  PLOG_INFO << "Removing '" << to_remove->getName() << "' from active mod list";
  PLOG_DEBUG << "Lifetime on removal = " << to_remove->getLifetime();
  lock();
  // Do cleanup for this mod, if necessary
  to_remove->_finish();
  modifiers.remove(to_remove);
  unlock();
}

// Tweak the event based on modifiers
bool ChaosEngine::sniffify(const DeviceEvent& input, DeviceEvent& output) {
  output = input;	
  bool valid = true;
  
  lock();
  // The options button pauses the chaos engine
  if (game.matchesID(input, ControllerSignal::OPTIONS) || game.matchesID(input, ControllerSignal::PS)) {
    if(input.value == 1 && pause == false) { // on rising edge
      pause = true;
      chaosInterface.sendMessage("{\"pause\":1}");
      pausePrimer = false;
      PLOG_INFO << "Game Paused";
    }
  }

  // The share button resumes the chaos engine
  if (game.matchesID(input, ControllerSignal::SHARE)) {
    if(input.value == 1 && pause == true) { // on rising edge
      pausePrimer = true;
    } else if(input.value == 0 && pausePrimer == true) { // falling edge
      pause = false;
      chaosInterface.sendMessage("{\"pause\":0}");
      PLOG_INFO << "Game Resumed";
    }
    output.value = 0;
  }
  unlock();
	
  if (!pause) {
    lock();
    // First call all remaps to translate the incomming signal 
    for (auto& mod : modifiers) {
      valid = mod->remap(output);
      if (!valid) {
        break;
      }
    }
    // Now pass to the regular tweak routines, which always see the fully remapped event
    if (valid) {
      for (auto& mod : modifiers) {
	      valid = mod->_tweak(output);
	      if (!valid) {
	        break;
	      }
      }
    }
    unlock();
  }
  return valid;
}

// This function is called by modifiers to inject a fake event into the event pipeline.
// Because the event can be modified by other mods, we find the location of the modifier
// and feed the fake event through the rest of the modifiers, in order.
void ChaosEngine::fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod) {
  bool valid = true;
	
  if (!pause) {
    // Find the modifier that sent the fake event in the modifier list
    auto mod = std::find_if(modifiers.begin(), modifiers.end(),
                            [&sourceMod](const auto& p) { return p == sourceMod; } );
    
    // iterate from the next element till the end and apply any tweaks
    for ( mod++; mod != modifiers.end(); mod++) {
      valid = (*mod)->_tweak(event);
      if (!valid) {
	      break;
      }
    }
  }
  // unless canceled, send the event out
  if (valid) {
    controller.applyEvent(event);
  }
}

void ChaosEngine::sendInterfaceMessage(const std::string& msg) {
  chaosInterface.sendMessage(msg);
}

bool ChaosEngine::eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) { 
  std::shared_ptr<ControllerInput> signal = command->getInput();
  return controller.matches(event, signal); 
}

void ChaosEngine::setOff(std::shared_ptr<GameCommand> command) {
  std::shared_ptr<ControllerInput> signal = command->getInput();
  controller.setOff(signal);
}
    
void ChaosEngine::setOn(std::shared_ptr<GameCommand> command) {
  std::shared_ptr<ControllerInput> signal = command->getInput();
  controller.setOn(signal);
}

void ChaosEngine::setValue(std::shared_ptr<GameCommand> command, short value) {
  std::shared_ptr<ControllerInput> signal = command->getInput();
  controller.setValue(signal, value);
}
