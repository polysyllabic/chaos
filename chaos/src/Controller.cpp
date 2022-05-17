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
#include <cstring>
#include <map>
#include <iomanip>
#include <plog/Log.h>

#include "Controller.hpp"
#include "ControllerInput.hpp"
#include "GameCommand.hpp"
#include "GameCondition.hpp"

using namespace Chaos;

Controller::Controller() {
  memset(controllerState, 0, sizeof(controllerState));
  PLOG_DEBUG << "Initializing controller";
  this->setEndpoint(0x84);	// Works for both dualshock4 and dualsense
  mRawGadgetPassthrough.addObserver(this);
	
  mRawGadgetPassthrough.initialize();
  mRawGadgetPassthrough.start();
	
  while (!mRawGadgetPassthrough.readyProductVendor()) {
    usleep(10000);
  }
	
  mControllerState = ControllerState::factory(mRawGadgetPassthrough.getVendor(), mRawGadgetPassthrough.getProduct());
  //chaosHid = new ChaosUhid(mControllerState);
  //chaosHid->start();
	
  if (mControllerState == nullptr) {
    PLOG_ERROR << "ERROR!  Could not build a ControllerState for vendor=0x"
	       << std::setfill('0') << std::setw(4) << std::hex << mRawGadgetPassthrough.getVendor()
	       << " product=0x" << std::setfill('0') << std::setw(4) << std::hex << mRawGadgetPassthrough.getProduct();
    exit(EXIT_FAILURE);
  }
}

void Controller::doAction() {
  if (deviceEventQueue.size() > 0) {
    PLOG_VERBOSE << "Queue length = " << deviceEventQueue.size();
  }
  while (!bufferQueue.empty()) {
    lock();
    std::array<unsigned char, 64> bufferFront = bufferQueue.front();
    bufferQueue.pop_front();
    unlock();
		
    // First convert the incoming buffer into a device event. A buffer can contain multiple events.
    std::vector<DeviceEvent> deviceEvents;
    mControllerState->getDeviceEvents(bufferFront.data(), 64, deviceEvents);
		
    for (std::vector<DeviceEvent>::iterator it=deviceEvents.begin();
	 it != deviceEvents.end();
	 it++) {
      DeviceEvent& event = *it;
      handleNewDeviceEvent(event);
    }
    pause();
  }
}

void Controller::notification(unsigned char* buffer, int length) {
	
  lock();
  bufferQueue.push_back( *(std::array<unsigned char, 64>*) buffer);
  unlock();
	
  resume();	// kick off the thread if paused
	
  // This is our only chance to intercept the data.
  // Take the mControllerState and replace the provided buffer:
  mControllerState->applyHackedState(buffer, controllerState);
}

short Controller::getState(std::shared_ptr<ControllerInput> signal) {
  return getState(signal->getID(), signal->getButtonType());
}

void Controller::storeState(const DeviceEvent& event) {
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
    storeState(updatedEvent);
  } else {
    PLOG_VERBOSE << "Event with id " << event.id << " was NOT applied\n";
  }	
}

void Controller::addInjector(ControllerInjector* injector) {
  this->controllerInjector = injector;
}

/*
bool Controller::matches(const DeviceEvent& event, ControllerSignal signal) {
  std::shared_ptr<ControllerInput> sig = ControllerInput::get(signal);
  return (event.type == sig->getButtonType() && event.id == sig->getID());
} */

bool Controller::matches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) {
  return (event.type == command->getInput()->getButtonType() && event.id == command->getInput()->getID());
}

// Send a new event to turn off the command.
void Controller::setOff(std::shared_ptr<GameCommand> command) {
  DeviceEvent event;
  switch (command->getInput()->getType()) {
  case ControllerSignalType::BUTTON:
    event = {0, 0, TYPE_BUTTON, command->getInput()->getID()};
    break;
  case ControllerSignalType::HYBRID:
    event = {0, 0, TYPE_BUTTON, command->getInput()->getID()};
    storeState(event);
    event = {0, JOYSTICK_MIN, TYPE_AXIS, command->getInput()->getHybridAxis()};
    break;
  default:
    event = {0, 0, TYPE_AXIS, command->getInput()->getID()};
  }
  storeState(event);
}

// Send a new event to turn the command to its maximum value.
void Controller::setOn(std::shared_ptr<GameCommand> command) {
  DeviceEvent event;
  switch (command->getInput()->getType()) {
  case ControllerSignalType::BUTTON:
    event = {0, 1, TYPE_BUTTON, command->getInput()->getID()};
    break;
  case ControllerSignalType::HYBRID:
    event = {0, 1, TYPE_BUTTON, command->getInput()->getID()};
    storeState(event);
    event = {0, JOYSTICK_MAX, TYPE_AXIS, command->getInput()->getHybridAxis()};
    break;
  default:
    event = {0, JOYSTICK_MAX, TYPE_AXIS, command->getInput()->getID()};
  }
  storeState(event);
}
