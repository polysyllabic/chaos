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
#include <memory>

#include "raw-gadget.hpp"
#include "controllerState.hpp"
#include "chaosUhid.hpp"
#include "deviceTypes.hpp"
#include "gamepadInput.hpp"

namespace Chaos {

  class ControllerInjector  {
  public:
    /**
     * Sniff the input and pass/modify it on way to the output.
     */
    virtual bool sniffify(const DeviceEvent& input, DeviceEvent& output) = 0;
  };

  class Controller {
  protected:
    ControllerState* mControllerState;

    std::deque<DeviceEvent> deviceEventQueue;
	
    virtual bool applyHardware(const DeviceEvent& event) = 0;
	
    void storeState(const DeviceEvent& event);
	
    void handleNewDeviceEvent(const DeviceEvent& event);
	
    /**
     * Database for tracking current button states:
     * Input: ((int)event->type<<8) + (int)event->id ] = event->value;
     *  Output: state of that type/id combo
     */
    short controllerState[1024];
	
    ControllerInjector* controllerInjector = NULL;

  public:
    Controller();

    /**
     * \brief The low-level function to get the current controlller state.
     * \param id The id of the axis/button
     * \param type The signal type
     * \return The current value of the signal.
     *
     * Directly reads the buffer holding the current controller state.
     *
     */
    inline int getState(uint8_t id, uint8_t type) {
      return controllerState[((int) type << 8) + (int) id];
    }
    /**
     * \brief Get the current controlller state of a particular gampepad input.
     * \param command  The signal to check
     *
     * Directly read the buffer holding the current controller state. Note that for the
     * hybrid controls, we deliberately only read the button form, since currently we're
     * only interested in whether it is on or off. This may need to change for other
     * games.
     *
     */
    int getState(GPInput command);

    /**
     * \brief Test if event matches a specific input signal
     * 
     * This tests the signal only, regarless of any condition associated with a game command.
     */
    bool matches(const DeviceEvent& event, GPInput signal);
    /**
     * \brief Is the event an instance of the specified input command?
     * 
     * This tests both that the event against the defined signal and that any defined condition
     * is also in effect.
     */
    bool matches(const DeviceEvent& event, std::shared_ptr<GameCommand> command);
    /**
     * Tests if the button/axis is in on or off
     */
    bool isOn(GPInput command) { return getState(command) != 0; }
    /**
     * Turns off the button/axis associated with a command.
     * \param[in] The command that we're disabling.
     */
    void setOff(std::shared_ptr<GameCommand> command);
    
    void applyEvent(const DeviceEvent& event);
    void addInjector(ControllerInjector* injector);

    virtual short int getJoystickMin() = 0;
    virtual short int getJoystickMax() = 0;

    inline short joystickLimit(int input) {
      return fmin(fmax(input, getJoystickMin()), getJoystickMax());
    }
    
  };

};

