/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#ifndef DUALSHOCK_HPP
#define DUALSHOCK_HPP

#include <cstdint>

#include "controllerState.hpp"

namespace Chaos {

  class Dualshock : public ControllerState {
    friend ControllerState;
	
  protected:
    Dualshock();
	
  public:
    void applyHackedState(unsigned char* buffer, short* chaosState);
    
    ~Dualshock();

  private:
    void getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events);

    typedef struct {
      uint8_t counter : 7;
      uint8_t active : 1;	// first 7 bits are a counter.  bit 8 is a touch indicator
      int16_t x : 12;
      int16_t y : 12;	
    }__attribute__((packed)) TouchpadFinger;

    typedef struct {
      uint8_t timestamp;
      TouchpadFinger finger1;
      TouchpadFinger finger2;
    }__attribute__((packed)) TouchpadEvent;

    typedef struct {
      uint8_t  reportId;                   // Report ID = 0x01 (1)
      uint8_t  GD_GamePadX;                // Usage 0x00010030: X, Value = 0 to 255
      uint8_t  GD_GamePadY;                // Usage 0x00010031: Y, Value = 0 to 255
      uint8_t  GD_GamePadZ;                // Usage 0x00010032: Z, Value = 0 to 255
      uint8_t  GD_GamePadRz;               // Usage 0x00010035: Rz, Value = 0 to 255
      uint8_t  GD_GamePadHatSwitch : 4;    // Usage 0x00010039: Hat switch, Value = 0 to 7, Physical = Value x 45 in degrees
      uint8_t  BTN_GamePadButton1 : 1;     // Usage 0x00090001: Button 1 Primary/trigger, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton2 : 1;     // Usage 0x00090002: Button 2 Secondary, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton3 : 1;     // Usage 0x00090003: Button 3 Tertiary, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton4 : 1;     // Usage 0x00090004: Button 4, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton5 : 1;     // Usage 0x00090005: Button 5, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton6 : 1;     // Usage 0x00090006: Button 6, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton7 : 1;     // Usage 0x00090007: Button 7, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton8 : 1;     // Usage 0x00090008: Button 8, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton9 : 1;     // Usage 0x00090009: Button 9, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton10 : 1;    // Usage 0x0009000A: Button 10, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton11 : 1;    // Usage 0x0009000B: Button 11, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton12 : 1;    // Usage 0x0009000C: Button 12, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton13 : 1;    // Usage 0x0009000D: Button 13, Value = 0 to 1, Physical = Value x 315
      uint8_t  BTN_GamePadButton14 : 1;    // Usage 0x0009000E: Button 14, Value = 0 to 1, Physical = Value x 315
      uint8_t  VEN_GamePad0020 : 6;        // Usage 0xFF000020: , Value = 0 to 127, Physical = Value x 315 / 127
      uint8_t  GD_GamePadRx;               // Usage 0x00010033: Rx, Value = 0 to 255, Physical = Value x 21 / 17
      uint8_t  GD_GamePadRy;               // Usage 0x00010034: Ry, Value = 0 to 255, Physical = Value x 21 / 17
      uint16_t timestamp; // 10,11
      uint8_t  VEN_GamePadunknown1; // 12	// Seems to only report 0x16
      int16_t  GD_GYRO_X; // 13
      int16_t  GD_GYRO_Y; // 15
      int16_t  GD_GYRO_Z; // 17
      int16_t  GD_ACC_X; // 19
      int16_t  GD_ACC_Y; // 21
      int16_t  GD_ACC_Z; // 23	
      uint8_t  VEN_GamePadunknown2[5]; // 25
      uint8_t  BATTERY; // 30
      uint8_t  VEN_GamePadunknown3[2]; // 31, 32	
      uint8_t  TOUCH_COUNT; // 33
      TouchpadEvent TOUCH_EVENTS[3];	// 34 (9-bytes each -> 27 total	
      uint8_t  VEN_GamePad0021[3];        // Usage 0xFF000021: , Value = 0 to 255, Physical = Value x 21 / 17	
    }__attribute__((packed)) inputReport;
  
  };

};

#endif
