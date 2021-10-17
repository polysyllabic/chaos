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
#include <vector>
#include <cstdint>
#include <map>

#include "controllerCommand.hpp"
#include "gameCommand.hpp"
#include "deviceTypes.hpp"

namespace Chaos {
  
  /**
   * This class is responsible for interpreting and tracking states of of controllers.
   */
  class ControllerState {
  protected:
    int stateLength;

    unsigned short lastTouchpadCountFromController;
    unsigned short currentTouchpadCount;
    bool shouldClearTouchpadCount;
    
    void* trueState;
    void* hackedState;

    static const short int JOYSTICK_MIN = -128;
    static const short int JOYSTICK_MAX = 128;

    // Helper functions for raw interpretation:
    
    inline short int unpackJoystick(uint8_t& input) { return ((short int) input) - 128;}
    inline uint8_t packJoystick(short int& input) { return input + 128; }

    short int fixShort(short int input);
    // short int unfixShort(short int& input);

    short int positionDY(const uint8_t& input );
    short int  positionDX(const uint8_t& input );
    uint8_t packDpad(const short int& dx, const short int& dy);
    
  public:
    static ControllerState* factory(int vendor, int product);

    /**
     * Holds the data about the button/axis commands.
     */
    static std::vector<ControllerCommand> buttonInfo;
    
    virtual void getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events) = 0;
	
    // This has to be virtual since we don't modify all values in a report structure:
    virtual void applyHackedState(unsigned char* buffer, short* chaosState) = 0;
	
    virtual ~ControllerState() = 0;
	
    void* getHackedState();

    inline short int getJoystickMin() { return JOYSTICK_MIN; }
    inline short int getJoystickMax() { return JOYSTICK_MAX; }

  };
  
};
