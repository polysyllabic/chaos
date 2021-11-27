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
#include <stdexcept>
#include <string_view>

#include <toml++/toml.h>
#include <plog/Log.h>

#include "disableModifier.hpp"

using namespace Chaos;

const std::string DisableModifier::name = "disable";

DisableModifier::DisableModifier(const toml::table& config) {
  initialize(config);

  if (commands.empty() && ! applies_to_all) {
    throw std::runtime_error("No command(s) specified with 'appliesTo'");
  }
  
  disableOnStart = config["disableOnStart"].value_or(false);

  // If the filter isn't specified, default to block all values of the signal
  filter = DisableFilter::ALL;
  std::optional<std::string_view> f = config["filter"].value<std::string_view>();
  if (f) {
    if (*f == "above") {
      filter = DisableFilter::ABOVE_THRESHOLD;
    } else if (*f == "below") {
      filter = DisableFilter::BELOW_THRESHOLD;
    } else if (*f != "all") {
      PLOG_WARNING << "Unrecognized filter type: '" << *f << "'. Using ALL instead.\n";
    }
  }
  filter_threshold = config["filterThreshold"].value_or(0);
  
  condition = GPInput::NONE;
  std::optional<std::string> cond = config["condition"].value<std::string>();

  if (config.contains("unless")) {
    // 'condition' and 'unless' are mutually exclusive options.
    if (cond) {
      PLOG_ERROR << "Cannot use both 'condition' and 'unless' in one command definition.\n";
      throw std::runtime_error("Incompatible command options");
    }
    cond = config["unless"].value<std::string>();
    invertCondition = true;
  } 
  
  // If the condition doesn't match a defined GameCommand name, this will be set to GPInput::NONE.
  if (cond) {
    condition = GameCommand::getInput(*cond);
  }
}

void DisableModifier::begin() {
  DeviceEvent event;
  if (disableOnStart) {
    // Turn off the commands we're disabling if the player is currently holding down that input
    for (auto& cmd : commands) {
      controller->setOff(cmd);
    }
  }
}

/**
 * \brief Disables an incoming game command, in part or in full
 * \param event The incoming device event we need to check
 * \return Whether the event is valid. Returning false stops further processing.
 *
 * We look up the current mapping of the command to button preses and block presses comming from
 * that button/axis. This ensures that we disable the right control regardless of any command
 * remapping that may have occurred.
 */
bool DisableModifier::tweak (DeviceEvent& event) {
  bool disable = false;
  updateGamestates(event);
  if (! inGamestate()) {
    return true;
  }
  // Check any condition
  if (condition != GPInput::NONE) {
    // Only filter if in state under the ordinary condition, or the state is false with inverted
    // polarity. In other words, if the two states are equal, we do not filter and so return immediately.
    if (controller->isOn(condition) == invertCondition) {
      return true;
    }
  }
  // Traverse the list of affected commands
  for (auto& cmd : commands) {
    if (controller->matches(event, cmd)) {
      switch (filter) {
      case DisableFilter::ALL:
	event.value = 0;
	break;
      case DisableFilter::ABOVE_THRESHOLD:
	event.value = (event.value > filter_threshold) ? 0 : event.value;
	break;
      case DisableFilter::BELOW_THRESHOLD:
	event.value = (event.value < filter_threshold) ? 0 : event.value;
      }
      // no need to keep searching after a match
      break;
    }
  }
  return true;
}
