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

#include <raw-gadget.hpp>

#include "config.hpp"

#include "controllerState.hpp"
#include "chaosUhid.hpp"
#include "signalTypes.hpp"
#include "gamepadInput.hpp"
#include "gameCommand.hpp"

namespace Chaos {

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
   * Since this is a unique hardware device, it's a legit. instance of a singleton.
   * We use Scott Meyers's technique of creating global objects with local static initialization.
   */
  class Controller : public EndpointObserver, public Mogi::Thread {
  private:
    std::deque<std::array<unsigned char,64>> bufferQueue;
	
    ChaosUhid* chaosHid;
	
    // Main purpose of this Mogi::Thread is to handle DeviceEvent queue
    void doAction();

    // overloaded from EndpointObserver
    void notification(unsigned char* buffer, int length);
    
    RawGadgetPassthrough mRawGadgetPassthrough;
    
  protected:
    Controller();
    
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
	
    ControllerInjector* controllerInjector = NULL;

  public:
    void initialize();
    
    static Controller& instance() {
      static Controller controller{};
      return controller;
    }

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
    inline int getState(uint8_t id, uint8_t type) {
      return controllerState[((int) type << 8) + (int) id];
    }
    /**
     * \brief Get the current controlller state of a particular gampepad input.
     * \param command  The command to check
     *
     * Read the current controller state for the given command as it is currently mapped. In other
     * words, this routine returns the state of whatever signal the game command is currently
     * mapped to. Note that for the hybrid controls, we only read the button form, since currently
     * we're only interested in whether it is on or off. This may need to change for other games.
     */
    int getState(std::shared_ptr<GameCommand> command);
    int getState(std::shared_ptr<GamepadInput> signal);

    /**
     * \brief Change the controller state
     * 
     * \param event New event to go to the console
     */
    void applyState(const DeviceEvent& event) { storeState(event); }
    
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

    short joystickLimit(int input) {
      return fmin(fmax(input, JOYSTICK_MIN), JOYSTICK_MAX);
    }
    
  };

};

