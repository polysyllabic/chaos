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
#ifndef CONTROLLER_STATE_HPP
#define CONTROLLER_STATE_HPP
#include <vector>
#include <cstdint>
#include <map>

#include "deviceTypes.hpp"

namespace Chaos {
  
  /**
   * This class is responsible for interpreting and tracking states of of raw-usb controllers.
   */
  class ControllerState {
  protected:
    int stateLength;	
    void* trueState;
    void* hackedState;

    /**
     * We map button and axis enumerations onto a specific numerical value in the child classes.
     * This allows us (at least theoretically) to support multiple  controllers without recompiling.
     * The default mapping below is for the DualShock controller.
     */
    std::map<Button, uint8_t> buttons {
      {Button::X, 0},
      {Button::CIRCLE, 1},
      {Button::TRIANGLE, 2},
      {Button::SQUARE, 3},
      {Button::L1, 4},
      {Button::R1, 5},
      {Button::L2, 6},
      {Button::R2, 7},
      {Button::SHARE, 8},
      {Button::OPTIONS, 9},
      {Button::PS, 10},
      {Button::L3, 11},
      {Button::R3, 12},
      {Button::TOUCHPAD, 13},
      {Button::TOUCHPAD_ACTIVE, 14}
    };
    std::map<Axis, uint8_t> axes {
      {Axis::LX, 0},
      {Axis::LY, 1},
      {Axis::L2, 2},
      {Axis::RX, 3},
      {Axis::RY, 4},
      {Axis::R2, 5},
      {Axis::DX, 6},
      {Axis::DY, 7},
      {Axis::ACCX, 8},
      {Axis::ACCY, 9},
      {Axis::ACCZ, 10},
      {Axis::GYRX, 11},
      {Axis::GYRY, 12},
      {Axis::GYRZ, 13},
      {Axis::TOUCHPAD_X, 14},
      {Axis::TOUCHPAD_Y, 15}
    };
    
    short int JOYSTICK_MIN = -128;
    short int JOYSTICK_MAX = 128;

  public:
    static ControllerState* factory(int vendor, int product);
	
    virtual void getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events) = 0;
	
    // This has to be virtual since we don't modify all values in a report structure:
    virtual void applyHackedState(unsigned char* buffer, short* chaosState) = 0;
	
    virtual ~ControllerState() = 0;
	
    void* getHackedState();

    uint8_t getButton(Button b);
    uint8_t getAxis(Axis a);

    short int getJoystickMin();
    short int getJoystickMax();
    
    // Helper functions for raw interpretation:
    int toIndex(Button b);
    int toIndex(Axis a);
    
    short int unpackJoystick(uint8_t& input);
    uint8_t packJoystick(short int& input);

    short int fixShort(short int input);
    short int unfixShort(short int& input);

    short int positionDY( const uint8_t& input );
    short int  positionDX( const uint8_t& input );
    uint8_t packDpad( const short int& dx, const short int& dy );
        
  };
  
};

#endif
