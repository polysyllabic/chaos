/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
#include "dualsense.hpp"

using namespace Chaos;

typedef enum DualsenseButtons {
  PS5_BUTTON_X = 0,
  PS5_BUTTON_CIRCLE = 1,
  PS5_BUTTON_TRIANGLE = 2,
  PS5_BUTTON_SQUARE = 3,
  PS5_BUTTON_L1 = 4,
  PS5_BUTTON_R1 = 5,
  PS5_BUTTON_L2 = 6,
  PS5_BUTTON_R2 = 7,
  PS5_BUTTON_SHARE = 8,
  PS5_BUTTON_OPTIONS = 9,
  PS5_BUTTON_PS = 10,
  PS5_BUTTON_L3 = 11,
  PS5_BUTTON_R3 = 12,
  PS5_BUTTON_TOUCHPAD = 13,	// This is handled separately as a mouse event
  PS5_BUTTON_TOUCHPAD_ACTIVE = 14
} DualsenseButtons;
 
 typedef enum DualsenseAxes {
   PS5_AXIS_LX = 0,
   PS5_AXIS_LY = 1,
   PS5_AXIS_L2 = 2,
   PS5_AXIS_RX = 3,
   PS5_AXIS_RY = 4,
   PS5_AXIS_R2 = 5,
   PS5_AXIS_DX = 6,
   PS5_AXIS_DY = 7,
   PS5_AXIS_ACCX = 8,
   PS5_AXIS_ACCY = 9,
   PS5_AXIS_ACCZ = 10,
   PS5_AXIS_GYRX = 11,
   PS5_AXIS_GYRY = 12,
   PS5_AXIS_GYRZ = 13,
   PS5_AXIS_TOUCHPAD_X = 14,
   PS5_AXIS_TOUCHPAD_Y = 15
 } DualsenseAxes;

Dualsense::Dualsense() {
  stateLength = sizeof(inputReport);
  trueState = (void*)new inputReport;
  hackedState = (void*)new inputReport;
}

Dualsense::~Dualsense() {
  delete (inputReport*)trueState;
  delete (inputReport*)hackedState;
}

void Dualsense::applyHackedState(unsigned char* buffer, short* chaosState) {
  inputReport* report = (inputReport*) buffer;
	
  report->BTN_GamePadButton1 = chaosState[PS5_BUTTON_SQUARE];
  report->BTN_GamePadButton2 = chaosState[PS5_BUTTON_X];
  report->BTN_GamePadButton3 = chaosState[PS5_BUTTON_CIRCLE];
  report->BTN_GamePadButton4 = chaosState[PS5_BUTTON_TRIANGLE];
  report->BTN_GamePadButton5 = chaosState[PS5_BUTTON_L1];
  report->BTN_GamePadButton6 = chaosState[PS5_BUTTON_R1];
  report->BTN_GamePadButton7 = chaosState[PS5_BUTTON_L2];
  report->BTN_GamePadButton8 = chaosState[PS5_BUTTON_R2];
  report->BTN_GamePadButton9 = chaosState[PS5_BUTTON_SHARE];
  report->BTN_GamePadButton10 = chaosState[PS5_BUTTON_OPTIONS)];
  report->BTN_GamePadButton11 = chaosState[PS5_BUTTON_L3];
  report->BTN_GamePadButton12 = chaosState[PS5_BUTTON_R3];
  report->BTN_GamePadButton13 = chaosState[PS5_BUTTON_PS];
  report->BTN_GamePadButton14 = chaosState[PS5_BUTTON_TOUCHPAD];
  
  report->GD_GamePadX = packJoystick(chaosState[PS5_AXIS_LX]);
  report->GD_GamePadY = packJoystick(chaosState[PS5_AXIS_LY]);
  report->GD_GamePadZ = packJoystick(chaosState[PS5_AXIS_RX]);
  report->GD_GamePadRz = packJoystick(chaosState[PS5_AXIS_RY]);
	
  report->GD_GamePadHatSwitch = packDpad(chaosState[PS5_AXIS_DX],
					 chaosState[PS5_AXIS_DY]);
}

