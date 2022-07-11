/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
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
#include <algorithm>
#include <stdexcept>

#include "DelayModifier.hpp"
#include "EngineInterface.hpp"
#include "TOMLUtils.hpp"

using namespace Chaos;

const std::string DelayModifier::mod_type = "delay";

DelayModifier::DelayModifier(toml::table& config, EngineInterface* e) {
  
  TOMLUtils::checkValid(config, std::vector<std::string>{"name", "description", "type", "groups",
							  "appliesTo", "delay", "beginSequence", "finishSequence", "unlisted"});
  initialize(config, e);

  if (commands.empty() && ! applies_to_all) {
    throw std::runtime_error("No command(s) specified with 'appliesTo'");
  }
  
  delayTime = config["delay"].value_or(0.0);

  if (delayTime <= 0) {
    throw std::runtime_error("Bad or missing delay time. The 'delay' parameter must be positive.");
  }
  PLOG_VERBOSE << " - delay: " << delayTime;
}

void DelayModifier::update() {
  while ( !eventQueue.empty() ) {
    if( (timer.runningTime() - eventQueue.front().time) >= delayTime ) {
      // Reintroduce the event.
      PLOG_DEBUG << "Defered event sent: " << eventQueue.front().event.type << "." << eventQueue.front().event.id;
      engine->fakePipelinedEvent(eventQueue.front().event, getptr());
      eventQueue.pop();
    }
    else {
      break;
    }
  }
}

// Block the original command that's being delayed. We add it to a queue that is popped and sent
// as a new event when the timer expires
bool DelayModifier::tweak(DeviceEvent& event) {
  // Shortcut if we're working on all commands
  if (applies_to_all) {
    PLOG_DEBUG << "Incoming event (" << event.type << "." << event.id << ") queued";
    eventQueue.push ({this->timer.runningTime(), event});
    return false;
  }
  else {
    for (auto cmd : commands) {
      if (engine->eventMatches(event, cmd)) {
        PLOG_DEBUG << "Incoming event (" << event.type << "." << event.id << ") queued";
      	eventQueue.push ({this->timer.runningTime(), event});
	      return false;
      }
    }
  }
  return true;
}
