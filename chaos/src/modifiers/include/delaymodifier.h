/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78, with additional contributions by polysyl.
*
* TCC is free software: you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* TCC is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along
* with TCC.  If not, see <https://www.gnu.org/licenses/>.
*---------------------------------------------------------------------------*/
#ifndef DELAYMODIFIER_H
#define DELAYMODIFIER_H

#include "modifier.h"

namespace Chaos {

  typedef struct _TimeAndEvent{
    double time;
    DeviceEvent event;
  } TimeAndEvent;

  // Subclass of modifiers that introduce a delay between a button press and
  // when it is passed to the controller.
  class DelayModifier : public Chaos::Modifier {
  private:
    std::queue<TimeAndEvent> eventQueue;
    double delayTime;

  protected:
    DelayModifier(double delay);

  };
};

#endif
