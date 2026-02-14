/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS
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

bool ChaosEngine::setGame(const std::string& name) {
  pause.store(true);
  pausePrimer = false;

  lock();
  bool loaded = game.loadConfigFile(name, this);
  bool playable = loaded && (game.getErrors() == 0);
  game_ready.store(playable);
  unlock();

  if (!playable) {
    PLOG_WARNING << "Game configuration '" << name
                 << "' has errors or failed to load. Staying paused until a valid game is loaded.";
  }
  return playable;
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
    lock();
    std::shared_ptr<Modifier> mod = game.getModifier(root["winner"].asString());
    double time_active = game.getTimePerModifier();
    if (mod != nullptr) {
      if (root.isMember("time")) {
        time_active = root["time"].asDouble();
      }
      PLOG_INFO << "Adding Modifier: " << mod->getName() << " lifespan = " << time_active;
      mod->setLifespan(time_active);
      modifiersThatNeedToStart.push(mod);
    } else {
      PLOG_ERROR << "ERROR: Modifier not found: " << command;
    }
    unlock();
  }
  
  // Manually remove a mod
  if (root.isMember("remove")) {
    lock();
    std::shared_ptr<Modifier> mod = game.getModifier(root["remove"].asString());
    unlock();
    if (mod != nullptr) {
      PLOG_INFO << "Manually removing modifier '" << mod->getName();
      removeMod(mod);
    } else {
      PLOG_ERROR << "ERROR: Modifier not found: " << command;
    }
  }

  // Remove all mods
  if (root.isMember("reset")) {
    while (true) {
      lock();
      bool no_mods = modifiers.empty();
      unlock();
      if (no_mods) {
        break;
      }
      removeOldestMod();
    }
  }

  if (root.isMember("game")) {
    reportGameStatus();
  }

  if (root.isMember("newgame")) {
    setGame(root["newgame"].asString());
    reportGameStatus();
  }

  if (root.isMember("nummods")) {
    int nmods = root["nummods"].asInt();
    if (nmods < 1) {
      PLOG_ERROR << "Number of active modifiers must be at least one";
    } else {
      lock();
      game.setNumActiveMods(nmods);
      unlock();
      // If we've reduced the number of mods to less than the current number of active mods, the
      // excess will be removed in the main loop
    }    
  }

  if (root.isMember("exit")) {
    keep_going.store(false);
  }
}

// Tell the interface about the game we're playing
void ChaosEngine::reportGameStatus() {
  PLOG_DEBUG << "Sending game information to the interface.";
  Json::Value msg;
  lock();
  msg["game"] = game.getName();
  msg["errors"] = game.getErrors();
  msg["nmods"] = game.getNumActiveMods();
  msg["can_unpause"] = game_ready.load();
  double t = std::chrono::duration<double>(game.getTimePerModifier()).count();
  msg["modtime"] = t;
  msg["mods"] = game.getModList();
  unlock();
  chaosInterface.sendMessage(Json::writeString(jsonWriterBuilder, msg));
}

void ChaosEngine::doAction() {
  usleep(500);	// sleep .5 milliseconds
		
  // Update timers/states of modifiers
  if (pause.load()) {
    pausedPrior = true;
    return;    
  }
  if (pausedPrior) {
    PLOG_DEBUG << "Resuming after pause";
  }
  // Initialize and update active mods
  std::shared_ptr<Modifier> mod_to_remove;
  lock();
  if (pause.load()) {
    unlock();
    pausedPrior = true;
    return;
  }
  while(!modifiersThatNeedToStart.empty()) {
    std::shared_ptr<Modifier> mod = modifiersThatNeedToStart.front();
    assert(mod);
    PLOG_DEBUG << "Initializing modifier " << mod->getName() << " lifespan = " << mod->lifespan();
    modifiers.push_back(mod);
    modifiersThatNeedToStart.pop();
    mod->_begin();
  }
  for (auto& mod : modifiers) {
    assert (mod);
    mod->_update(pausedPrior);
  }
  pausedPrior = false;
  
  // If we have too many mods, remove the oldest one
  if (modifiers.size() > game.getNumActiveMods()) {
    mod_to_remove = modifiers.front();
    for (auto& mod : modifiers) {
      if (mod_to_remove->lifetime() < mod->lifetime()) {
        mod_to_remove = mod;
      }
    }
  } else {
    // Check remaining mods for expiration.
    for (auto& mod : modifiers) {
      if (mod->lifetime() > mod->lifespan()) {
        mod_to_remove = mod;
        // Mods are added one at a time, so we can stop searching on the first expired mod
        break;
      }
    }
  }
  unlock();

  if (mod_to_remove) {
    removeMod(mod_to_remove);
  }
}

