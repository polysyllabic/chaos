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
#include <stdexcept>
#include <string_view>

#include <toml++/toml.h>
#include <plog/Log.h>

#include "config.hpp"
#include "DisableModifier.hpp"
#include "TOMLUtils.hpp"
#include "GameCommand.hpp"
#include "ControllerInput.hpp"

using namespace Chaos;

const std::string DisableModifier::mod_type = "disable";

DisableModifier::DisableModifier(toml::table& config, EngineInterface* e) {
  PLOG_VERBOSE << "constructing disable modifier";
  assert(config.contains("name"));
  assert(config.contains("type"));
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "applies_to", "begin_sequence", "finish_sequence",
      "filter", "while", "while_operation", "unlisted"});

  initialize(config, e);

  if (commands.empty() && ! applies_to_all) {
    throw std::runtime_error("No command(s) specified with 'applies_to'");
  }

  // If the filter isn't specified, default to block all values of the signal
  filter = DisableFilter::ALL;
  std::optional<std::string_view> f = config["filter"].value<std::string_view>();
  if (f) {
    if (*f == "above") {
      filter = DisableFilter::ABOVE;
    } else if (*f == "below") {
      filter = DisableFilter::BELOW;
    } else if (*f != "all") {
      PLOG_WARNING << "Unrecognized filter type: '" << *f << "' in definition for '" << config["name"]
		   << "' modifier. Using ALL instead.";
    }
  }
  
  switch (filter) {
  case DisableFilter::ALL:
    PLOG_VERBOSE << "Filter: ALL";
    break;
  case DisableFilter::ABOVE:
    PLOG_VERBOSE << "Filter: ABOVE";
    break;
  case DisableFilter::BELOW:
    PLOG_VERBOSE << "Filter: BELOW";
  }

  condition_active_last = false;
  blocked_command_values.clear();
}

void DisableModifier::begin() {
  condition_active_last = inCondition();
  blocked_command_values.clear();
  if (condition_active_last) {
    clampAppliedCommands();
  }
}

void DisableModifier::update() {
  syncConditionState();
}

void DisableModifier::clampAppliedCommands() {
  if (applies_to_all) {
    return;
  }
  for (auto& cmd : commands) {
    if (!cmd) {
      continue;
    }
    auto signal = cmd->getInput();
    if (!signal) {
      continue;
    }

    DeviceEvent current = {0, engine->getState(signal->getID(), signal->getButtonType()),
                           static_cast<uint8_t>(signal->getButtonType()), signal->getID()};
    const short filtered = getFilteredVal(current);
    if (filtered != current.value) {
      engine->setValue(cmd, filtered);
    }
  }
}

void DisableModifier::restoreBlockedCommands() {
  if (applies_to_all || blocked_command_values.empty()) {
    return;
  }
  for (auto& cmd : commands) {
    if (!cmd) {
      continue;
    }
    auto it = blocked_command_values.find(cmd->getName());
    if (it == blocked_command_values.end()) {
      continue;
    }
    engine->setValue(cmd, it->second);
  }
  blocked_command_values.clear();
}

void DisableModifier::syncConditionState() {
  const bool condition_active = inCondition();
  if (condition_active && !condition_active_last) {
    clampAppliedCommands();
  } else if (!condition_active && condition_active_last) {
    restoreBlockedCommands();
  }
  condition_active_last = condition_active;
}

short DisableModifier::getFilteredVal(DeviceEvent& event) {
  short rval = (event.type == TYPE_AXIS && ControllerInput::getType(event) == ControllerSignalType::HYBRID)
        ? JOYSTICK_MIN : 0;
  if (filter == DisableFilter::ABOVE) {
   	rval = (event.value > 0) ? rval : event.value;
  } else if (filter == DisableFilter::BELOW) {
   	rval = (event.value < 0) ? rval : event.value;
  }
  if (rval != event.value) {
    PLOG_VERBOSE << "filter applied " << engine->getEventName(event) << "(from " << (int) event.value << " to " << rval << ")";
  }
  return rval;
}

bool DisableModifier::tweak (DeviceEvent& event) {
  syncConditionState();

  short new_val = event.value;
  std::string cmd_name;
  // If the condition test returns false do not block
  if (!condition_active_last) {
    return true;
  }
  // Traverse the list of affected commands
  if (applies_to_all) {
    new_val = getFilteredVal(event);
  } else {
    for (auto& cmd : commands) {
      if (engine->eventMatches(event, cmd)) {
        blocked_command_values[cmd->getName()] = event.value;
        new_val = getFilteredVal(event);
        cmd_name = cmd->getName();
        // Already matched, no need to keep looping
        break;
      }
    }
  }
  if (new_val != event.value) {
    PLOG_DEBUG << "Blocking " << cmd_name << "(" << (int) event.type << "." << (int) event.id << ") value= " <<
      event.value << " set to " << new_val;
  }
  event.value = new_val;
  return true;
}
