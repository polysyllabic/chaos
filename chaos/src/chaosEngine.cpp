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
#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <json/json.h>
#include <plog/Log.h>

#include "chaosEngine.hpp"

using namespace Chaos;

ChaosEngine::ChaosEngine(Controller* controller)
  : controller{controller}, timePerModifier{30.0}, pause{true}
{
  controller->addInjector(this);
  time.initialize();
  chaosInterface.addObserver(this);
}

void ChaosEngine::newCommand(const std::string& command) {
  PLOG_INFO << "Chaos::ChaosEngine::newCommand() received: " << command << std::endl;
	
  Json::Value root;
  Json::CharReaderBuilder builder;
  Json::CharReader* reader = builder.newCharReader();
  std::string errs;
  bool parsingSuccessful = reader->parse(command.c_str(), command.c_str() + command.length(), &root, &errs);
	
  if (!parsingSuccessful) {
    PLOG_ERROR << " - Unable to parse this message!" << std::endl;
  }
	
  if (root.isMember("winner")) {
    Modifier* mod = Modifier::build(root["winner"].asString());
    if (mod != NULL) {
      PLOG_DEBUG << "Adding Modifier: " << typeid(*mod).name() << std::endl;
			
      mod->setController(controller);
      mod->setChaosEngine(this);
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
    printf("New Modifier Time: %d\n", newModifierTime);
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
	
  lock();
  while(!modifiersThatNeedToStart.empty()) {
    modifiersThatNeedToStart.front()->begin();
    modifiersThatNeedToStart.pop();
  }
  unlock();
	
  lock();
  for (std::list<Modifier*>::iterator it = modifiers.begin(); it != modifiers.end(); it++) {
    (*it)->_update(pausedPrior);
  }
  pausedPrior = false;
  unlock();
	
  // Check front element.  If timer ran out, remove it.
  if (modifiers.size() > 0) {
    Modifier* front = modifiers.front();
    if (front->lifetime() > timePerModifier) {
      PLOG_DEBUG << "Killing Modifier: " << typeid(*front).name() << std::endl;
      lock();
      front->finish();	// Do any cleanup, if necessary
      modifiers.pop_front();
      delete front;
      unlock();
    }
  }
}

// Tweak the event based on modifiers
bool ChaosEngine::sniffify(const DeviceEvent* input, DeviceEvent* output) {
  *output = *input;
	
  bool valid = true;
  lock();
  
  // The options button pauses the chaos engine
  if (input->type == TYPE_BUTTON &&
      (input->id == controller->getButton(Button::OPTIONS) || input->id == controller->getButton(Button::PS)) ) {
    if(input->value == 1 && pause == false) { // on rising edge
      pause = true;
      chaosInterface.sendMessage("{\"pause\":1}");
      pausePrimer = false;
      PLOG_INFO << "Paused" << std::endl;
    }
  }

  // The share button resumes the chaos engine
  if (input->type == TYPE_BUTTON && input->id == controller->getButton(Button::SHARE)) {
    if(input->value == 1 && pause == true) { // on rising edge
      pausePrimer = true;
    } else if(input->value == 0 && pausePrimer == true) { // falling edge
      pause = false;
      chaosInterface.sendMessage("{\"pause\":0}");
      PLOG_INFO << "Resumed" << std::endl;
    }
    output->value = 0;
  }
	
  unlock();
	
  if (!pause) {
    lock();
    for (std::list<Modifier*>::iterator it = modifiers.begin(); it != modifiers.end(); it++) {
      valid &= (*it)->tweak(output);
      if (!valid) {
	break;
      }
    }
    unlock();
  }	
  return valid;
}

// This function is called by modifiers to inject a fake event into the event pipeline.
// Because the event can be modified by other mods, we find the location of the modifier
// and feed the fake event through the rest of the modifiers, in order.
void ChaosEngine::fakePipelinedEvent(DeviceEvent* fakeEvent, Modifier* modifierThatSentTheFakeEvent) {
  bool valid = true;
	
  if (!pause) {
    // Find the modifier that sent the fake event in the modifier list
    std::list<Modifier*>::iterator it = std::find(modifiers.begin(), modifiers.end(), modifierThatSentTheFakeEvent);
    // iterate from the next element till the end and apply any tweaks
    for ( it++; it != modifiers.end(); it++) {
      valid &= (*it)->tweak(fakeEvent);
      if (!valid) {
	break;
      }
    }
  }
  // unless canceled, send the event out
  if (valid) {
    controller->applyEvent(fakeEvent);
  }
}

void ChaosEngine::setInterfaceReply(const std::string& reply) {
  chaosInterface.sendMessage(reply);
}

void ChaosEngine::setTimePerModifier(double time) {
  timePerModifier = time;
}

bool ChaosEngine::isPaused() {
  return pause;
}
