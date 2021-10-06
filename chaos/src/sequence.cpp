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

Sequence::Sequence(Controller* c) : controller{c} {
}

void Sequence::disablejoysticks() {
  events.push_back( {0, 0, TYPE_AXIS, controller->getAxis(Axis::DX)} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getAxis(Axis::DY)} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getAxis(Axis::RX)} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getAxis(Axis::RY)} );
  events.push_back( {0, 0, TYPE_AXIS, controller->getAxis(Axis::LX)} );
  events.push_back( {0, -128, TYPE_AXIS, controller->getAxis(Axis::L2)} );
  events.push_back( {0, -128, TYPE_AXIS, controller->getAxis(Axis::R2)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::X)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::SQUARE)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::TRIANGLE)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::CIRCLE)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::R1)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::L1)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::R2)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::L2)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::R3)} );
  events.push_back( {0, 0, TYPE_BUTTON, controller->getButton(Button::L3)} );
  events.push_back( {TIME_AFTER_JOYSTICK_DISABLE, 0, TYPE_AXIS, controller->getAxis(Axis::LY)} );
}

void Sequence::addButtonPress( Button id ) {
  events.push_back( {  TIME_PER_BUTTON_PRESS, 1, TYPE_BUTTON, controller->getButton(id)} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_BUTTON, controller->getButton(id)} );
}

void Sequence::addButtonHold( Button id ) {
  events.push_back( {  0, 1, TYPE_BUTTON, controller->getButton(id)} );
}
void Sequence::addButtonRelease( Button id ) {
  events.push_back( {  0, 0, TYPE_BUTTON, controller->getButton(id)} );
}

void Sequence::addTimeDelay( unsigned int timeInMilliseconds ) {
  events.push_back( { timeInMilliseconds*1000, 0, 255, 255} );
}

void Sequence::addAxisPress( Axis id, short value ) {
  events.push_back( {  TIME_PER_BUTTON_PRESS, value, TYPE_AXIS, controller->getAxis(id)} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_AXIS, controller->getAxis(id)} );
}

void Sequence::addAxisHold( Axis id, short value ) {
  events.push_back( {  TIME_PER_BUTTON_PRESS, value, TYPE_AXIS, controller->getAxis(id)} );
}

void Sequence::send() {
  for (std::vector<DeviceEvent>::iterator it = events.begin(); it != events.end(); it++) {
    DeviceEvent& event = (*it);
    PLOG_VERBOSE << "Sending event for button " << (int) event.type << ": " << (int) event.id
	       << ":" << (int) event.value << "\t sleeping for " << event.time << '\n';
    controller->applyEvent( &event );
    usleep( event.time );
  }
}

void Sequence::clear() {
    events.clear();
}