// Remove oldest mod whether or not it's expired. This keeps manually inserted mods from
// going beyond the specified modifier count.
void ChaosEngine::removeOldestMod() {
  PLOG_DEBUG << "Looking for oldest mod";
  std::shared_ptr<Modifier> oldest;
  lock();
  if (modifiers.size() > 0) {
    oldest = modifiers.front();
    for (auto& mod : modifiers) {
      if (oldest->lifetime() < mod->lifetime()) {
        oldest = mod;
      }
    }
  }
  unlock();
  if (oldest) {
    removeMod(oldest);
  }
}

void ChaosEngine::removeMod(std::shared_ptr<Modifier> to_remove) {
  assert(to_remove);

  lock();
  auto it = std::find(modifiers.begin(), modifiers.end(), to_remove);
  if (it == modifiers.end()) {
    unlock();
    return;
  }
  PLOG_INFO << "Removing '" << to_remove->getName() << "' from active mod list";
  PLOG_DEBUG << "Lifetime = " << to_remove->lifetime() << " of lifespan = " << to_remove->lifespan();
  // Do cleanup for this mod, if necessary
  to_remove->_finish();
  modifiers.erase(it);
  unlock();
}

// Tweak the event based on modifiers
bool ChaosEngine::sniffify(const DeviceEvent& input, DeviceEvent& output) {
  output = input;	
  bool valid = true;
  
  lock();
  // The options button pauses the chaos engine
  if (game.matchesID(input, ControllerSignal::OPTIONS) || game.matchesID(input, ControllerSignal::PS)) {
    if(input.value == 1 && pause.load() == false) { // on rising edge
      pause.store(true);
      chaosInterface.sendMessage("{\"pause\":1}");
      pausePrimer = false;
      PLOG_INFO << "Game Paused";
    }
  }

  // The share button resumes the chaos engine
  if (game.matchesID(input, ControllerSignal::SHARE)) {
    if(input.value == 1 && pause.load() == true) { // on rising edge
      if (game_ready.load()) {
        pausePrimer = true;
      } else {
        pausePrimer = false;
        PLOG_WARNING << "Ignoring unpause command: no valid game configuration loaded.";
      }
    } else if(input.value == 0 && pausePrimer == true) { // falling edge
      pause.store(false);
      chaosInterface.sendMessage("{\"pause\":0}");
      PLOG_INFO << "Game Resumed";
    }
    output.value = 0;
  }
  unlock();
	
  if (!pause.load()) {
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
		
  if (!pause.load()) {
    lock();
    if (pause.load()) {
      unlock();
      return;
    }
    // Find the modifier that sent the fake event in the modifier list
    auto mod = std::find_if(modifiers.begin(), modifiers.end(),
                            [&sourceMod](const auto& p) { return p == sourceMod; } );
    
    // iterate from the next element till the end and apply any tweaks
    if (mod != modifiers.end()) {
      for (mod++; mod != modifiers.end(); mod++) {
        valid = (*mod)->_tweak(event);
        if (!valid) {
		      break;
        }
      }
    }
    unlock();
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
