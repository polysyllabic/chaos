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
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <unistd.h>
#include <plog/Log.h>

#include "ControllerRaw.hpp"
#include "DeviceEvent.hpp"

using namespace Chaos;

ControllerRaw::ControllerRaw() : Controller() {
  initialize();
}

ControllerRaw::~ControllerRaw() {
  mUsbPassthrough.stop();
  WaitForInternalThreadToExit();
  lock();
  mControllerState.reset();
  unlock();
}

void ControllerRaw::initialize() {
  PLOG_VERBOSE << "Initializing controller";
  mUsbPassthrough.setEndpoint(0x84);  // Works for both dualshock4 and dualsense

  // Register to be notified when the raw gadget has an event
  mUsbPassthrough.addObserver(this);
  if (mUsbPassthrough.initialize() != 0) {
    PLOG_ERROR << "UsbPassthrough initialization failed. Controller hot-plug will be unavailable.";
    return;
  }
  mUsbPassthrough.start();

  initializeControllerStateIfPossible();
}

void ControllerRaw::initializeControllerStateIfPossible() {
  if (!mUsbPassthrough.readyProductVendor()) {
    return;
  }

  const int vendor = mUsbPassthrough.getVendor();
  const int product = mUsbPassthrough.getProduct();
  lock();
  if (vendor == mLastFactoryVendor && product == mLastFactoryProduct) {
    unlock();
    return;
  }

  if (vendor != mLastFactoryVendor || product != mLastFactoryProduct) {
    if (mControllerState != nullptr) {
      PLOG_INFO << "Controller VID/PID changed from 0x"
                << std::setfill('0') << std::setw(4) << std::hex << mLastFactoryVendor
                << ":0x" << std::setw(4) << mLastFactoryProduct
                << " to 0x" << std::setw(4) << vendor
                << ":0x" << std::setw(4) << product << std::dec
                << ". Rebinding controller parser.";
    }
    mControllerState.reset();
    deviceEventQueue.clear();
  }

  mLastFactoryVendor = vendor;
  mLastFactoryProduct = product;
  mControllerState = std::shared_ptr<ControllerState>(ControllerState::factory(vendor, product));
  if (mControllerState == nullptr) {
    PLOG_ERROR << "Could not build ControllerState for vendor=0x"
               << std::setfill('0') << std::setw(4) << std::hex << vendor
               << " product=0x" << std::setw(4) << product << std::dec
               << ". Waiting for a supported controller to be plugged in.";
  }
  unlock();
}

/* applyHardware() was a function for the GPIO interface. Does nothing now, so removed. 
bool ControllerRaw::applyHardware(const DeviceEvent& event) {
	//	State will already be stored, nothing to be done for raw
	return true;
}
*/

void ControllerRaw::doAction() {
  while (true) {
    std::shared_ptr<ControllerState> controllerStateSnapshot;
    std::array<unsigned char, 64> bufferFront{};
    lock();
    if (deviceEventQueue.empty()) {
      unlock();
      break;
    }
    controllerStateSnapshot = mControllerState;
    bufferFront = deviceEventQueue.front();
    deviceEventQueue.pop_front();
    unlock();
    
    if (controllerStateSnapshot == nullptr) {
      continue;
    }

    // Convert the incoming buffer into a series of device events.
    std::vector<DeviceEvent> deviceEvents;
    controllerStateSnapshot->getDeviceEvents(bufferFront.data(), 64, deviceEvents);
		
    for (std::vector<DeviceEvent>::iterator it=deviceEvents.begin(); it != deviceEvents.end(); it++) {
      DeviceEvent& event = *it;
      handleNewDeviceEvent(event);
    }
  }
  pause();
}

void ControllerRaw::notification(unsigned char* buffer, int length) {
  initializeControllerStateIfPossible();
  std::shared_ptr<ControllerState> controllerStateSnapshot;
  lock();
  controllerStateSnapshot = mControllerState;
  unlock();
  if (controllerStateSnapshot == nullptr) {
    PLOG_VERBOSE << "Dropping controller report because controller state is not initialized.";
    return;
  }
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
  controllerStateSnapshot->applyHackedState(buffer, controllerState);
}
