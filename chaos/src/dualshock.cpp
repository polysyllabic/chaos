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

  touchCounter = 0;
  touchTimeStamp = 0;
  priorFingerActive[0] = 0;
  priorFingerActive[1] = 0;
}

Dualshock::~Dualshock() {
  delete (inputReport*) trueState;
  delete (inputReport*) hackedState;
}

void Dualshock::applyHackedState(unsigned char* buffer, short* chaosState) {
  inputReport* report = (inputReport*) buffer;
	
  report->BTN_GamePadButton1 = chaosState[buttonInfo[GPInput::SQUARE].getIndex()];
  report->BTN_GamePadButton2 = chaosState[buttonInfo[GPInput::X].getIndex()];
  report->BTN_GamePadButton3 = chaosState[buttonInfo[GPInput::CIRCLE].getIndex()];
  report->BTN_GamePadButton4 = chaosState[buttonInfo[GPInput::TRIANGLE].getIndex()];
  report->BTN_GamePadButton5 = chaosState[buttonInfo[GPInput::L1].getIndex()];
  report->BTN_GamePadButton6 = chaosState[buttonInfo[GPInput::R1].getIndex()];
  report->BTN_GamePadButton7 = chaosState[buttonInfo[GPInput::L2].getIndex()];
  report->BTN_GamePadButton8 = chaosState[buttonInfo[GPInput::R2].getIndex()];
  report->BTN_GamePadButton9 = chaosState[buttonInfo[GPInput::SHARE].getIndex()];
  report->BTN_GamePadButton10 = chaosState[buttonInfo[GPInput::OPTIONS].getIndex()];
  report->BTN_GamePadButton11 = chaosState[buttonInfo[GPInput::L3].getIndex()];
  report->BTN_GamePadButton12 = chaosState[buttonInfo[GPInput::R3].getIndex()];
  report->BTN_GamePadButton13 = chaosState[buttonInfo[GPInput::PS].getIndex()];
  report->BTN_GamePadButton14 = chaosState[buttonInfo[GPInput::TOUCHPAD].getIndex()];
	
  report->GD_GamePadX = packJoystick(chaosState[buttonInfo[GPInput::LX].getIndex()]);
  report->GD_GamePadY = packJoystick(chaosState[buttonInfo[GPInput::LY].getIndex()]);
  report->GD_GamePadZ = packJoystick(chaosState[buttonInfo[GPInput::RX].getIndex()]);
  report->GD_GamePadRz = packJoystick(chaosState[buttonInfo[GPInput::RY].getIndex()]);
  
  report->GD_ACC_X = fixShort(chaosState[buttonInfo[GPInput::ACCX].getIndex()]);
  report->GD_ACC_Y = fixShort(chaosState[buttonInfo[GPInput::ACCY].getIndex()]);
  report->GD_ACC_Z = fixShort(chaosState[buttonInfo[GPInput::ACCZ].getIndex()]);
	
  report->GD_GamePadRx = packJoystick(chaosState[buttonInfo[GPInput::L2].getIndex()]);
  report->GD_GamePadRy = packJoystick(chaosState[buttonInfo[GPInput::R2].getIndex()]);
	
  report->GD_GamePadHatSwitch = packDpad(chaosState[buttonInfo[GPInput::DX].getIndex()],
					 chaosState[buttonInfo[GPInput::DY].getIndex()]);


  // Touchpad handing. Yup, it's a lot and shouldn't be handled here.
  touchTimeStamp += 7;  // sometimes this also increments by 8

  if ((chaosState[buttonInfo[GPInput::TOUCHPAD_ACTIVE].getIndex()] &&
       (lastX[0] != chaosState[buttonInfo[GPInput::TOUCHPAD_X].getIndex()] ||
	lastX[1] != chaosState[buttonInfo[GPInput::TOUCHPAD_X_2].getIndex()])) ||
      (chaosState[buttonInfo[GPInput::TOUCHPAD_ACTIVE].getIndex()] &&
       (lastY[0] != chaosState[buttonInfo[GPInput::TOUCHPAD_Y].getIndex()] ||
	lastY[1] != chaosState[buttonInfo[GPInput::TOUCHPAD_Y_2].getIndex()])))
     {
       touchTimeStampToReport = touchTimeStamp;
       report ->TOUCH_EVENTS[0].timestamp = touchTimeStampToReport;

       lastX[0] = chaosState[buttonInfo[GPInput::TOUCHPAD_X].getIndex()];
       lastX[1] = chaosState[buttonInfo[GPInput::TOUCHPAD_X_2].getIndex()];
       lastY[0] = chaosState[buttonInfo[GPInput::TOUCHPAD_Y].getIndex()];
       lastY[1] = chaosState[buttonInfo[GPInput::TOUCHPAD_Y_2].getIndex()];
     }

  if (chaosState[buttonInfo[GPInput::TOUCHPAD_ACTIVE].getIndex()]) {
    report->TOUCH_COUNT = 1;
    report->TOUCH_EVENTS[0].finger[0].active = 0;
    report->TOUCH_EVENTS[0].finger[0].x = chaosState[buttonInfo[GPInput::TOUCHPAD_X].getIndex()];
    report->TOUCH_EVENTS[0].finger[0].y = chaosState[buttonInfo[GPInput::TOUCHPAD_Y].getIndex()];
    
    if (priorFingerActive[0] == 0) {
      priorFingerActive[0] = 1;
      currentTouchpadCount++;
      currentTouchpadCount &= 0x7F;
      touchCounterSaved[0] = currentTouchpadCount;
    }

    report->TOUCH_EVENTS[0].finger[0].counter = touchCounterSaved[0];
    
  } else {
    priorFingerActive[0] = 0;
    report->TOUCH_EVENTS[0].finger[0].active = 1;
  }
  
  *(inputReport*) hackedState = *report;
}

