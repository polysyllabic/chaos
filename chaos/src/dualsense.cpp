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

Dualsense::Dualsense() {
  buttons[Button::X]        = 1;
  buttons[Button::CIRCLE]   = 2;
  buttons[Button::TRIANGLE] = 3;
  buttons[Button::SQUARE]   = 0;
  buttons[Button::L1]       = 4;
  buttons[Button::R1]       = 5;
  buttons[Button::L2]       = 6;
  buttons[Button::R2]       = 7;
  buttons[Button::SHARE]    = 8;
  buttons[Button::OPTIONS]  = 9;
  buttons[Button::PS]       = 12;
  buttons[Button::L3]       = 10;
  buttons[Button::R3]       = 11;
  buttons[Button::TOUCHPAD] = 13;
  
  axes[Axis::LX] = 0;
  axes[Axis::LY] = 1;
  axes[Axis::L2] = 2;
  axes[Axis::RX] = 3;
  axes[Axis::RY] = 5;
  axes[Axis::R2] = 4;
  axes[Axis::DX] = 6;
  axes[Axis::DY] = 7;
  axes[Axis::ACCX] = 8;
  axes[Axis::ACCY] = 9;
  axes[Axis::ACCZ] = 10;
  axes[Axis::GYRX] = 11;
  axes[Axis::GYRY] = 12;
  axes[Axis::GYRZ] = 13;
  axes[Axis::TOUCHPAD_X] = 14;
  axes[Axis::TOUCHPAD_Y] = 15; 

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
	
  report->BTN_GamePadButton1 = chaosState[toIndex(Button::SQUARE)];
  report->BTN_GamePadButton2 = chaosState[toIndex(Button::X)];
  report->BTN_GamePadButton3 = chaosState[toIndex(Button::CIRCLE)];
  report->BTN_GamePadButton4 = chaosState[toIndex(Button::TRIANGLE)];
  report->BTN_GamePadButton5 = chaosState[toIndex(Button::L1)];
  report->BTN_GamePadButton6 = chaosState[toIndex(Button::R1)];
  report->BTN_GamePadButton7 = chaosState[toIndex(Button::L2)];
  report->BTN_GamePadButton8 = chaosState[toIndex(Button::R2)];
  report->BTN_GamePadButton9 = chaosState[toIndex(Button::SHARE)];
  report->BTN_GamePadButton10 = chaosState[toIndex(Button::OPTIONS)];
  report->BTN_GamePadButton11 = chaosState[toIndex(Button::L3)];
  report->BTN_GamePadButton12 = chaosState[toIndex(Button::R3)];
  report->BTN_GamePadButton13 = chaosState[toIndex(Button::PS)];
  report->BTN_GamePadButton14 = chaosState[toIndex(Button::TOUCHPAD)];
  
  report->GD_GamePadX = packJoystick(chaosState[toIndex(Axis::LX)]);
  report->GD_GamePadY = packJoystick(chaosState[toIndex(Axis::LY)]);
  report->GD_GamePadZ = packJoystick(chaosState[toIndex(Axis::RX)]);
  report->GD_GamePadRz = packJoystick(chaosState[toIndex(Axis::RY)]);
	
  report->GD_GamePadHatSwitch = packDpad(chaosState[toIndex(Axis::DX)],
					 chaosState[toIndex(Axis::DY)]);
}

