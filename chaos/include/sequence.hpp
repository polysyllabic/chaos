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
#ifndef SEQUENCE_HPP
#define SEQUENCE_HPP

#include <vector>

#include "controller.hpp"

#define TIME_AFTER_JOYSTICK_DISABLE ((unsigned int)(1000000/3))	// in us
#define TIME_PER_BUTTON_PRESS ((unsigned int)(50000*1.25))	// in us
#define TIME_PER_BUTTON_RELEASE ((unsigned int)(50000*1.25))	// in us
#define MENU_SELECT_DELAY ((unsigned int)(50)) // in ms

namespace Chaos {

  class Sequence {
  private:
    std::vector<DeviceEvent> events;
	
  public:
    Sequence();
    void disablejoysticks();	// Needed for proper menuing, probably
    void addButtonPress( ButtonID id );
    void addButtonHold( ButtonID id );
    void addButtonRelease( ButtonID id );
    void addTimeDelay( unsigned int timeInMilliseconds );
    void addAxisPress( AxisID id, short value );
    void addAxisHold( AxisID id, short value );
    void send(Controller* dualshock);
	
    void clear();
    // For building a sequence:
    //void changeMenuOption();
  };

};

#endif
