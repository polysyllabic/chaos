/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
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
#include <plog/Log.h>

#include "Sequence.hpp"
#include "Controller.hpp"
#include "ControllerInput.hpp"

using namespace Chaos;

unsigned int Sequence::press_time;
unsigned int Sequence::release_time;

Sequence::Sequence(Controller& c) : controller{c} {}

void Sequence::addEvent(DeviceEvent event) {
  events.push_back(event);
}

void Sequence::addSequence(std::shared_ptr<Sequence> seq) {
  assert (seq);
  events.insert(events.end(), seq->getEvents().begin(), seq->getEvents().end());
}

void Sequence::addPress(std::shared_ptr<ControllerInput> signal, short value) {
  addHold(signal, value, press_time);
  addRelease(signal, release_time);
}

void Sequence::addHold(std::shared_ptr<ControllerInput> signal, short value, unsigned int hold_time) {
  assert(signal);
  short int hybrid_value;  
  unsigned int hybrid_hold;
  // If a value is passed for a hybrid signal (L2/R2), it applies to the axis signal. The button
  // signal will just use 1.
  if (signal->getType() == ControllerSignalType::HYBRID) {
    hybrid_value = (value == 0) ? JOYSTICK_MAX : value;
    value = 1;
    hybrid_hold = hold_time;
    hold_time = 0;
  } else if (value == 0) {
    // For other types, if value not set, we use the maximum value for that signal type.
    value = (signal->getType() == ControllerSignalType::BUTTON ||
	     signal->getType() == ControllerSignalType::THREE_STATE) ? 1 : JOYSTICK_MAX;
  }
  PLOG_DEBUG << "Adding hold: " << (int) signal->getButtonType() << ": " << (int) signal->getID()
    << ":" << value << " for " << hold_time << " microseconds";
  events.push_back({hold_time, value, (uint8_t) signal->getButtonType(), signal->getID()});
  if (signal->getType() == ControllerSignalType::HYBRID) {
    events.push_back( {hybrid_hold, hybrid_value, TYPE_AXIS, signal->getHybridAxis()} );
    PLOG_DEBUG << "Adding hold: " << (int) TYPE_AXIS << ": " << (int) signal->getHybridAxis()
      << ":" << hybrid_value << " for " << hybrid_hold << " microseconds";
  }
}

void Sequence::addRelease(std::shared_ptr<ControllerInput> signal, unsigned int release_time) {
  assert(signal);
  unsigned int hybrid_release;
  if (signal->getType() == ControllerSignalType::HYBRID) {
    hybrid_release = release_time;
    release_time = 0;
  }
  PLOG_DEBUG << "Adding release: " << (int) signal->getButtonType() << ": " << (int) signal->getID()
    << " for " << release_time << " microseconds";
  events.push_back({release_time, 0, (uint8_t) signal->getButtonType(), signal->getID()});
  if (signal->getType() == ControllerSignalType::HYBRID) {
    PLOG_DEBUG << "Adding release: " << TYPE_AXIS << ": " << (int) signal->getHybridAxis()
      << " for " << hybrid_release << " microseconds";
    events.push_back( {hybrid_release, JOYSTICK_MIN, TYPE_AXIS, signal->getHybridAxis()} );
  }
}

void Sequence::addDelay(unsigned int delay) {
  PLOG_DEBUG << "adding delay of " << delay << "usecs";
  events.push_back( {delay, 0, 255, 255} );
}

void Sequence::send() {
  PLOG_DEBUG << "Sending sequence";
  for (auto& event : events) {
    PLOG_DEBUG << "Sending event for input (" << (int) event.type << ":" << (int) event.id
	       << ") value=" << (int) event.value << "; sleeping for " << (int) event.time << " microseconds";
    controller.applyEvent(event);
    if (event.time > 0) {
      usleep(event.time);
    }
  }
}

bool Sequence::sendParallel(double sequenceTime) {
  unsigned int elapsed = (unsigned int) (sequenceTime * SEC_TO_MICROSEC);
  while (current_step < events.size()) {
    DeviceEvent e = events[current_step];
    if (e.isDelay()) {
      // pure delay (no attatched event to apply)
      wait_until += e.time;
      PLOG_DEBUG << "Delay of " << e.time << "usecs";
    } else {
      // return until delay expires, then move to the next step
      if (elapsed < wait_until) {
        return false;
      }
      // send out events until we hit the next delay
      PLOG_DEBUG << "Parallel step " << current_step << ": signal = ("
        << (int) e.type << "." << (int) e.id << ") value = " << e.value << " next delay = " << e.time <<
        "; elapsed usec=" << elapsed;
      controller.applyEvent(e);
      wait_until += e.time;
      ++current_step;
    }
  }
  // We're at the end of the sequence. Wait for any final delay from the last step
  if (elapsed < wait_until) {
    return false;
  }
  // Now we're actually done. Reset the steps and time for the next iteration
  PLOG_DEBUG << "parallel send finished";
  current_step = 0;
  wait_until = 0;
  return true;
}

void Sequence::clear() {
    events.clear();
}

bool Sequence::empty() {
  return events.empty();
}

void Sequence::setPressTime(double time) {
  press_time = (unsigned int) (time * SEC_TO_MICROSEC);
  PLOG_DEBUG << "press_time = " << time << " = " << press_time << " usecs";
}

void Sequence::setReleaseTime(double time) {
  release_time = (unsigned int) (time * SEC_TO_MICROSEC);
  PLOG_DEBUG << "release_time = " << time << " = " << release_time << " usecs";
}