void  Dualsense::getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events)  {
	
  inputReport* currentState = (inputReport*)buffer;
  inputReport* myState = (inputReport*)trueState;
	
  if (currentState->BTN_GamePadButton1 != myState->BTN_GamePadButton1 ) {
    events.push_back({0, currentState->BTN_GamePadButton1, TYPE_BUTTON, getButton(Button::SQUARE)}); }
  if (currentState->BTN_GamePadButton2 != myState->BTN_GamePadButton2 ) {
    events.push_back({0, currentState->BTN_GamePadButton2, TYPE_BUTTON, getButton(Button::X)}); }
  if (currentState->BTN_GamePadButton3 != myState->BTN_GamePadButton3 ) {
    events.push_back({0, currentState->BTN_GamePadButton3, TYPE_BUTTON, getButton(Button::CIRCLE)}); }
  if (currentState->BTN_GamePadButton4 != myState->BTN_GamePadButton4 ) {
    events.push_back({0, currentState->BTN_GamePadButton4, TYPE_BUTTON, getButton(Button::TRIANGLE)}); }
  if (currentState->BTN_GamePadButton5 != myState->BTN_GamePadButton5 ) {
    events.push_back({0, currentState->BTN_GamePadButton5, TYPE_BUTTON, getButton(Button::L1)}); }
  if (currentState->BTN_GamePadButton6 != myState->BTN_GamePadButton6 ) {
    events.push_back({0, currentState->BTN_GamePadButton6, TYPE_BUTTON, getButton(Button::R1)}); }
  if (currentState->BTN_GamePadButton7 != myState->BTN_GamePadButton7 ) {
    events.push_back({0, currentState->BTN_GamePadButton7, TYPE_BUTTON, getButton(Button::L2)}); }
  if (currentState->BTN_GamePadButton8 != myState->BTN_GamePadButton8 ) {
    events.push_back({0, currentState->BTN_GamePadButton8, TYPE_BUTTON, getButton(Button::R2)}); }
  if (currentState->BTN_GamePadButton9 != myState->BTN_GamePadButton9 ) {
    events.push_back({0, currentState->BTN_GamePadButton9, TYPE_BUTTON, getButton(Button::SHARE)}); }
  if (currentState->BTN_GamePadButton10 != myState->BTN_GamePadButton10 ) {
    events.push_back({0, currentState->BTN_GamePadButton10, TYPE_BUTTON, getButton(Button::OPTIONS)}); }
  if (currentState->BTN_GamePadButton11 != myState->BTN_GamePadButton11 ) {
    events.push_back({0, currentState->BTN_GamePadButton11, TYPE_BUTTON, getButton(Button::L3)}); }
  if (currentState->BTN_GamePadButton12 != myState->BTN_GamePadButton12 ) {
    events.push_back({0, currentState->BTN_GamePadButton12, TYPE_BUTTON, getButton(Button::R3)}); }
  if (currentState->BTN_GamePadButton13 != myState->BTN_GamePadButton13 ) {
    events.push_back({0, currentState->BTN_GamePadButton13, TYPE_BUTTON, getButton(Button::PS)}); }
  if (currentState->BTN_GamePadButton14 != myState->BTN_GamePadButton14 ) {
    events.push_back({0, currentState->BTN_GamePadButton14, TYPE_BUTTON, getButton(Button::TOUCHPAD)}); }
	
  if (currentState->GD_GamePadX != myState->GD_GamePadX ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadX), TYPE_AXIS, getAxis(Axis::LX)}); }
  if (currentState->GD_GamePadY != myState->GD_GamePadY ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadY), TYPE_AXIS, getAxis(Axis::LY)}); }
  if (currentState->GD_GamePadZ != myState->GD_GamePadZ ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadZ), TYPE_AXIS, getAxis(Axis::RX)}); }
  if (currentState->GD_GamePadRz != myState->GD_GamePadRz ) {
    events.push_back({0, unpackJoystick(currentState->GD_GamePadRz), TYPE_AXIS, getAxis(Axis::RY)}); }
	
  if (currentState->GD_GamePadHatSwitch != myState->GD_GamePadHatSwitch ) {
    short int currentX = positionDX( currentState->GD_GamePadHatSwitch );
    short int currentY = positionDY( currentState->GD_GamePadHatSwitch );
		
    if (positionDY(myState->GD_GamePadHatSwitch) != currentY) {
      events.push_back({0, currentY, TYPE_AXIS, getAxis(Axis::DY)});
    }
    if (positionDX(myState->GD_GamePadHatSwitch) != currentX) {
      events.push_back({0, currentX, TYPE_AXIS, getAxis(Axis::DX)});
    }		
  }
	
  // Need to compare for next time:
  *(inputReport*)trueState = *currentState;

}
