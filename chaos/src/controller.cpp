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

#include "controller.hpp"

using namespace Chaos;

Controller::Controller() {
  memset(controllerState, 0, sizeof(controllerState));
}

int Controller::getState(int id, ButtonType type) {
  return controllerState[ ((int)type<<8) + (int)id ];
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
    //printf("Event with id %d was NOT applied\n", event->id);
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
