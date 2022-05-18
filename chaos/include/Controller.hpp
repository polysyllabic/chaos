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
#pragma once
#include <deque>
#include <memory>

#include <mogi/thread.h>
#include <raw-gadget.hpp>

#include "config.hpp"
#include "DeviceEvent.hpp"
#include "ControllerInjector.hpp"
#include "ControllerState.hpp"
#include "signals.hpp"
#include "Touchpad.hpp"

namespace Chaos {

  // Forward references
  class ControllerInput;
  class GameCommand;

  /**
   * \brief Abstract class to manage the gamepad controller.
   *
   */
  class Controller {
  protected:

 	  // based on DS4 and Dualsense sizes
	  std::deque<std::array<unsigned char,64>> deviceEventQueue;

    virtual bool applyHardware(const DeviceEvent& event) = 0;

    void storeState(const DeviceEvent& event);

    void handleNewDeviceEvent(const DeviceEvent& event);

	
    /**
     * Database for tracking current button states:
     * Input: ((int)event->type<<8) + (int)event->id ] = event->value;
     *  Output: state of that type/id combo
     */
    short controllerState[1024];
	
    ControllerInjector* controllerInjector = nullptr;

    //Touchpad touchpad;
  public:
    Controller();
    
    /**
     * \brief The low-level function to get the current controlller state.
     * \param id The id of the axis/button
     * \param type The signal type
     * \return The current value of the signal.
     *
     * Directly reads the buffer holding the current controller state for the specified button
     * or axis. This is the low-level routine and returns the raw controller state. It does not
     * handle the remapping of signals.
     */
    inline short getState(uint8_t id, uint8_t type) {
      return controllerState[((int) type << 8) + (int) id];
    }

    /**
     * \brief Get the current controlller state of a particular controller input.
     * \param command  The command to check
     *
     * Read the current controller state for the actual command (not the remapped state). Note that
     * for the hybrid controls, we only read the button signal, since currently we're only interested
     * in whether it is on or off. This may need to change for other games.
     */
    short getState(std::shared_ptr<ControllerInput> signal);

    /**
     * \brief Change the controller state
     * 
     * \param event New event to go to the console
     */
    void applyEvent(const DeviceEvent& event);
    
    /**
     * \brief Test if event matches a specific input signal
     * 
     * This tests the signal only, regarless of any condition associated with a game command.
     */
    //bool matches(const DeviceEvent& event, ControllerSignal signal);

    /**
     * \brief Is the event an instance of the specified input command?
     * 
     * This tests both that the event against the defined signal and that any defined condition
     * is also in effect.
     */
    bool matches(const DeviceEvent& event, std::shared_ptr<GameCommand> command);
    
    /**
     * Turns off the button/axis associated with a command.
     * \param[in] The command that we're disabling.
     */
    void setOff(std::shared_ptr<GameCommand> command);

    /**
     * Sets the button/axis associated with a command to its maximum.
     * \param[in] The command that we're turning on.
     */
    void setOn(std::shared_ptr<GameCommand> command);
    
    void addInjector(ControllerInjector* injector);

    
  };

};

