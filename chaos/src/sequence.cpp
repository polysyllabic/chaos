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
#include "sequence.hpp"

#include <unistd.h>
#include <plog/Log.h>

using namespace Chaos;

Sequence::Sequence() {
}

void Sequence::disablejoysticks() {
  events.push_back( {0, 0, TYPE_AXIS, AXIS_DX} );
  events.push_back( {0, 0, TYPE_AXIS, AXIS_DY} );
  events.push_back( {0, 0, TYPE_AXIS, AXIS_RX} );
  events.push_back( {0, 0, TYPE_AXIS, AXIS_RY} );
  events.push_back( {0, 0, TYPE_AXIS, AXIS_LX} );
  events.push_back( {0, -128, TYPE_AXIS, AXIS_L2} );
  events.push_back( {0, -128, TYPE_AXIS, AXIS_R2} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_X} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_SQUARE} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_TRIANGLE} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_CIRCLE} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_R1} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_L1} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_R2} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_L2} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_R3} );
  events.push_back( {0, 0, TYPE_BUTTON, BUTTON_L3} );
  events.push_back( {TIME_AFTER_JOYSTICK_DISABLE, 0, TYPE_AXIS, AXIS_LY} );
}

void Sequence::addButtonPress( ButtonID id ) {
  events.push_back( {  TIME_PER_BUTTON_PRESS, 1, TYPE_BUTTON, (unsigned char) id} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_BUTTON, (unsigned char) id} );
}

void Sequence::addButtonHold( ButtonID id ) {
  events.push_back( {  0, 1, TYPE_BUTTON, (unsigned char) id} );
}
void Sequence::addButtonRelease( ButtonID id ) {
  events.push_back( {  0, 0, TYPE_BUTTON, (unsigned char) id} );
}

void Sequence::addTimeDelay( unsigned int timeInMilliseconds ) {
  events.push_back( { timeInMilliseconds*1000, 0, 255, 255} );
}

void Sequence::addAxisPress( AxisID id, short value ) {
  events.push_back( {  TIME_PER_BUTTON_PRESS, value, TYPE_AXIS, (unsigned char) id} );
  events.push_back( {TIME_PER_BUTTON_RELEASE, 0, TYPE_AXIS, (unsigned char) id} );
}

void Sequence::addAxisHold( AxisID id, short value ) {
  events.push_back( {  TIME_PER_BUTTON_PRESS, value, TYPE_AXIS, (unsigned char) id} );
}

void Sequence::send(Controller* dualshock) {
  for (std::vector<DeviceEvent>::iterator it = events.begin(); it != events.end(); it++) {
    DeviceEvent& event = (*it);
    PLOG_DEBUG << "Sending event for button " << (int)event.type << ": " << (int)event.id << ":" << (int)event.value << "\t sleeping for " << event.time << std::endl;
    dualshock->applyEvent( &event );
    usleep( event.time );
  }
}

void Sequence::clear() {
    events.clear();
}
