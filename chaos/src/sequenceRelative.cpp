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
#include <unistd.h>
#include <sys/time.h>
#include <plog/Log.h>

#include "sequence.hpp"


using namespace Chaos;

SequenceRelative::SequenceRelative(Controller* c) : Sequence(c) {
  tickTime = 200;
}

void SequenceRelative::send() {
  struct timeval startTime;
  gettimeofday(&startTime, NULL);

  long long lastTime = 0;
  
  struct timeval currentTime = startTime;
  
  long long runningTimeInMicroseconds = 0;

  bool done;
  
  for (std::vector<DeviceEvent>::iterator it = events.begin(); it != events.end(); it++) {
    DeviceEvent& event = (*it);

    done = false;

    while (!done) {
      // Send ASAP, and then check the next event.
      if (((long long) event.time + lastTime) <= runningTimeInMicroseconds) {
	lastTime += event.time;
	
	controller->applyEvent( event );
	done = true;
	continue;
      }

      // Event not ready. Wait, then check new time
      usleep(tickTime);

      gettimeofday(&currentTime, NULL);
      runningTimeInMicroseconds = (long long)(currentTime.tv_sec = startTime.tv_sec)*1000000 + (long long)(currentTime.tv_usec - startTime.tv_usec);
    }
  }
}
