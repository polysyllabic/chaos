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
#include <iomanip>
#include <cstring>
#include <plog/Log.h>

#include "ControllerRaw.hpp"
#include "DeviceEvent.hpp"

using namespace Chaos;

ControllerRaw::ControllerRaw() : Controller() {
  initialize();
}

void ControllerRaw::initialize() {
  PLOG_VERBOSE << "Initializing controller";
  this->setEndpoint(0x84);	// Works for both dualshock4 and dualsense

  // Register to be notified when the raw gadget has an event
  mRawGadgetPassthrough.addObserver(this);	
  mRawGadgetPassthrough.initialize();
  mRawGadgetPassthrough.start();
	
  // Wait for vendor and product ID
  while (!mRawGadgetPassthrough.readyProductVendor()) {
    usleep(10000);
  }
	
  mControllerState = ControllerState::factory(mRawGadgetPassthrough.getVendor(), mRawGadgetPassthrough.getProduct());
  //chaosHid = new ChaosUhid(mControllerState);
  //chaosHid->start();
	
  if (mControllerState == nullptr) {
    PLOG_ERROR << "ERROR!  Could not build a ControllerState for vendor=0x"
	       << std::setfill('0') << std::setw(4) << std::hex << mRawGadgetPassthrough.getVendor()
	       << " product=0x" << mRawGadgetPassthrough.getProduct() << std::dec;
    exit(EXIT_FAILURE);
  }
}

/* applyHardware() was a function for the GPIO interface. Does nothing now, so removed. 
bool ControllerRaw::applyHardware(const DeviceEvent& event) {
	//	State will already be stored, nothing to be done for raw
	return true;
}
*/

void ControllerRaw::doAction() {
  while (true) {
    lock();
    if (deviceEventQueue.empty()) {
      unlock();
      break;
    }
    std::array<unsigned char, 64> bufferFront = deviceEventQueue.front();
    deviceEventQueue.pop_front();
    unlock();
			
    // Convert the incoming buffer into a series of device events.
    std::vector<DeviceEvent> deviceEvents;
    mControllerState->getDeviceEvents(bufferFront.data(), 64, deviceEvents);
		
    for (std::vector<DeviceEvent>::iterator it=deviceEvents.begin(); it != deviceEvents.end(); it++) {
      DeviceEvent& event = *it;
      handleNewDeviceEvent(event);
    }
  }
  pause();
}

void ControllerRaw::notification(unsigned char* buffer, int length) {
  if (length < 64) {
    PLOG_WARNING << "Dropping short controller report: expected 64 bytes, got " << length;
    return;
  }
		
  std::array<unsigned char, 64> report{};
  std::memcpy(report.data(), buffer, report.size());
  lock();
  deviceEventQueue.push_back(report);
  unlock();
	
  resume();	// kick off the thread if paused
	
  // This is our only chance to intercept the data.
  // Take the mControllerState and replace the provided buffer:
  mControllerState->applyHackedState(buffer, controllerState);
}
