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
      "name", "description", "type", "groups", "appliesTo", "beginSequence", "finishSequence",
      "filter", "threshold", "condition", "conditionTest", "unless", "unlessTest", "unlisted"});

  initialize(config, e);

  if (commands.empty() && ! applies_to_all) {
    throw std::runtime_error("No command(s) specified with 'appliesTo'");
  }

  // If the filter isn't specified, default to block all values of the signal
  filter = DisableFilter::ALL;
  std::optional<std::string_view> f = config["filter"].value<std::string_view>();
  if (f) {
    if (*f == "above") {
      filter = DisableFilter::ABOVE_THRESHOLD;
    } else if (*f == "below") {
      filter = DisableFilter::BELOW_THRESHOLD;
    } else if (*f != "all") {
      PLOG_WARNING << "Unrecognized filter type: '" << *f << "' in definition for '" << config["name"]
		   << "' modifier. Using ALL instead.";
    }
  }
  filterThreshold = config["filterThreshold"].value_or(0);
  
  switch (filter) {
  case DisableFilter::ALL:
    PLOG_VERBOSE << "Filter: ALL";
    break;
  case DisableFilter::ABOVE_THRESHOLD:
    PLOG_VERBOSE << "Filter: ABOVE";
    break;
  case DisableFilter::BELOW_THRESHOLD:
    PLOG_VERBOSE << "Filter: BELOW";
  }
  PLOG_VERBOSE << "FilterThreshold: " << filterThreshold;
}

bool DisableModifier::tweak (DeviceEvent& event) {

  // If the condition test returns false, or the unless test returns true, do not block
  if (inCondition() == false || inUnless() == true) {
    return true;
  }
  
  // Traverse the list of affected commands
  for (auto& cmd : commands) {
    if (engine->eventMatches(event, cmd)) {
      short min_val = (event.type == TYPE_AXIS && cmd->getInput()->getType() == ControllerSignalType::HYBRID)
        ? JOYSTICK_MIN : 0;
      switch (filter) {
      case DisableFilter::ALL:
        PLOG_VERBOSE << "Blocking " << cmd->getName() << "(" << (int) event.type << "." << (int) event.id << ") min_val=" << min_val;
	      event.value = min_val;
        break;
      case DisableFilter::ABOVE_THRESHOLD:
        PLOG_VERBOSE << "+ Filter of " << cmd->getName() << "(" << (int)  event.type << "." << (int)  event.id << ") min_val=" << min_val;
      	event.value = (event.value > filterThreshold) ? min_val : event.value;
      	break;
      case DisableFilter::BELOW_THRESHOLD:
        PLOG_VERBOSE << "- Filter of " << cmd->getName() << "(" << (int)  event.type << "." << (int)  event.id << ") min_val=" << min_val;
      	event.value = (event.value < filterThreshold) ? min_val : event.value;
      }
      // No need to keep searching after a match
      break;
    }
  }
  return true;
}
