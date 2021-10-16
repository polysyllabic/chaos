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

  // These are the values for the DualShock2, the only controller currently supported. They're
  // defined in a class with ID values that can be changed in case we are able to add additional
  // controller types some day. NB: the order of insertion must match the enumeration in GPInput,
  // since we use that as an index for this vector.
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 1)); // GPInput::X
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 2)); // GPInput::TRIANGLE
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 3)); // GPInput::SQUARE
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 4)); // GPInput::L1
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 5)); // GPInput::R1
  buttonInfo.push_back(ControllerCommand(GPInputType::HYBRID, 6, 2)); // GPInput::L2
  buttonInfo.push_back(ControllerCommand(GPInputType::HYBRID, 7, 5)); // GPInput::R2
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 8)); // GPInput::SHARE
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 9)); // GPInput::OPTIONS
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 10)); // GPInput::PS
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 11)); // GPInput::L3
  buttonInfo.push_back(ControllerCommand(GPInputType::BUTTON, 12)); // GPInput::R3
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 0)); // GPInput::LX
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 1)); // GPInput::LY
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 3)); // GPInput::RX
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 4)); // GPInput::RY
  buttonInfo.push_back(ControllerCommand(GPInputType::THREE_STATE, 6)); // GPInput::DX
  buttonInfo.push_back(ControllerCommand(GPInputType::THREE_STATE, 7)); // GPInput::DY
  buttonInfo.push_back(ControllerCommand(GPInputType::ACCELEROMETER, 8)); // GPInput::ACCX
  buttonInfo.push_back(ControllerCommand(GPInputType::ACCELEROMETER, 9)); // GPInput::ACCY
  buttonInfo.push_back(ControllerCommand(GPInputType::ACCELEROMETER, 10)); // GPInput::ACCZ
  buttonInfo.push_back(ControllerCommand(GPInputType::GYROSCOPE, 11)); // GPInput::GYRX
  buttonInfo.push_back(ControllerCommand(GPInputType::GYROSCOPE, 12)); // GPInput::GYRY
  buttonInfo.push_back(ControllerCommand(GPInputType::GYROSCOPE, 13)); // GPInput::GYRZ
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 14)); // GPInput::TOUCHPAD_X
  buttonInfo.push_back(ControllerCommand(GPInputType::AXIS, 15)); // GPInput::TOUCHPAD_Y    
}

int Controller::getState(GPInput command) {
  return getState(command.getID(), command.getButtonType());
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
    PLOG_VERBOSE << "Event with id " << event-id << " was NOT applied\n";
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
  reutrn (getState(command) == state);
}

void Controller::setOff(GPInput command) {
  DeviceEvent event;
  switch (buttonInfo.getType(command)) {
  case GPInputType::BUTTON:
    event = {0, 0, TYPE_BUTTON, command->getID()};
    break;
  case GPInputType::HYBRID:
    event = {0, 0, TYPE_BUTTON, command->getID()};
    applyEvent(&event);
    event = {0, getJoystickMin(), TYPE_AXIS, command->getSecondID()};
    break;
  default:
    event = {0, 0, TYPE_AXIS, command->getID()};
  }
  applyEvent(&event);
}

bool Controller::matches(const DeviceEvent& event, GPInput command) {
  
  if (buttonInfo.getType(command) == GPInputType::HYBRID) {
    return ((event->id == buttonInfo.getID(command) && event->type == TYPE_BUTTON) ||
	    (event->id == buttonInfo.getSecondID(command) && event->type == TYPE_AXIS));
    
  }
  return (event->id == buttonInfo.getID(command) && event->type == buttonInfo.getButtonType(command));
}

/**
 * Handle remapping of the controller command. If the commands are remapped across button types, we
 * need some extra logic to handle the differences in signals.
 */
void Controller::setState(DeviceEvent* event, int new_value, GPInput remapped, GPInput real) {
  if (remapped == real) {
    // No remapping has occurred
    event->value = new_value;
  }
  else if (remapped.getType() == real.getType()) {
    // The swap is among controller commands of the same type
    event->value = new_value;
    if (real.getType() == GPInputType::HYBRID && event->type == TYPE_AXIS) {
      event->id = real.getSecondID();
    } else {
      event->id = real.getID();
    }
  } else {
    // The swap is between controller commands of different types
  }
    
}
