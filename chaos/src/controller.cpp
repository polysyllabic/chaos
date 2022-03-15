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
#include <cstring>
#include <map>
#include <iomanip>
#include <plog/Log.h>

#include "controller.hpp"

using namespace Chaos;

Controller::Controller() {
  memset(controllerState, 0, sizeof(controllerState));
}

void Controller::doAction() {
  PLOG_VERBOSE << "Queue length = " << deviceEventQueue.size() << std::endl;
	
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

void Controller::initialize() {

  this->setEndpoint(0x84);	// Works for both dualshock4 and dualsense
  mRawGadgetPassthrough.addObserver(this);
	
  mRawGadgetPassthrough.initialize();
  mRawGadgetPassthrough.start();
	
  while (!mRawGadgetPassthrough.readyProductVendor()) {
    usleep(10000);
  }
	
  mControllerState = ControllerState::factory(mRawGadgetPassthrough.getVendor(), mRawGadgetPassthrough.getProduct());
  chaosHid = new ChaosUhid(mControllerState);
  chaosHid->start();
	
  if (mControllerState == NULL) {
    PLOG_ERROR << "ERROR!  Could not build a ControllerState for vendor=0x"
	       << std::setfill('0') << std::setw(4) << std::hex << mRawGadgetPassthrough.getVendor()
	       << " product=0x" << std::setfill('0') << std::setw(4) << std::hex << mRawGadgetPassthrough.getProduct() << std::endl;
    exit(EXIT_FAILURE);
  }
}

// Events are remapped before the mods see them, but this command looks at the current state of the
// controller, and since the input parameter is a game command, we need to check the remapped state
// (the signal we expect from the controller) and not the one the console expects.
short int Controller::getState(std::shared_ptr<GameCommand> command) {
  return getState(GamepadInput::get(command->getInput()->getRemap());
}

// Note: For the hybrid buttons L2 and R2, we handle them as ordinary buttons for the purpose of
// monitoring state. That is, we return the state of the button event, not the exis event.
short int Controller::getState(std::shared_ptr<GamepadInput> signal) {
  return getState(input->getID(), input->getButtonType());
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
  if (controllerInjector != NULL) {
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

bool Controller::matches(const DeviceEvent& event, GPInput signal) {
  std::shared_ptr<GamepadInput> sig = GamepadInput::get(signal);
  return (event.type == sig->getButtonType() && event.id == sig->getID());
}

bool Controller::matches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) {
  if (command->getCondition()->inCondition()) {
    return (event.type == command->getInput()->getButtonType() && event.id == command->getInput()->getID());
  }
  return false;
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
  case GPInputType::BUTTON:
    event = {0, 1, TYPE_BUTTON, command->getInput()->getID()};
    break;
  case GPInputType::HYBRID:
    event = {0, 1, TYPE_BUTTON, command->getInput()->getID()};
    storeState(event);
    event = {0, JOYSTICK_MAX, TYPE_AXIS, command->getInput()->getHybridAxis()};
    break;
  default:
    event = {0, JOYSTICK_MAX, TYPE_AXIS, command->getInput()->getID()};
  }
  storeState(event);
}
