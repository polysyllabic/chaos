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
  std::optional<int> ft = config["filterThreshold"].value<int>();
  if (ft) {
    filter_threshold = *ft;
  }

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
  
  // Condition/unless should be the name of a previously defined GameCommand
  if (cond) {
    if (GameCommand::bindingMap.count(*cond) == 1) {
      // Look up the controller command that maps to this game command. We store the index to the
      // controller command for efficiency.
      
      condition = GameCommand::bindingMap.at(*cond)->getReal();
      condition_remap = condition;
      // default threshold for a condition
      condition_threshold = 1;
      
    } else {
      PLOG_ERROR << "Condition '" << *cond << "' is not a defined command.\n"; 
      throw std::runtime_error("Condition is an undefined command");
    }
  }
  
}

// add an explicit event to turn off if the button is currently pressed when we apply the mod.
void DisableModifier::begin() {
  DeviceEvent event;
  if (disableOnStart) {
    for (auto& cmd : commands) {
      controller->setOff(cmd->getReal());
    }
  }
}

/**
 * Disables an incoming game command, in part or in full
 *
 * We look up the current mapping of the command to button preses and block presses comming from
 * that button/axis. This ensures that we disable the right control regardless of any command
 * remapping that may have occurred.
 */
bool DisableModifier::tweak (DeviceEvent* event) {
  // check any condition
  if (condition_threshold) {
    // State is true if the value equals or exceeds the threshold
    bool state = controller->isState(condition_remap, condition_threshold);
    // Don't filter if the state is false under the ordinary condition, or the state is true with
    // inverted polarity.
    if ((state && invertCondition) || (!state && !invertCondition)) {
      return true;
    }
  }
  // traverse the list of affected commands
  // we match against the remapped command but alter the real one
  for (auto& cmd : commands) {
    int new_val;
    if (controller->matches(event, cmd->getRemap())) {
      switch (filter) {
      case DisableFilter::ALL:
	new_val = 0;
	break;
      case DisableFilter::ABOVE_THRESHOLD:
	new_val = (event->value > filter_threshold) ? 0 : event->value;
	break;
      case DisableFilter::BELOW_THRESHOLD:
	new_val = (event->value < filter_threshold) ? 0 : event->value;
      }
      controller->setState(event, new_val, cmd->getRemap(), cmd->getReal());
    }
  }
  return true;
}
