/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78
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
#include "delaymodifier.h"

using namespace Chaos;

void DelayModifier::DelayModifier(double delay) {
  delayTime = delay;
}

void DelayModifier::update() {
  while ( !eventQueue.empty() ) {
    if( (timer.runningTime() - eventQueue.front().time) >= delayTime ) {
      // Where events are actually sent:
      chaosEngine->fakePipelinedEvent(&eventQueue.front().event, me);
      eventQueue.pop();
    }
    else {
      break;
    }
  }
}

