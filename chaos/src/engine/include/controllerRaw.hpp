/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78
*
* TCC is free software: you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* TCC is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along
* with TCC.  If not, see <https://www.gnu.org/licenses/>.
*---------------------------------------------------------------------------*/
#ifndef CONTROLLER_RAW__HPP
#define CONTROLLER_RAW_HPP

#include <map>
#include <cstdio>
#include <deque>
#include <array>

#include "controller.hpp"
#include "device.h"	// Joystick, Mouse
#include "raw-gadget.hpp"
#include "controller-state.h"
#include "chaosUhid.h"

#define PWM_RANGE (11)

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
	
    void initialize( );	
  };

};

#endif
