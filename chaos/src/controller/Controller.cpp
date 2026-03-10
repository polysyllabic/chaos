/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributors.
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
#include <cstring>
#include <map>
#include <iomanip>
#include <plog/Log.h>

#include "Controller.hpp"
#include "ControllerInput.hpp"

using namespace Chaos;

Controller::Controller() {
  std::lock_guard<std::mutex> lock(stateMutex);
  memset(controllerState, 0, sizeof(controllerState));
  // Hybrid trigger axes are centered at JOYSTICK_MIN when released.
  controllerState[((int) TYPE_AXIS << 8) + (int) AXIS_L2] = JOYSTICK_MIN;
  controllerState[((int) TYPE_AXIS << 8) + (int) AXIS_R2] = JOYSTICK_MIN;
}

short Controller::getState(std::shared_ptr<ControllerInput> signal) {
  return getState(signal->getID(), signal->getButtonType());
}

void Controller::storeState(const DeviceEvent& event) {
  std::lock_guard<std::mutex> lock(stateMutex);
  int location = ((int) event.type << 8) + (int) event.id;
  if (location < 1024) {
    controllerState[location] = event.value;
  }
}

void Controller::handleNewDeviceEvent(const DeviceEvent& event) {
	
  // We have an event! Before sending it, allow the mods to play around with the values.
  DeviceEvent updatedEvent;
  bool validEvent = false;
  if (controllerInjector != nullptr) {
    validEvent = controllerInjector->sniffify(event, updatedEvent);
  } else {
    validEvent = true;
    updatedEvent = event;
  }
	
  // Is our event valid?  If so, send the chaos modified data:
  if (validEvent) {
    if (!dispatchEvent(updatedEvent)) {
      PLOG_VERBOSE << "Event with id " << event.id << " was suppressed by injector policy\n";
    }
  } else {
    PLOG_VERBOSE << "Event with id " << event.id << " was NOT applied\n";
  }	
}

bool Controller::dispatchEvent(const DeviceEvent& event, bool allow_during_menu) {
  if (controllerInjector != nullptr) {
    return controllerInjector->dispatchControllerEvent(
        event,
        [this](const DeviceEvent& emitted) { applyEvent(emitted); },
        allow_during_menu);
  }
  applyEvent(event);
  return true;
}

void Controller::addInjector(ControllerInjector* injector) {
  this->controllerInjector = injector;
}

bool Controller::matches(const DeviceEvent& event, std::shared_ptr<ControllerInput> signal) {
  return (event.type == signal->getButtonType() && event.id == signal->getID());
}

void Controller::setValue(std::shared_ptr<ControllerInput> signal, short value) {
  // to do: screen to max for signal type
  value = ControllerInput::joystickLimit(value);
  DeviceEvent event = {0, value, (uint8_t) signal->getButtonType(), signal->getID()};
  if (signal->getType() == ControllerSignalType::HYBRID) {
    dispatchEvent(event);
    event = {0, (short)((value) ? JOYSTICK_MAX : JOYSTICK_MIN), TYPE_AXIS, signal->getHybridAxis()};
  }
  PLOG_DEBUG << "Setting " << signal->getName() << " to " << value;
  dispatchEvent(event);
}

// Send a new event to turn off the command.
void Controller::setOff(std::shared_ptr<ControllerInput> signal) {
  setValue(signal, 0);
}

// Send a new event to turn the command to its maximum value.
void Controller::setOn(std::shared_ptr<ControllerInput> signal) {
  DeviceEvent event;
  PLOG_DEBUG << "Turning " << signal->getName() << " on";
  switch (signal->getType()) {
  case ControllerSignalType::BUTTON:
    event = {0, 1, TYPE_BUTTON, signal->getID()};
    break;
  case ControllerSignalType::HYBRID:
    event = {0, 1, TYPE_BUTTON, signal->getID()};
    dispatchEvent(event);
    event = {0, JOYSTICK_MAX, TYPE_AXIS, signal->getHybridAxis()};
    break;
  default:
    event = {0, JOYSTICK_MAX, TYPE_AXIS, signal->getID()};
  }
  dispatchEvent(event);
}

void Controller::applyEvent(const DeviceEvent& event) {
	//if (!applyHardware(event)) {
	//	return;
	//}
	storeState(event);
}
