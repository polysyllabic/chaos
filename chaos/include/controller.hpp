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
#include <array>
#include <memory>

#include <mogi/thread.h>
#include <raw-gadget.hpp>

#include "config.hpp"
#include "deviceEvent.hpp"
#include "controllerState.hpp"
#include "signals.hpp"
#include "touchpad.hpp"

namespace Chaos {

  // Forward references
  class ControllerInput;
  class GameCommand;

  class ControllerInjector  {
  public:
    /**
     * Sniff the input and pass/modify it on way to the output.
     */
    virtual bool sniffify(const DeviceEvent& input, DeviceEvent& output) = 0;
  };

  /**
   * Class to manage the gamepad controller.
   *
   * Since this is a unique hardware device, we use Scott Meyers's technique of creating a global
   * object with local static initialization to ensure it is only created once and is properly
   * initialized on the first access.
   */
  class Controller : public EndpointObserver, public Mogi::Thread {
  private:
    std::deque<std::array<unsigned char,64>> bufferQueue;
	
    //ChaosUhid* chaosHid;
	
    // Main purpose of this Mogi::Thread is to handle DeviceEvent queue
    void doAction();

    // overloaded from EndpointObserver
    void notification(unsigned char* buffer, int length);
    
    RawGadgetPassthrough mRawGadgetPassthrough;
    
  protected:
    
    ControllerState* mControllerState;

    std::deque<DeviceEvent> deviceEventQueue;
	
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
    void applyEvent(const DeviceEvent& event) { storeState(event); }
    
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

    //bool touchpadActive() { return touchpad.isActive(); }
    //void touchpadClearActive() { touchpad.clearActive(); }
    //void setTouchpadActive(bool state) { touchpad.setActive(state); }
    
  };

};

