/**
 * @file controller.hpp
 *
 * @brief Definition of the Controller class
 *
 * @author blegas78
 * @author polysyl
 *
 */
/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78. Additional code copyright 2021 polysyl.
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
#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

//#include <map>
#include <cstdio>
#include <deque>
#include <array>

#include "device.hpp"	// Joystick, Mouse
#include "raw-gadget.hpp"
#include "controllerState.hpp"
#include "chaosUhid.hpp"

namespace Chaos {

  class ControllerInjector  {
  public:
    /**
     * sniff the input and pass/modify it on way to the output.
     */
    virtual bool sniffify(const DeviceEvent* input, DeviceEvent* output) = 0;
  };

  class Controller {
  protected:
    std::deque<DeviceEvent> deviceEventQueue;
	
    virtual bool applyHardware(const DeviceEvent* event) = 0;
	
    void storeState(const DeviceEvent* event);
	
    void handleNewDeviceEvent(const DeviceEvent* event);
	
    // Database for tracking current button states:
    // Input: ((int)event->type<<8) + (int)event->id ] = event->value;
    // Output: state of that type/id combo
    short controllerState[1024];
	
    ControllerInjector* controllerInjector = NULL;
	
  public:
    Controller();
    int getState(int id, ButtonType type);
	
    void applyEvent(const DeviceEvent* event);
    void addInjector(ControllerInjector* injector);

    const short int JOYSTICK_MIN = -128;
    const short int JOYSTICK_MAX = 128;
  };

};

#endif
