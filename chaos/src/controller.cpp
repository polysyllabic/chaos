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

int Controller::getState(GPInput command) {
  return getState(getGPInfo(command).getID(), getGPInfo(command).getButtonType());
}

void Controller::storeState(const DeviceEvent* event) {
  int location = ((int)event->type<<8) + (int)event->id;
  if (location < 1024) {
    controllerState[ location ] = event->value;
  }
}

void Controller::handleNewDeviceEvent(const DeviceEvent* event) {
	
  // We have an event!  Let's send it right away, but let's let chaos engine play around with the values
  DeviceEvent updatedEvent;
  bool validEvent = false;
  if (controllerInjector != NULL) {
    validEvent = controllerInjector->sniffify(event, &updatedEvent);
  } else {
    validEvent = true;
    updatedEvent = *event;
  }
	
  // Is our event valid?  If so, send the chaos modified data:
  if (validEvent) {
    applyEvent(&updatedEvent);
  } else {
    PLOG_VERBOSE << "Event with id " << event->id << " was NOT applied\n";
  }	
}

void Controller::applyEvent(const DeviceEvent* event) {
  if (!applyHardware(event)) {
    return;
  }
	
  storeState(event);
}

void Controller::addInjector(ControllerInjector* injector) {
  this->controllerInjector = injector;
}

bool Controller::isState(GPInput command, int state) {
  return (getState(command) == state);
}

void Controller::setOff(GPInput command) {
  DeviceEvent event;
  switch (getGPInfo(command).getType()) {
  case GPInputType::BUTTON:
    event = {0, 0, TYPE_BUTTON, getGPInfo(command).getID()};
    break;
  case GPInputType::HYBRID:
    event = {0, 0, TYPE_BUTTON, getGPInfo(command).getID()};
    applyEvent(&event);
    event = {0, getJoystickMin(), TYPE_AXIS, getGPInfo(command).getSecondID()};
    break;
  default:
    event = {0, 0, TYPE_AXIS, getGPInfo(command).getID()};
  }
  applyEvent(&event);
}

bool Controller::matches(const DeviceEvent* event, GPInput command) {
  
  if (getGPInfo(command).getType() == GPInputType::HYBRID) {
    return ((event->id == getGPInfo(command).getID() && event->type == TYPE_BUTTON) ||
	    (event->id == getGPInfo(command).getSecondID() && event->type == TYPE_AXIS));
    
  }
  return (event->id == getGPInfo(command).getID() && event->type == getGPInfo(command).getButtonType());
}

void Controller::setState(DeviceEvent* event, int new_value, GPInput remapped, GPInput real) {
  if (remapped == real) {
    // No remapping has occurred
    event->value = new_value;
  }
  else if (getGPInfo(remapped).getType() == getGPInfo(real).getType()) {
    // The swap is among controller commands of the same type
    event->value = new_value;
    if (getGPInfo(real).getType() == GPInputType::HYBRID && event->type == TYPE_AXIS) {
      event->id = getGPInfo(real).getSecondID();
    } else {
      event->id = getGPInfo(real).getID();
    }
  } else {
    // The swap is between controller commands of different types
  }
    
}
