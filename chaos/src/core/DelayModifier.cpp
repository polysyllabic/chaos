/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributors.
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
#include <optional>
#include <stdexcept>

#include "DelayModifier.hpp"
#include "EngineInterface.hpp"
#include "TOMLUtils.hpp"

using namespace Chaos;

const std::string DelayModifier::mod_type = "delay";

DelayModifier::DelayModifier(toml::table& config, EngineInterface* e) {
  
  TOMLUtils::checkValid(config, std::vector<std::string>{"name", "description", "type", "groups",
							  "applies_to", "delay", "begin_sequence", "finish_sequence", "unlisted"});
  initialize(config, e);

  if (commands.empty() && ! applies_to_all) {
    throw std::runtime_error("No command(s) specified with 'applies_to'");
  }
  
  delayTime = config["delay"].value_or(0.0);

  if (delayTime <= 0) {
    throw std::runtime_error("Bad or missing delay time. The 'delay' parameter must be positive.");
  }
  PLOG_VERBOSE << " - delay: " << delayTime;
}

void DelayModifier::begin() {
  clearPendingInjectedEvents();
}

void DelayModifier::finish() {
  clearPendingInjectedEvents();
}

std::size_t DelayModifier::clearPendingInjectedEvents() {
  std::lock_guard<std::mutex> lock(queue_mutex);
  const std::size_t cleared = eventQueue.size();
  std::queue<TimeAndEvent> empty;
  std::swap(eventQueue, empty);
  return cleared;
}

void DelayModifier::update() {
  const auto now = std::chrono::steady_clock::now();
  while (true) {
    std::optional<TimeAndEvent> delayed;
    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      if (eventQueue.empty()) {
        break;
      }
      const auto elapsed = std::chrono::duration<double>(now - eventQueue.front().time).count();
      if (elapsed < delayTime) {
        break;
      }
      delayed = eventQueue.front();
      eventQueue.pop();
    }
    if (!delayed) {
      break;
    }
    // Reintroduce the event after dropping the queue lock; injected events can
    // recursively touch this modifier.
    PLOG_DEBUG << "Defered event sent: " << engine->getEventName(delayed->event);
    engine->fakePipelinedEvent(delayed->event, getptr());
  }
}

// Block the original command that's being delayed. We add it to a queue that is popped and sent
// as a new event when the timer expires
bool DelayModifier::tweak(DeviceEvent& event) {
  const auto now = std::chrono::steady_clock::now();
  // Shortcut if we're working on all commands
  if (applies_to_all) {
    PLOG_DEBUG << "Incoming event " << engine->getEventName(event) << " queued";
    std::lock_guard<std::mutex> lock(queue_mutex);
    eventQueue.push ({now, event});
    return false;
  }
  else {
    for (auto cmd : commands) {
      if (engine->eventMatches(event, cmd)) {
        PLOG_DEBUG << "Incoming event (" << engine->getEventName(event) << ") queued";
        std::lock_guard<std::mutex> lock(queue_mutex);
      	eventQueue.push ({now, event});
	      return false;
      }
    }
  }
  return true;
}
