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
#pragma once

#include <deque>
#include <array>

#include "raw-gadget.hpp"
#include "controllerState.hpp"
#include "chaosUhid.hpp"
#include "deviceTypes.hpp"
#include "controllerCommand.hpp"

namespace Chaos {

  class ControllerInjector  {
  public:
    /**
     * Sniff the input and pass/modify it on way to the output.
     */
    virtual bool sniffify(const DeviceEvent* input, DeviceEvent* output) = 0;
  };

  class Controller {
  protected:
    std::deque<DeviceEvent> deviceEventQueue;
	
    virtual bool applyHardware(const DeviceEvent* event) = 0;
	
    void storeState(const DeviceEvent* event);
	
    void handleNewDeviceEvent(const DeviceEvent* event);
	
    /**
     * Database for tracking current button states:
     * Input: ((int)event->type<<8) + (int)event->id ] = event->value;
     *  Output: state of that type/id combo
     */
    short controllerState[1024];
	
    ControllerInjector* controllerInjector = NULL;

    /**
     * Holds the data for the button/axis commands.
     */
    std::vector<ControllerCommand> buttonInfo;

  public:
    Controller();
    /**
     * The low-level function to get the current controlller state.
     */
    inline int getState(int id, ButtonType type) { return controllerState[((int) type<<8) + (int)id ]; }
    /**
     * Get the state of the button/axis. For the hybrid type, the button state is returned
     */
    int getState(GPInput command);
    /**
     */
    bool matches(const DeviceEvent& event, GPInput command);
    /**
     * Tests if the button/axis is in a particular state
     */
    bool isState(GPInput command, int state);
    /**
     * Turns off the button/axis
     */
    void setOff(GPInput command);
    
    void applyEvent(const DeviceEvent* event);
    void addInjector(ControllerInjector* injector);

    virtual int getJoystickMin() = 0;
    virtual int getJoystickMax() = 0;
  };

};