void Dualshock::getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events)  {
	
  inputReport currentState = *(inputReport*)buffer;
	
  if (currentState.BTN_GamePadButton1 != ((inputReport*)trueState)->BTN_GamePadButton1 ) {
    events.push_back({0, currentState.BTN_GamePadButton1, TYPE_BUTTON, buttonInfo[GPInput::SQUARE].getID()}); }
  if (currentState.BTN_GamePadButton2 != ((inputReport*)trueState)->BTN_GamePadButton2 ) {
    events.push_back({0, currentState.BTN_GamePadButton2, TYPE_BUTTON, buttonInfo[GPInput::X].getID()}); }
  if (currentState.BTN_GamePadButton3 != ((inputReport*)trueState)->BTN_GamePadButton3 ) {
    events.push_back({0, currentState.BTN_GamePadButton3, TYPE_BUTTON, buttonInfo[GPInput::CIRCLE].getID()}); }
  if (currentState.BTN_GamePadButton4 != ((inputReport*)trueState)->BTN_GamePadButton4 ) {
    events.push_back({0, currentState.BTN_GamePadButton4, TYPE_BUTTON, buttonInfo[GPInput::TRIANGLE].getID()}); }
  if (currentState.BTN_GamePadButton5 != ((inputReport*)trueState)->BTN_GamePadButton5 ) {
    events.push_back({0, currentState.BTN_GamePadButton5, TYPE_BUTTON, buttonInfo[GPInput::L1].getID()}); }
  if (currentState.BTN_GamePadButton6 != ((inputReport*)trueState)->BTN_GamePadButton6 ) {
    events.push_back({0, currentState.BTN_GamePadButton6, TYPE_BUTTON, buttonInfo[GPInput::R1].getID()}); }
  if (currentState.BTN_GamePadButton7 != ((inputReport*)trueState)->BTN_GamePadButton7 ) {
    events.push_back({0, currentState.BTN_GamePadButton7, TYPE_BUTTON, buttonInfo[GPInput::L2].getID()}); }
  if (currentState.BTN_GamePadButton8 != ((inputReport*)trueState)->BTN_GamePadButton8 ) {
    events.push_back({0, currentState.BTN_GamePadButton8, TYPE_BUTTON, buttonInfo[GPInput::R2].getID()}); }
  if (currentState.BTN_GamePadButton9 != ((inputReport*)trueState)->BTN_GamePadButton9 ) {
    events.push_back({0, currentState.BTN_GamePadButton9, TYPE_BUTTON, buttonInfo[GPInput::SHARE].getID()}); }
  if (currentState.BTN_GamePadButton10 != ((inputReport*)trueState)->BTN_GamePadButton10 ) {
    events.push_back({0, currentState.BTN_GamePadButton10, TYPE_BUTTON, buttonInfo[GPInput::OPTIONS].getID()}); }
  if (currentState.BTN_GamePadButton11 != ((inputReport*)trueState)->BTN_GamePadButton11 ) {
    events.push_back({0, currentState.BTN_GamePadButton11, TYPE_BUTTON, buttonInfo[GPInput::L3].getID()}); }
  if (currentState.BTN_GamePadButton12 != ((inputReport*)trueState)->BTN_GamePadButton12 ) {
    events.push_back({0, currentState.BTN_GamePadButton12, TYPE_BUTTON, buttonInfo[GPInput::R3].getID()}); }
  if (currentState.BTN_GamePadButton13 != ((inputReport*)trueState)->BTN_GamePadButton13 ) {
    events.push_back({0, currentState.BTN_GamePadButton13, TYPE_BUTTON, buttonInfo[GPInput::PS].getID()}); }
  if (currentState.BTN_GamePadButton14 != ((inputReport*)trueState)->BTN_GamePadButton14 ) {
    events.push_back({0, currentState.BTN_GamePadButton14, TYPE_BUTTON, buttonInfo[GPInput::TOUCHPAD].getID()}); }
	
  if (currentState.GD_GamePadX != ((inputReport*)trueState)->GD_GamePadX ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadX), TYPE_AXIS, buttonInfo[GPInput::LX].getID()}); }
  if (currentState.GD_GamePadY != ((inputReport*)trueState)->GD_GamePadY ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadY), TYPE_AXIS, buttonInfo[GPInput::LY].getID()}); }
  if (currentState.GD_GamePadZ != ((inputReport*)trueState)->GD_GamePadZ ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadZ), TYPE_AXIS, buttonInfo[GPInput::RX].getID()}); }
  if (currentState.GD_GamePadRz != ((inputReport*)trueState)->GD_GamePadRz ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadRz), TYPE_AXIS, buttonInfo[GPInput::RY].getID()}); }
  if (currentState.GD_GamePadRx != ((inputReport*)trueState)->GD_GamePadRx ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadRx), TYPE_AXIS, buttonInfo[GPInput::L2].getSecondID()}); }
  if (currentState.GD_GamePadRy != ((inputReport*)trueState)->GD_GamePadRy ) {
    events.push_back({0, unpackJoystick(currentState.GD_GamePadRy), TYPE_AXIS, buttonInfo[GPInput::R2].getSecondID()}); }
  if (currentState.GD_ACC_X != ((inputReport*)trueState)->GD_ACC_X ) {
    events.push_back({0, fixShort(currentState.GD_ACC_X), TYPE_AXIS, buttonInfo[GPInput::ACCX].getID()}); }
  if (currentState.GD_ACC_Y != ((inputReport*)trueState)->GD_ACC_Y ) {
    events.push_back({0, fixShort(currentState.GD_ACC_Y), TYPE_AXIS, buttonInfo[GPInput::ACCY].getID()}); }
  if (currentState.GD_ACC_Z != ((inputReport*)trueState)->GD_ACC_Z ) {
    events.push_back({0, fixShort(currentState.GD_ACC_Z), TYPE_AXIS, buttonInfo[GPInput::ACCZ].getID()}); }
	
  if (currentState.GD_GamePadHatSwitch != ((inputReport*)trueState)->GD_GamePadHatSwitch ) {
    short int currentX = positionDX( currentState.GD_GamePadHatSwitch );
    short int currentY = positionDY( currentState.GD_GamePadHatSwitch );

    if (positionDY(((inputReport*)trueState)->GD_GamePadHatSwitch) != currentY) {
      events.push_back({0, currentY, TYPE_AXIS, buttonInfo[GPInput::DY].getID()});
    }
    if (positionDX(((inputReport*)trueState)->GD_GamePadHatSwitch) != currentX) {
      events.push_back({0, currentX, TYPE_AXIS, buttonInfo[GPInput::DX].getID()});
    }
  }
	
  // Touchpad events:
  for (int e = 0; e < ((inputReport*)trueState)->TOUCH_COUNT; e++) {
    for (int f = 0; f < 2; f++) {
      if (currentState.TOUCH_EVENTS[e].finger[f].active != ((inputReport*) trueState)->TOUCH_EVENTS[e].finger[f].active) {
	events.push_back({0, !currentState.TOUCH_EVENTS[e].finger[f].active, TYPE_BUTTON, buttonInfo[GPInput::TOUCHPAD_ACTIVE].getID()});
      }
      
      if (currentState.TOUCH_EVENTS[e].finger[f].x != ((inputReport*) trueState)->TOUCH_EVENTS[e].finger[f].x) {
	events.push_back({0, currentState.TOUCH_EVENTS[e].finger[f].x, TYPE_AXIS, buttonInfo[GPInput::TOUCHPAD_X].getID()});      
      }

      if (currentState.TOUCH_EVENTS[e].finger[f].y != ((inputReport*) trueState)->TOUCH_EVENTS[e].finger[f].y) {
	events.push_back({0, currentState.TOUCH_EVENTS[e].finger[f].y, TYPE_AXIS, buttonInfo[GPInput::TOUCHPAD_Y].getID()});      
      }
    }
  }
 
  // Need to compare for next time:
  *(inputReport*)trueState = currentState;
}
