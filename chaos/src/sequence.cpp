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
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::DX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::DY)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::RX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::RY)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::LX)->getID()} );
  events.push_back( {0, 0, TYPE_AXIS, GamepadInput::get(GPInput::LY)->getID()} );
  events.push_back( {0, -128, TYPE_AXIS, GamepadInput::get(GPInput::L2)->getHybridAxis()} );
  events.push_back( {0, -128, TYPE_AXIS, GamepadInput::get(GPInput::R2)->getHybridAxis()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::X)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::SQUARE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::TRIANGLE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::CIRCLE)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::R1)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::L1)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::R2)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::L2)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::R3)->getID()} );
  events.push_back( {0, 0, TYPE_BUTTON, GamepadInput::get(GPInput::L3)->getID()} );
  events.push_back( {TIME_AFTER_JOYSTICK_DISABLE, 0, TYPE_AXIS, GamepadInput::get(GPInput::LY)->getID()} );
}

void Sequence::addButtonPress(GPInput button) {
  assert(GamepadInput::get(button)->getButtonType() == TYPE_BUTTON);
  
  events.push_back( {  TIME_PER_BUTTON_PRESS, 1, TYPE_BUTTON, GamepadInput::get(button)->getID()} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_BUTTON, GamepadInput::get(button)->getID()} );
}

void Sequence::addButtonHold(GPInput button) {
  assert(GamepadInput::get(button)->getButtonType() == TYPE_BUTTON);
  
  events.push_back( {  0, 1, TYPE_BUTTON, GamepadInput::get(button)->getID()} );
}
void Sequence::addButtonRelease(GPInput button) {
  assert(GamepadInput::get(button)->getButtonType() == TYPE_BUTTON);
  
  events.push_back( {  0, 0, TYPE_BUTTON, GamepadInput::get(button)->getID()} );
}

void Sequence::addTimeDelay(unsigned int timeInMilliseconds) {
  events.push_back( { timeInMilliseconds*1000, 0, 255, 255} );
}

void Sequence::addAxisPress(GPInput axis, short value ) {
  assert(GamepadInput::get(axis)->getButtonType() == TYPE_AXIS);
  
  uint8_t ctrl = (GamepadInput::get(axis)->getType() == GPInputType::HYBRID) ?
    GamepadInput::get(axis)->getHybridAxis() : GamepadInput::get(axis)->getID();
  events.push_back( {  TIME_PER_BUTTON_PRESS, value, TYPE_AXIS, ctrl} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_AXIS, ctrl} );
}

void Sequence::addAxisHold(GPInput axis, short value ) {
  assert(GamepadInput::get(axis)->getButtonType() == TYPE_AXIS);
  
  uint8_t ctrl = (GamepadInput::get(axis)->getType() == GPInputType::HYBRID) ?
    GamepadInput::get(axis)->getHybridAxis() : GamepadInput::get(axis)->getID();
  events.push_back( {  0, value, TYPE_AXIS, ctrl} );
}

void Sequence::send() {
  for (std::vector<DeviceEvent>::iterator it = events.begin(); it != events.end(); it++) {
    DeviceEvent& event = (*it);
    PLOG_VERBOSE << "Sending event for button " << (int) event.type << ": " << (int) event.id
	       << ":" << (int) event.value << "\t sleeping for " << event.time << '\n';
    controller->applyEvent( event );
    if (event.time) {
      usleep(event.time);
    }
  }
}

void Sequence::clear() {
    events.clear();
}
