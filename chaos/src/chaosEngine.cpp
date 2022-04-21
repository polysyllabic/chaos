/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
#include <json/json.h>
#include <plog/Log.h>
#include <toml++/toml.h>

#include "chaosEngine.hpp"
#include "gamepadInput.hpp"

using namespace Chaos;

ChaosEngine::ChaosEngine() : controller{Controller::instance()}, pause(true)
{
  controller.addInjector(this);
  time.initialize();
  chaosInterface.addObserver(this);
}

void ChaosEngine::newCommand(const std::string& command) {
  PLOG_VERBOSE << "Chaos::ChaosEngine::newCommand() received: " << command << std::endl;
	
  Json::Value root;
  Json::CharReaderBuilder builder;
  Json::CharReader* reader = builder.newCharReader();
  std::string errs;
  bool parsingSuccessful = reader->parse(command.c_str(), command.c_str() + command.length(), &root, &errs);
	
  if (!parsingSuccessful) {
    PLOG_ERROR << " - Unable to parse this message!" << std::endl;
  }
	
  if (root.isMember("winner")) {
    std::shared_ptr<Modifier> mod = Modifier::mod_list.at(root["winner"].asString());
    if (mod != nullptr) {
      PLOG_DEBUG << "Adding Modifier: " << typeid(*mod).name() << std::endl;
			
      lock();
      modifiers.push_back(mod);
      modifiersThatNeedToStart.push(mod);
      unlock();
    } else {
      PLOG_ERROR << "ERROR: Unable to build Modifier for key: " << command << std::endl;
    }
  }
	
  if (root.isMember("timePerModifier")) {
    int newModifierTime = root["timePerModifier"].asFloat();
    PLOG_DEBUG << "New Modifier Time: " << newModifierTime << std::endl;
    setTimePerModifier(newModifierTime);
  }
	
}

void ChaosEngine::doAction() {
  usleep(500);	// 200Hz
	
  // Update timers/states of modifiers
  if(pause) {
    //std::cout << "Paused" << std::endl;
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
      PLOG_DEBUG << "Killing Modifier: " << typeid(*front).name() << std::endl;
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
  if (GamepadInput::matchesID(input, GPInput::OPTIONS) ||
      GamepadInput::matchesID(input, GPInput::PS)) {
    if(input.value == 1 && pause == false) { // on rising edge
      pause = true;
      chaosInterface.sendMessage("{\"pause\":1}");
      pausePrimer = false;
      PLOG_INFO << "Paused" << std::endl;
    }
  }

  // The share button resumes the chaos engine (also not remapped)
  if (GamepadInput::matchesID(input, GPInput::SHARE)) {
    if(input.value == 1 && pause == true) { // on rising edge
      pausePrimer = true;
    } else if(input.value == 0 && pausePrimer == true) { // falling edge
      pause = false;
      chaosInterface.sendMessage("{\"pause\":0}");
      PLOG_INFO << "Resumed" << std::endl;
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

void ChaosEngine::setInterfaceReply(const std::string& reply) {
  chaosInterface.sendMessage(reply);
}


