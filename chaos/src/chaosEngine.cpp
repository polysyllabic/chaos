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

#include "chaosEngine.hpp"
#include "controllerInput.hpp"

using namespace Chaos;

ChaosEngine::ChaosEngine() : controller{Controller::instance()}, pause(true)
{
  controller.addInjector(this);
  time.initialize();
  chaosInterface.addObserver(this);
  jsonReader = jsonReaderBuilder.newCharReader();
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
    std::shared_ptr<Modifier> mod = Modifier::mod_list.at(root["winner"].asString());
    if (mod != nullptr) {
      PLOG_INFO << "Adding Modifier: " << typeid(*mod).name();
			
      lock();
      modifiers.push_back(mod);
      modifiersThatNeedToStart.push(mod);
      unlock();
    } else {
      PLOG_ERROR << "ERROR: Modifier not found: " << command;
    }
  }

  if (root.isMember("game")) {
    // Tell the interface what game we're playing
    PLOG_DEBUG << "Sending game to interface: " << game;
    Json::Value msg;
    msg["game"] = game;
    chaosInterface.sendMessage(Json::writeString(jsonWriterBuilder, msg));
  }

  if (root.isMember("activeMods")) {
    // Tell the interface how many active mods we support
    PLOG_DEBUG << "Sending activeMods to interface: " << activeModifiers;
    Json::Value msg;
    msg["activeModifiers"] = activeModifiers;
    chaosInterface.sendMessage(Json::writeString(jsonWriterBuilder, msg));
  }

  // timePerModifier is now fixed by the config file. This command now sends the time to the
  // interface instead of setting it.
  if (root.isMember("timePerModifier")) {
    PLOG_DEBUG << "Sending timePerModifier to interface: " << timePerModifier;
    Json::Value msg;
    msg["timePerModifier"] = timePerModifier;
    chaosInterface.sendMessage(Json::writeString(jsonWriterBuilder, msg));
  }

  if (root.isMember("modlist")) {
    // Announce the mods to the chaos interface
    std::string reply = Modifier::getModList();
    PLOG_DEBUG << "Json mod list = " << reply;
    chaosInterface.sendMessage(reply);
  }

  if (root.isMember("exit")) {
    keep_going = false;
  }
}

void ChaosEngine::doAction() {
  usleep(500);	// 200Hz
	
  // Update timers/states of modifiers
  if (pause) {
    pausedPrior = true;
    return;    
  }

  // initialize the mods that are waiting
  lock();
  while(!modifiersThatNeedToStart.empty()) {
    modifiersThatNeedToStart.front()->begin();
    modifiersThatNeedToStart.pop();
  }
  unlock();
	
  lock();
  for (auto& mod : modifiers) {
    (*mod)._update(pausedPrior);
  }
  pausedPrior = false;
  unlock();
	
  // Check front element.  If timer ran out, remove it.
  if (modifiers.size() > 0) {
    std::shared_ptr<Modifier> front = modifiers.front();
    if ((front->lifespan() >= 0 && front->lifetime() > front->lifespan()) ||
	      (front->lifespan()  < 0 && front->lifetime() > timePerModifier)) {
      PLOG_INFO << "Removing modifier: " << typeid(*front).name();
      lock();
      // Do cleanup for this mod, if necessary
      front->finish();
      // delete front;
      modifiers.pop_front();
      // Execute apply() on remaining modifiers for post-removal actions
      for (auto& m : modifiers) {
	      m->apply();
      }
      unlock();
    }
  }
}

// Tweak the event based on modifiers
bool ChaosEngine::sniffify(const DeviceEvent& input, DeviceEvent& output) {
  output = input;	
  bool valid = true;
  
  lock();
  
  // The options button pauses the chaos engine (always check real buttons for this, not a remap)
  if (ControllerInput::matchesID(input, ControllerSignal::OPTIONS) ||
      ControllerInput::matchesID(input, ControllerSignal::PS)) {
    if(input.value == 1 && pause == false) { // on rising edge
      pause = true;
      chaosInterface.sendMessage("{\"pause\":1}");
      pausePrimer = false;
      PLOG_INFO << "Game Paused";
    }
  }

  // The share button resumes the chaos engine (also not remapped)
  if (ControllerInput::matchesID(input, ControllerSignal::SHARE)) {
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

    // Translate incomming signal to what the console expects before passing that on to the mods
    valid = Modifier::remapEvent(output);
    if (valid) {
      for (auto& mod : modifiers) {
	valid = (*mod).tweak(output);
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
void ChaosEngine::fakePipelinedEvent(DeviceEvent& fakeEvent, std::shared_ptr<Modifier> modifierThatSentTheFakeEvent) {
  bool valid = true;
	
  if (!pause) {
    // Find the modifier that sent the fake event in the modifier list
    std::list<std::shared_ptr<Modifier>>::iterator mod =
      std::find_if(modifiers.begin(), modifiers.end(), [&](const auto& p) {
	      return p == modifierThatSentTheFakeEvent; } );
    
      // iterate from the next element till the end and apply any tweaks
      for ( mod++; mod != modifiers.end(); mod++) {
        valid = (*mod)->tweak(fakeEvent);
        if (!valid) {
	      break;
      }
    }
  }
  // unless canceled, send the event out
  if (valid) {
    controller.applyEvent(fakeEvent);
  }
}

void ChaosEngine::sendInterfaceMessage(const std::string& msg) {
  chaosInterface.sendMessage(msg);
}

