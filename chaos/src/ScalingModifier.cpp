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
#include <cmath>
#include <plog/Log.h>
#include <toml++/toml.h>

#include "config.hpp"
#include "ScalingModifier.hpp"
#include "EngineInterface.hpp"
#include "TOMLUtils.hpp"

using namespace Chaos;

const std::string ScalingModifier::mod_type = "scaling";

ScalingModifier::ScalingModifier(toml::table& config, EngineInterface* e) {
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "applies_to", "begin_sequence", "finish_sequence",
      "unlisted", "amplitude", "offset"});
  initialize(config, e);

  if (commands.empty()) {
    throw std::runtime_error("No commands defined in applies_to");
  }

  amplitude = config["amplitude"].value_or(1.0);
  offset = config["offset"].value_or(0.0);
  sign_tweak = (amplitude < 0.0) ? 1 : 0;
}

bool ScalingModifier::tweak(DeviceEvent& event) {
  // Traverse the list of affected commands
  for (auto& cmd : commands) {
    if (engine->eventMatches(event, cmd)) {     
      event.value = fmin(fmax((int)(amplitude * (event.value+sign_tweak) + offset), JOYSTICK_MIN), JOYSTICK_MAX);
      break;
    }
  }
  return true;
}

