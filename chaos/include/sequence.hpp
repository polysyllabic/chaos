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
#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP
#include <vector>

#include "controller.hpp"

namespace Chaos {

  class Sequence {
  private:
    std::vector<DeviceEvent> events;
    Controller* controller;
    
    unsigned int TIME_AFTER_JOYSTICK_DISABLE = (unsigned int) (1000000/3); // in µs
    unsigned int TIME_PER_BUTTON_PRESS = (unsigned int) (50000*1.25);	// in µs
    unsigned int TIME_PER_BUTTON_RELEASE = (unsigned int) (50000*1.25);	// in µs
    unsigned int MENU_SELECT_DELAY = 50; // in ms
    
  public:
    Sequence(Controller* c);
    void disablejoysticks();	// Needed for proper menuing, probably
    void addButtonPress( Button id );
    void addButtonHold( Button id );
    void addButtonRelease( Button id );
    void addTimeDelay( unsigned int timeInMilliseconds );
    void addAxisPress( Axis id, short value );
    void addAxisHold( Axis id, short value );
    void send();
    void clear();
  };

};

#endif
