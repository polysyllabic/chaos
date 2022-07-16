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
							  "begin_sequence", "finish_sequence", "applies_to", "trigger", "while", "unless",
                "cumulative", "time_on", "time_off", "unlisted"});
  initialize(config, e);
  if (commands.empty()) {
    throw std::runtime_error("No command associated with cooldown modifier.");
  }

  engine->addGameCommands(config, "trigger", trigger);

  time_on = config["time_on"].value_or(0.0);
  if (time_on <= 0.0) {
    throw std::runtime_error("Cooldown time_on must be a positive number");
  }
  
  time_off = config["time_off"].value_or(0.0);
  if (time_off <= 0.0) {
    throw std::runtime_error("Cooldown time_off must be a positive number");
  }

  cumulative = config["cumulative"].value_or(false);

  PLOG_VERBOSE << "Cooldown " << getName() << ": time_on = " << time_on << "; time_off = " << time_off << "; cumulative = " << cumulative;
  for (auto& t : trigger) {
    PLOG_VERBOSE << "Trigger on event " << t->getName();
  }
  for (auto& c : conditions) {
    PLOG_VERBOSE << "With condition" << c->getName();
  }
}

void CooldownModifier::begin() {
  cooldown_timer = 0;
  state = CooldownState::UNTRIGGERED;
  PLOG_DEBUG << "Initialized " << getName();
}

void CooldownModifier::update() {

  // Get the difference since the last time update.
  double deltaT = timer.dTime();

  if (state == CooldownState::BLOCK) {
    // count down until cooldown ends
    cooldown_timer -= deltaT;
    if (cooldown_timer <= 0) {
      cooldown_timer = 0;
      state = CooldownState::UNTRIGGERED;
      PLOG_DEBUG << "Cooldown for " << getName() << " expired";
    }
  } else if (state == CooldownState::ALLOW) {
    // Increment cooldown timer until time_on exceeded. Cumulative cooldowns only increment
    // if the while condition is true
    if (!cumulative || inCondition()) {
      cooldown_timer += deltaT;
      PLOG_DEBUG << "timer_on period = " << cooldown_timer;
    }
    if (cooldown_timer > time_on) {
      PLOG_DEBUG << "Cooldown for " << getName() << " started";
	    cooldown_timer = time_off;
	    state = CooldownState::BLOCK;
      for (auto& cmd : commands) {
        
      }
      // engine->fakePipelinedEvent(cooldown_event, getptr());
    }
  }
}

bool CooldownModifier::tweak(DeviceEvent& event) {
  if (state == CooldownState::UNTRIGGERED) {
    // Check any incomming event to start the trigger
    for (auto& sig : trigger) {
      if (sig->matches(event) && inCondition()) {
        PLOG_DEBUG << "Trigger matched on event " << (int) event.type << "." << (int) event.id;
        state = CooldownState::ALLOW;
        return true;
      }
    }
  }
  // Block events in the command list while in cooldown
  for (auto& cmd : commands) {
    assert(cmd);
    std::shared_ptr<ControllerInput> sig = cmd->getInput();
    PLOG_DEBUG << "Checking " << cmd->getName() << ", maps to " << ((sig) ? sig->getName() : "NULL");
    if (sig && sig->matches(event)) {
      return state != CooldownState::BLOCK;
    }
  }
  return true;
}
