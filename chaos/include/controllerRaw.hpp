/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
#ifndef CONTROLLER_RAW_HPP
#define CONTROLLER_RAW_HPP

#include <cstdio>
#include <deque>
#include <array>

#include "raw-gadget.hpp"

#include "controller.hpp"
//#include "device.hpp"	// Joystick, Mouse
#include "controllerState.hpp"
#include "chaosUhid.hpp"

namespace Chaos {

  class ControllerRaw : public Controller, public EndpointObserver, public Mogi::Thread {
  private:
    ControllerState* mControllerState;
    std::deque<std::array<unsigned char,64>> bufferQueue;
	
    ChaosUhid* chaosHid;
	
    bool applyHardware(const DeviceEvent* event);
	
    // Main purpose of this Mogi::Thread is to handle DeviceEvent queue
    void doAction();
	
    void notification(unsigned char* buffer, int length); // overloaded from EndpointObserver

    //FILE* spooferFile = NULL;
  public:
    RawGadgetPassthrough mRawGadgetPassthrough;
	
    void initialize();
    
    uint8_t getButton(Button b);
    uint8_t getAxis(Axis a);
    int getJoystickMin();
    int getJoystickMax();
  };

};

#endif
