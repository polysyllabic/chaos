/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#include <map>
#include <array>
#include <unistd.h>
#include <plog/Log.h>

#include "controllerRaw.hpp"

using namespace Chaos;

// State will already be stored, nothing to be done for raw
bool ControllerRaw::applyHardware(const DeviceEvent* event) {
  return true;
}

void ControllerRaw::doAction() {
  PLOG_DEBUG << "Queue length = " << deviceEventQueue.size() << std::endl;
	
  while (!bufferQueue.empty()) {
    lock();
    std::array<unsigned char, 64> bufferFront = bufferQueue.front();
    bufferQueue.pop_front();
    unlock();
		
    // First convert the incoming buffer into a device event:
    // A buffer can contain multiple events
    std::vector<DeviceEvent> deviceEvents;
    mControllerState->getDeviceEvents(bufferFront.data(), 64, deviceEvents);
		
    for (std::vector<DeviceEvent>::iterator it=deviceEvents.begin();
	 it != deviceEvents.end();
	 it++) {
      DeviceEvent& event = *it;
      handleNewDeviceEvent( &event );
    }
    pause();
  }
}

void ControllerRaw::notification(unsigned char* buffer, int length) {
	
  lock();
  bufferQueue.push_back( *(std::array<unsigned char, 64>*) buffer);
  unlock();
	
  resume();	// kick off the thread if paused
	
  // This is our only chance to intercept the data.
  // Take the mControllerState and replace the provided buffer:
  mControllerState->applyHackedState(buffer, controllerState);
}

void ControllerRaw::initialize( ) {

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
