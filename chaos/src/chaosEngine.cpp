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

ChaosEngine::ChaosEngine(Controller& c, const std::string& configfile) : controller{c}, pause(true)
{
  controller.addInjector(this);
  time.initialize();
  chaosInterface.addObserver(this);
  jsonReader = jsonReaderBuilder.newCharReader();
  game.loadConfigFile(configfile);
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
    PLOG_DEBUG << "Sending game information to interface: " << game.getName() << "(parsed with " << game.getErrors() << "errors)";
    Json::Value msg;
    msg["game"] = game.getName();
    msg["errors"] = game.getErrors();
    msg["nmods"] = game.getNumActiveMods();
    msg["modtime"] = game.getTimePerModifier();
    chaosInterface.sendMessage(Json::writeString(jsonWriterBuilder, msg));
  }

  if (root.isMember("modlist")) {
    // Announce the mods to the chaos interface
    std::string reply = game.getModList();
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
	      (front->lifespan()  < 0 && front->lifetime() > game.getTimePerModifier())) {
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
  if (game.matchesID(input, ControllerSignal::OPTIONS) || game.matchesID(input, ControllerSignal::PS)) {
    if(input.value == 1 && pause == false) { // on rising edge
      pause = true;
      chaosInterface.sendMessage("{\"pause\":1}");
      pausePrimer = false;
      PLOG_INFO << "Game Paused";
    }
  }

  // The share button resumes the chaos engine (also not remapped)
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
    // Translate incomming signal to what the console expects before passing that on to the mods
    valid = remapEvent(output);
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
void ChaosEngine::fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod) {
  bool valid = true;
	
  if (!pause) {
    // Find the modifier that sent the fake event in the modifier list
    auto mod = std::find_if(modifiers.begin(), modifiers.end(),
                            [&sourceMod](const auto& p) { return p == sourceMod; } );
    
    // iterate from the next element till the end and apply any tweaks
    for ( mod++; mod != modifiers.end(); mod++) {
      valid = (*mod)->tweak(event);
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

bool ChaosEngine::remapEvent(DeviceEvent& event) {
  DeviceEvent new_event;

  // Get the object with information about the incomming signal
  std::shared_ptr<ControllerInput> from_controller = game.getSignalTable().getInput(event);
  assert (from_controller);
  std::shared_ptr<ControllerInput> to_console = from_controller->getRemap();

  // If this signal isn't remapped, we're done
  if (to_console == nullptr) {
    return true;
  }

  // Touchpad active requires special setup and tear-down
  if (from_controller->getSignal() == ControllerSignal::TOUCHPAD_ACTIVE) {
    prepTouchpad(event);
  }

  // When mapping from any type of axis to buttons/hybrids and the choice of button is determined
  // by the sign of the value. If the negative remap is a button, the positive will be a button also,
  // so it's safe to check that.
  if (event.value < 0 && event.id == TYPE_AXIS && to_console->getButtonType() == TYPE_BUTTON) {
    to_console = from_controller->getNegRemap();
    assert (to_console);
  }

  // If we are remapping to NOTHING, we drop this signal.
  if (to_console->getSignal() == ControllerSignal::NOTHING) {
    PLOG_DEBUG << "dropping remapped event";
    return false;
  }

  // Now handle cases that require additional actions
  switch (from_controller->getType()) {
  case ControllerSignalType::HYBRID:
    // Going from hybrid to button, we drop the axis component
    if (to_console->getType() == ControllerSignalType::BUTTON && event.type == TYPE_AXIS) {
 	    return false;
    }
  case ControllerSignalType::BUTTON:
    // From button to hybrid, we need to insert a new event for the axis
    if (to_console->getType() == ControllerSignalType::HYBRID) {
   	  new_event.id = to_console->getHybridAxis();
      new_event.type = TYPE_AXIS;
   	  new_event.value = event.value ? JOYSTICK_MAX : JOYSTICK_MIN;
      controller.applyEvent(new_event);
    }
    break;
  case ControllerSignalType::AXIS:
    // From axis to button, we need a new event to zero out the second button when the value is
    // below the threshold. The first button will get a 0 value from the changed regular value
    // At this point, to_console has been set with the negative remap value.
    if (to_console->getButtonType() == TYPE_BUTTON && abs(event.value) < from_controller->getThreshold()) {
      new_event.id = to_console->getID();
      new_event.value = 0;
      new_event.type = TYPE_BUTTON;
      controller.applyEvent(new_event);
    }
  }

  // Touchpad-to-axis conversion has to be handled in a different conversion routine
  if (from_controller->getType() == ControllerSignalType::TOUCHPAD && 
      to_console->getType() == ControllerSignalType::AXIS) {
    event.value = game.getSignalTable().touchpadToAxis(from_controller->getSignal(), event.value);
  }
  // Update the event
  event.type = to_console->getButtonType();
  event.id = to_console->getID(event.type);
  event.value = from_controller->remapValue(to_console, event.value);
  return true;
}

void ChaosEngine::prepTouchpad(const DeviceEvent& event) {
  Touchpad& touchpad = game.getSignalTable().getTouchpad();
  if (! touchpad.isActive() && event.value == 0) {
    touchpad.clearActive();
    PLOG_DEBUG << "Begin touchpad use";
  } else if (touchpad.isActive() && event.value) {
    PLOG_DEBUG << "End touchpad use";
    // We're stopping touchpad use. Zero out all remapped axes in use.
    disableTPAxis(ControllerSignal::TOUCHPAD_X);
    disableTPAxis(ControllerSignal::TOUCHPAD_Y);
    disableTPAxis(ControllerSignal::TOUCHPAD_X_2);
    disableTPAxis(ControllerSignal::TOUCHPAD_Y_2);
  }
  touchpad.setActive(!event.value);
}

// If the touchpad axis is currently being remapped, send a 0 signal to the remapped axis
void ChaosEngine::disableTPAxis(ControllerSignal tp_axis) {
  DeviceEvent new_event{0, 0, TYPE_AXIS, AXIS_RX};
  std::shared_ptr<ControllerInput> tp = game.getSignalTable().getInput(tp_axis);
  assert (tp && tp->getType() == ControllerSignalType::TOUCHPAD);

  if (tp->getRemap()) {
    std::shared_ptr<ControllerInput> r = tp->getRemap();
    assert(r);
    new_event.id = r->getID();
    controller.applyEvent(new_event);
  }
}
