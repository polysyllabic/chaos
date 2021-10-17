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
#include "sequence.hpp"

#include <unistd.h>
#include <plog/Log.h>

using namespace Chaos;

void Sequence::disableControls() {
  events.push_back( {0, 0, TYPE_AXIS, controller->getGPInfo(GPInput::DX).getID()} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getGPID(GPInput::DY)} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getGPID(GPInput::RX)} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getGPID(GPInput::RY)} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getGPID(GPInput::LX)} );
  events.push_back( {0, -128, TYPE_AXIS, controller->getGPSecondID(GPInput::L2)} );
  events.push_back( {0, -128, TYPE_AXIS, controller->getGPSecondID(GPInput::R2)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::X)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::SQUARE)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::TRIANGLE)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::CIRCLE)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::R1)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::L1)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::R2)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::L2)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::R3)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getGPID(GPInput::L3)} );
  events.push_back( {TIME_AFTER_JOYSTICK_DISABLE, 0, TYPE_AXIS, controller->getGPID(GPInput::LY)} );
}

void Sequence::addButtonPress(GPInput button) {
  assert(controller->getGPButtonType(button) == TYPE_BUTTON);
  
  events.push_back( {  TIME_PER_BUTTON_PRESS, 1, TYPE_BUTTON, controller->getGPID(button)} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_BUTTON, controller->getGPID(button)} );
}

void Sequence::addButtonHold(GPInput button) {
  assert(controller->getGPButtonType(button) == TYPE_BUTTON);
  
  events.push_back( {  0, 1, TYPE_BUTTON, controller->getGPID(button)} );
}
void Sequence::addButtonRelease(GPInput button) {
  assert(controller->getGPButtonType(button) == TYPE_BUTTON);
  
  events.push_back( {  0, 0, TYPE_BUTTON, controller->getGPID(button)} );
}

void Sequence::addTimeDelay(unsigned int timeInMilliseconds) {
  events.push_back( { timeInMilliseconds*1000, 0, 255, 255} );
}

void Sequence::addAxisPress(GPInput axis, short value ) {
  assert(controller->getGPButtonType(axis) == TYPE_AXIS);
  
  uint8_t ctrl = (controller->getGPType(axis) == GPInputType::HYBRID) ?
    controller->getGPSecondID(axis) : controller->getGPID(axis);
  events.push_back( {  TIME_PER_BUTTON_PRESS, value, TYPE_AXIS, ctrl} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_AXIS, ctrl} );
}

void Sequence::addAxisHold(GPInput axis, short value ) {
  assert(controller->getGPButtonType(axis) == TYPE_AXIS);
  
  uint8_t ctrl = (controller->getGPType(axis) == GPInputType::HYBRID) ?
    controller->getGPSecondID(axis) : controller->getGPID(axis);
  events.push_back( {  0, value, TYPE_AXIS, ctrl} );
}

void Sequence::send() {
  for (std::vector<DeviceEvent>::iterator it = events.begin(); it != events.end(); it++) {
    DeviceEvent& event = (*it);
    PLOG_VERBOSE << "Sending event for button " << (int) event.type << ": " << (int) event.id
	       << ":" << (int) event.value << "\t sleeping for " << event.time << '\n';
    controller->applyEvent( &event );
    if (event.time) {
      usleep(event.time);
    }
  }
}

void Sequence::clear() {
    events.clear();
}
