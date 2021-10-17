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
#pragma once
#include <vector>

#include "controller.hpp"

namespace Chaos {

  class Sequence {
  protected:
    std::vector<DeviceEvent> events;
    Controller* controller;
    
    unsigned int TIME_AFTER_JOYSTICK_DISABLE = (unsigned int) (1000000/3); // in µs
    unsigned int TIME_PER_BUTTON_PRESS = (unsigned int) (50000*1.25);	// in µs
    unsigned int TIME_PER_BUTTON_RELEASE = (unsigned int) (50000*1.25);	// in µs
    unsigned int MENU_SELECT_DELAY = 50; // in ms
    
  public:
    Sequence(Controller* c) : controller{c} {}

    /**
     * \brief Stop all control input.
     *
     * Needed (probably) to ensure that the user can't interfere with menuing selection.
     */
    void disableControls();
    void addButtonPress(GPInput button);
    void addButtonHold(GPInput button);
    void addButtonRelease(GPInput button);
    void addTimeDelay(unsigned int timeInMilliseconds);
    void addAxisPress(GPInput axis, short value);
    void addAxisHold(GPInput axis, short value);
    
    virtual void send();
    
    void clear();
  };

  class SequenceRelative : public Sequence {
  private:
    int tickTime;

  public:
    SequenceRelative(Controller* c);

    void send();
    void setMinimumTickInMicroseconds(int minTickTime);
  };

  class SequenceAbsolute : public Sequence {
  private:
    int tickTime;

  public:
    SequenceAbsolute(Controller* c);

    void send();
    void setMinimumTickInMicroseconds(int minTickTime);
  };

};
