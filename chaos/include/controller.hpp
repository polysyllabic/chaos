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
#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <deque>
#include <array>

//#include "device.hpp"	// Joystick, Mouse
#include "raw-gadget.hpp"
#include "controllerState.hpp"
#include "chaosUhid.hpp"
#include "deviceTypes.hpp"

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

    virtual uint8_t getButton(Button b) = 0;
    virtual uint8_t getAxis(Axis a) = 0;
    virtual int getJoystickMin() = 0;
    virtual int getJoystickMax() = 0;
  };

};

#endif
