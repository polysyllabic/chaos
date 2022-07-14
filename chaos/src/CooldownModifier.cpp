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
#include <cassert>
#include <plog/Log.h>
#include <timer.hpp>

#include "CooldownModifier.hpp"
#include "EngineInterface.hpp"
#include "TOMLUtils.hpp"
#include "GameCommand.hpp"
#include "ControllerInput.hpp"

using namespace Chaos;

const std::string CooldownModifier::mod_type = "cooldown";

CooldownModifier::CooldownModifier(toml::table& config, EngineInterface* e) {
  
  TOMLUtils::checkValid(config, std::vector<std::string>{"name", "description", "type", "groups",
							  "begin_sequence", "finish_sequence", "applies_to", "trigger", "time_on",
                "time_off", "unlisted"});
  initialize(config, e);
  if (commands.empty()) {
    throw std::runtime_error("No command associated with cooldown modifier.");
  }

  if (trigger == nullptr) {
    throw std::runtime_error("No trigger defined");
  }
  
  time_on = config["time_on"].value_or(0.0);
  if (time_on == 0) {
    PLOG_WARNING << "The time_on for this cooldown mod is 0 seconds!";
  }
  PLOG_VERBOSE << " - time_on: " << time_on;
  
  time_off = config["time_off"].value_or(0.0);
  if (time_off == 0) {
    PLOG_WARNING << "The time_off for this cooldown mod is 0 seconds!";
  }
  PLOG_VERBOSE << " - time_off: " << time_off;
}

void CooldownModifier::begin() {
  cooldown_timer = 0;
  in_cooldown = false;
}

void CooldownModifier::update() {

  // Get the difference since the last time update.
  double deltaT = timer.dTime();

  if (in_cooldown) {
    // count down until cooldown ends
    cooldown_timer -= deltaT;
    if (cooldown_timer <= 0) {
      cooldown_timer = 0;
      in_cooldown = false;
      PLOG_DEBUG << "Cooldown for " << getName() << " expired";
    }
  } else {
    if (trigger->isTriggered()) {
      // increment cooldown timer until time_on exceeded
      cooldown_timer += deltaT;
      if (cooldown_timer > time_on) {
        PLOG_DEBUG << "Cooldown for " << getName() << " started";
	      cooldown_timer = time_off;
	      in_cooldown = true;
	      // Disable event necessary?
	      // engine->fakePipelinedEvent(cooldown_event, getptr());
      }
    }
  }
}

bool CooldownModifier::tweak(DeviceEvent& event) {
  // Block events in the command list while in cooldown
  for (auto& cmd : commands) {
    if (cmd->getInput()->matches(event)) {
      return !in_cooldown;
    }
  }
  return true;
}
