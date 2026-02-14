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
#include <chrono>
#include <vector>
#include <cstdint>
#include <map>

#include "DeviceEvent.hpp"

namespace Chaos {
  
  /**
   * This class is responsible for interpreting and tracking states of of controllers.
   */
  class ControllerState {
  protected:
    int stateLength;
    bool touchpad_active;
    bool touchpad_timeout_emitted;
    bool touchpad_axis_seen;
    std::chrono::steady_clock::time_point last_touchpad_axis_event;

    static double touchpad_inactive_delay;
    
    void* trueState;
    void* hackedState;

    ControllerState();

    // Helper functions for interpreting raw controller data
    inline short int unpackJoystick(uint8_t& input) { return ((short int) input) - 128;}
    inline uint8_t packJoystick(short int& input) { return input + 128; }
    short int positionDY(const uint8_t& input);
    short int positionDX(const uint8_t& input);
    uint8_t packDpad(const short int& dx, const short int& dy);

    void noteTouchpadActiveEvent(short value);
    void noteTouchpadAxisEvent();
    void addTouchpadInactivityEvents(std::vector<DeviceEvent>& events);
    
  public:
    /**
     * \brief Factory to create the appropriate controller-state class based on the type of controller reported.
     * 
     * \param vendor Vendor ID received from controller
     * \param product Product ID received from controller
     * \return ControllerState* 
     * 
     * Currently only the DualShock is supported, but we could expand this with other controllers if we wanted.
     */
    static ControllerState* factory(int vendor, int product);

    virtual void getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events) = 0;
	
    // This has to be virtual since we don't modify all values in a report structure:
    virtual void applyHackedState(unsigned char* buffer, short* chaosState) = 0;
	
    virtual ~ControllerState() = 0;
		
    void* getHackedState();

    static void setTouchpadInactiveDelay(double delay);
    static double getTouchpadInactiveDelay() { return touchpad_inactive_delay; }

  };
  
};
