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
#include <cstdio>
#include <cstring>
#include <map>
#include <plog/Log.h>

#include "controller.hpp"

using namespace Chaos;

Controller::Controller() {
  memset(controllerState, 0, sizeof(controllerState));
}

// for hybrid controlls, we only check the button
int Controller::getState(GPInput command) {
  std::shared_ptr<GamepadInput> input = GamepadInput::get(command);
  return getState(input->getID(), input->getButtonType());
}

void Controller::storeState(const DeviceEvent& event) {
  int location = ((int) event.type << 8) + (int) event.id;
  if (location < 1024) {
    controllerState[location] = event.value;
  }
}

void Controller::handleNewDeviceEvent(const DeviceEvent& event) {
	
  // We have an event!  BEfore sending it, allow the mods to play around with the values
  DeviceEvent updatedEvent;
  bool validEvent = false;
  if (controllerInjector != NULL) {
    validEvent = controllerInjector->sniffify(event, updatedEvent);
  } else {
    validEvent = true;
    updatedEvent = event;
  }
	
  // Is our event valid?  If so, send the chaos modified data:
  if (validEvent) {
    applyEvent(updatedEvent);
  } else {
    PLOG_VERBOSE << "Event with id " << event.id << " was NOT applied\n";
  }	
}

void Controller::applyEvent(const DeviceEvent& event) {
  if (!applyHardware(event)) {
    return;
  }
	
  storeState(event);
}

void Controller::addInjector(ControllerInjector* injector) {
  this->controllerInjector = injector;
}

bool Controller::matches(const DeviceEvent& event, GPInput signal) {
  std::shared_ptr<GamepadInput> sig = GamepadInput::get(signal);
  return (event.type == sig->getButtonType() && event.id == sig->getID());
}

bool Controller::matches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) {
  if (command->getCondition() != GPInput::NONE) {
    if (isOn(command->getCondition()) == command->conditionInverted()) {
      return false;
    }
  }
  return (event.type == command->getInput()->getButtonType() && event.id == command->getInput()->getID());
}

// Send a new event to turn off the command.
void Controller::setOff(std::shared_ptr<GameCommand> command) {
  DeviceEvent event;
  switch (command->getInput()->getType()) {
  case GPInputType::BUTTON:
    event = {0, 0, TYPE_BUTTON, command->getInput()->getID()};
    break;
  case GPInputType::HYBRID:
    event = {0, 0, TYPE_BUTTON, command->getInput()->getID()};
    applyEvent(event);
    event = {0, getJoystickMin(), TYPE_AXIS, command->getInput()->getHybridAxis()};
    break;
  default:
    event = {0, 0, TYPE_AXIS, command->getInput()->getID()};
  }
  applyEvent(event);
}

