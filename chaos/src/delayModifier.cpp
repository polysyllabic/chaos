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
#include <algorithm>
#include <stdexcept>

#include "delayModifier.hpp"
#include "chaosEngine.hpp"

using namespace Chaos;

const std::string DelayModifier::name = "delay";

DelayModifier::DelayModifier(const toml::table& config) {
  initialize(config);

  if (commands.empty() && ! applies_to_all) {
    throw std::runtime_error("No command(s) specified with 'appliesTo'");
  }
  
  std::optional<double> delay_time = config["value"].value<double>();
  if (delay_time) {
    delayTime = *delay_time;
  }
  else {
    throw std::runtime_error("Missing 'value' or wrong value type (double required).");
  }
}

void DelayModifier::update() {
  while ( !eventQueue.empty() ) {
    if( (timer.runningTime() - eventQueue.front().time) >= delayTime ) {
      // Reintroduce the event.
      engine->fakePipelinedEvent(eventQueue.front().event, me);
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
    eventQueue.push ({this->timer.runningTime(), event});
    return false;
  }
  else {
    for (auto& cmd : commands) {
      if (controller->matches(event, cmd->getBinding())) {
	eventQueue.push ({this->timer.runningTime(), event});
	return false;
      }
    }
  }
  return true;
}
