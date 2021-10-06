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
#include "dualshock.hpp"

using namespace Chaos;

Dualshock::Dualshock() {
  stateLength = sizeof(inputReport);
  trueState = (void*) new inputReport;
  hackedState = (void*) new inputReport;
}

Dualshock::~Dualshock() {
  delete (inputReport*) trueState;
  delete (inputReport*) hackedState;
}

void Dualshock::applyHackedState(unsigned char* buffer, short* chaosState) {
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
  
  report->GD_ACC_X = fixShort(chaosState[toIndex(Axis::ACCX)]);
  report->GD_ACC_Y = fixShort(chaosState[toIndex(Axis::ACCY)]);
  report->GD_ACC_Z = fixShort(chaosState[toIndex(Axis::ACCZ)]);
	
  report->GD_GamePadRx = packJoystick(chaosState[toIndex(Axis::L2)]);
  report->GD_GamePadRy = packJoystick(chaosState[toIndex(Axis::R2)]);
	
  report->GD_GamePadHatSwitch = packDpad(chaosState[toIndex(Axis::DX)],
					 chaosState[toIndex(Axis::DY)]);
	
  *(inputReport*) hackedState = *report;
}

void Dualshock::getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events)  {
	
  inputReport currentState = *(inputReport*)buffer;
	
  if (currentState.BTN_GamePadButton1 != ((inputReport*)trueState)->BTN_GamePadButton1 ) {
    events.push_back({0, currentState.BTN_GamePadButton1, TYPE_BUTTON, getButton(Button::SQUARE)}); }
  if (currentState.BTN_GamePadButton2 != ((inputReport*)trueState)->BTN_GamePadButton2 ) {
    events.push_back({0, currentState.BTN_GamePadButton2, TYPE_BUTTON, getButton(Button::X)}); }
  if (currentState.BTN_GamePadButton3 != ((inputReport*)trueState)->BTN_GamePadButton3 ) {
    events.push_back({0, currentState.BTN_GamePadButton3, TYPE_BUTTON, getButton(Button::CIRCLE)}); }
  if (currentState.BTN_GamePadButton4 != ((inputReport*)trueState)->BTN_GamePadButton4 ) {
    events.push_back({0, currentState.BTN_GamePadButton4, TYPE_BUTTON, getButton(Button::TRIANGLE)}); }
  if (currentState.BTN_GamePadButton5 != ((inputReport*)trueState)->BTN_GamePadButton5 ) {
    events.push_back({0, currentState.BTN_GamePadButton5, TYPE_BUTTON, getButton(Button::L1)}); }
  if (currentState.BTN_GamePadButton6 != ((inputReport*)trueState)->BTN_GamePadButton6 ) {
    events.push_back({0, currentState.BTN_GamePadButton6, TYPE_BUTTON, getButton(Button::R1)}); }
  if (currentState.BTN_GamePadButton7 != ((inputReport*)trueState)->BTN_GamePadButton7 ) {
    events.push_back({0, currentState.BTN_GamePadButton7, TYPE_BUTTON, getButton(Button::L2)}); }
  if (currentState.BTN_GamePadButton8 != ((inputReport*)trueState)->BTN_GamePadButton8 ) {
    events.push_back({0, currentState.BTN_GamePadButton8, TYPE_BUTTON, getButton(Button::R2)}); }
  if (currentState.BTN_GamePadButton9 != ((inputReport*)trueState)->BTN_GamePadButton9 ) {
    events.push_back({0, currentState.BTN_GamePadButton9, TYPE_BUTTON, getButton(Button::SHARE)}); }
  if (currentState.BTN_GamePadButton10 != ((inputReport*)trueState)->BTN_GamePadButton10 ) {
    events.push_back({0, currentState.BTN_GamePadButton10, TYPE_BUTTON, getButton(Button::OPTIONS)}); }
  if (currentState.BTN_GamePadButton11 != ((inputReport*)trueState)->BTN_GamePadButton11 ) {
    events.push_back({0, currentState.BTN_GamePadButton11, TYPE_BUTTON, getButton(Button::L3)}); }
  if (currentState.BTN_GamePadButton12 != ((inputReport*)trueState)->BTN_GamePadButton12 ) {
    events.push_back({0, currentState.BTN_GamePadButton12, TYPE_BUTTON, getButton(Button::R3)}); }
  if (currentState.BTN_GamePadButton13 != ((inputReport*)trueState)->BTN_GamePadButton13 ) {
    events.push_back({0, currentState.BTN_GamePadButton13, TYPE_BUTTON, getButton(Button::PS)}); }
  if (currentState.BTN_GamePadButton14 != ((inputReport*)trueState)->BTN_GamePadButton14 ) {
    events.push_back({0, currentState.BTN_GamePadButton14, TYPE_BUTTON, getButton(Button::TOUCHPAD)}); }
	
  if (currentState.GD_GamePadX != ((inputReport*)trueState)->GD_GamePadX ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadX), TYPE_AXIS, getAxis(Axis::LX)}); }
  if (currentState.GD_GamePadY != ((inputReport*)trueState)->GD_GamePadY ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadY), TYPE_AXIS, getAxis(Axis::LY)}); }
  if (currentState.GD_GamePadZ != ((inputReport*)trueState)->GD_GamePadZ ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadZ), TYPE_AXIS, getAxis(Axis::RX)}); }
  if (currentState.GD_GamePadRz != ((inputReport*)trueState)->GD_GamePadRz ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadRz), TYPE_AXIS, getAxis(Axis::RY)}); }
  if (currentState.GD_GamePadRx != ((inputReport*)trueState)->GD_GamePadRx ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadRx), TYPE_AXIS, getAxis(Axis::L2)}); }
  if (currentState.GD_GamePadRy != ((inputReport*)trueState)->GD_GamePadRy ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadRy), TYPE_AXIS, getAxis(Axis::R2)}); }
  if (currentState.GD_ACC_X != ((inputReport*)trueState)->GD_ACC_X ) {
    events.push_back({0, fixShort(currentState.GD_ACC_X), TYPE_AXIS, getAxis(Axis::ACCX)}); }
  if (currentState.GD_ACC_Y != ((inputReport*)trueState)->GD_ACC_Y ) {
    events.push_back({0, fixShort(currentState.GD_ACC_Y), TYPE_AXIS, getAxis(Axis::ACCY)}); }
  if (currentState.GD_ACC_Z != ((inputReport*)trueState)->GD_ACC_Z ) {
    events.push_back({0, fixShort(currentState.GD_ACC_Z), TYPE_AXIS, getAxis(Axis::ACCZ)}); }
	
  if (currentState.GD_GamePadHatSwitch != ((inputReport*)trueState)->GD_GamePadHatSwitch ) {
    short int currentX = positionDX( currentState.GD_GamePadHatSwitch );
    short int currentY = positionDY( currentState.GD_GamePadHatSwitch );

    if (positionDY(((inputReport*)trueState)->GD_GamePadHatSwitch) != currentY) {
      events.push_back({0, currentY, TYPE_AXIS, getAxis(Axis::DY)});
    }
    if (positionDX(((inputReport*)trueState)->GD_GamePadHatSwitch) != currentX) {
      events.push_back({0, currentX, TYPE_AXIS, getAxis(Axis::DX)});
    }
  }
	
  // Touchpad events:
  if (currentState.TOUCH_EVENTS[0].finger1.active != ((inputReport*)trueState)->TOUCH_EVENTS[0].finger1.active ) {
    events.push_back({0, currentState.TOUCH_EVENTS[0].finger1.active, TYPE_BUTTON, getButton(Button::TOUCHPAD_ACTIVE)}); }
  if (currentState.TOUCH_EVENTS[0].finger1.x != ((inputReport*)trueState)->TOUCH_EVENTS[0].finger1.x) {
    events.push_back({0, currentState.TOUCH_EVENTS[0].finger1.x, TYPE_AXIS, getAxis(Axis::TOUCHPAD_X)}); }
  if (currentState.TOUCH_EVENTS[0].finger1.y != ((inputReport*)trueState)->TOUCH_EVENTS[0].finger1.y) {
    events.push_back({0, currentState.TOUCH_EVENTS[0].finger1.y, TYPE_AXIS, getAxis(Axis::TOUCHPAD_Y)}); }
	
  // Need to compare for next time:
  *(inputReport*)trueState = currentState;
}