void  Dualsense::getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events)  {
	
  inputReport* currentState = (inputReport*)buffer;
  inputReport* myState = (inputReport*)trueState;
	
  if (currentState->BTN_GamePadButton1 != myState->BTN_GamePadButton1 ) {
    events.push_back({0, currentState->BTN_GamePadButton1, TYPE_BUTTON, PS5_BUTTON_SQUARE}); }
  if (currentState->BTN_GamePadButton2 != myState->BTN_GamePadButton2 ) {
    events.push_back({0, currentState->BTN_GamePadButton2, TYPE_BUTTON, PS5_BUTTON_X}); }
  if (currentState->BTN_GamePadButton3 != myState->BTN_GamePadButton3 ) {
    events.push_back({0, currentState->BTN_GamePadButton3, TYPE_BUTTON, PS5_BUTTON_CIRCLE}); }
  if (currentState->BTN_GamePadButton4 != myState->BTN_GamePadButton4 ) {
    events.push_back({0, currentState->BTN_GamePadButton4, TYPE_BUTTON, PS5_BUTTON_TRIANGLE}); }
  if (currentState->BTN_GamePadButton5 != myState->BTN_GamePadButton5 ) {
    events.push_back({0, currentState->BTN_GamePadButton5, TYPE_BUTTON, PS5_BUTTON_L1}); }
  if (currentState->BTN_GamePadButton6 != myState->BTN_GamePadButton6 ) {
    events.push_back({0, currentState->BTN_GamePadButton6, TYPE_BUTTON, PS5_BUTTON_R1}); }
  if (currentState->BTN_GamePadButton7 != myState->BTN_GamePadButton7 ) {
    events.push_back({0, currentState->BTN_GamePadButton7, TYPE_BUTTON, PS5_BUTTON_L2}); }
  if (currentState->BTN_GamePadButton8 != myState->BTN_GamePadButton8 ) {
    events.push_back({0, currentState->BTN_GamePadButton8, TYPE_BUTTON, PS5_BUTTON_R2}); }
  if (currentState->BTN_GamePadButton9 != myState->BTN_GamePadButton9 ) {
    events.push_back({0, currentState->BTN_GamePadButton9, TYPE_BUTTON, PS5_BUTTON_SHARE}); }
  if (currentState->BTN_GamePadButton10 != myState->BTN_GamePadButton10 ) {
    events.push_back({0, currentState->BTN_GamePadButton10, TYPE_BUTTON, PS5_BUTTON_OPTIONS}); }
  if (currentState->BTN_GamePadButton11 != myState->BTN_GamePadButton11 ) {
    events.push_back({0, currentState->BTN_GamePadButton11, TYPE_BUTTON, PS5_BUTTON_L3}); }
  if (currentState->BTN_GamePadButton12 != myState->BTN_GamePadButton12 ) {
    events.push_back({0, currentState->BTN_GamePadButton12, TYPE_BUTTON, PS5_BUTTON_R3}); }
  if (currentState->BTN_GamePadButton13 != myState->BTN_GamePadButton13 ) {
    events.push_back({0, currentState->BTN_GamePadButton13, TYPE_BUTTON, PS5_BUTTON_PS}); }
  if (currentState->BTN_GamePadButton14 != myState->BTN_GamePadButton14 ) {
    events.push_back({0, currentState->BTN_GamePadButton14, TYPE_BUTTON, PS5_BUTTON_TOUCHPAD}); }
	
  if (currentState->GD_GamePadX != myState->GD_GamePadX ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadX), TYPE_AXIS, PS5_AXIS_LX}); }
  if (currentState->GD_GamePadY != myState->GD_GamePadY ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadY), TYPE_AXIS, PS5_AXIS_LY}); }
  if (currentState->GD_GamePadZ != myState->GD_GamePadZ ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadZ), TYPE_AXIS, PS5_AXIS_RX}); }
  if (currentState->GD_GamePadRz != myState->GD_GamePadRz ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadRz), TYPE_AXIS, PS5_AXIS_RY}); }
	
  if (currentState->GD_GamePadHatSwitch != myState->GD_GamePadHatSwitch ) {
    short int currentX = positionDX( currentState->GD_GamePadHatSwitch );
    short int currentY = positionDY( currentState->GD_GamePadHatSwitch );
		
    if (positionDY(myState->GD_GamePadHatSwitch) != currentY) {
      events.push_back({0, currentY, TYPE_AXIS, PS5_AXIS_DY});
    }
    if (positionDX(myState->GD_GamePadHatSwitch) != currentX) {
      events.push_back({0, currentX, TYPE_AXIS, PS5_AXIS_DX});
    }		
  }
	
  // Need to compare for next time:
  *(inputReport*)trueState = *currentState;

}
