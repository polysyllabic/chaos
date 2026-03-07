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
							  "begin_sequence", "finish_sequence", "applies_to", "while", "while_operation",
                "start_type", "time_on", "time_off", "trigger", "unlisted"});
  initialize(config, e);
  if (commands.empty()) {
    throw std::runtime_error("No command associated with cooldown modifier.");
  }

  time_on = config["time_on"].value_or(0.0);
  if (time_on <= 0.0) {
    throw std::runtime_error("Cooldown time_on must be a positive number");
  }
  
  time_off = config["time_off"].value_or(0.0);
  if (time_off <= 0.0) {
    throw std::runtime_error("Cooldown time_off must be a positive number");
  }

  engine->addGameCommands(config, "trigger", trigger);

  if (config.contains("cumulative")) {
    throw std::runtime_error(
        "Cooldown key 'cumulative' is no longer supported. Use start_type = \"cumulative\".");
  }

  start_type = CooldownStartType::TRIGGER;
  std::optional<std::string> start_type_str = config["start_type"].value<std::string>();
  if (start_type_str) {
    if (*start_type_str == "trigger") {
      start_type = CooldownStartType::TRIGGER;
    } else if (*start_type_str == "cumulative") {
      start_type = CooldownStartType::CUMULATIVE;
    } else if (*start_type_str == "cancelable") {
      start_type = CooldownStartType::CANCELABLE;
    } else {
      throw std::runtime_error("Unrecognized start_type: " + *start_type_str);
    }
  }

  PLOG_VERBOSE << "Cooldown " << getName() << ": time_on = " << time_on << "; time_off = " << time_off
               << "; start_type = "
               << ((start_type == CooldownStartType::TRIGGER) ? "trigger"
                   : ((start_type == CooldownStartType::CUMULATIVE) ? "cumulative" : "cancelable"));
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
  blocked_command_values.clear();
  PLOG_DEBUG << "Initialized " << getName();
}

void CooldownModifier::snapshotBlockedCommands() {
  blocked_command_values.clear();
  for (auto& cmd : commands) {
    if (!cmd) {
      continue;
    }
    auto signal = cmd->getInput();
    if (!signal) {
      continue;
    }
    blocked_command_values[cmd->getName()] = engine->getState(signal->getID(), signal->getButtonType());
  }
}

void CooldownModifier::restoreBlockedCommands() {
  if (blocked_command_values.empty()) {
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

void CooldownModifier::update() {

  // Get the difference since the last time update.
  double deltaT = timer.dTime();

  if (state == CooldownState::UNTRIGGERED) {
    // Scan for the triggering condition
    if (trigger.empty() && inCondition()) {
      // If there's no trigger, start the timer as soon as the condition is true
      state = CooldownState::ALLOW;
      PLOG_DEBUG << "Cooldown timer " << getName() << " triggered";
    }
  } else if (state == CooldownState::BLOCK) {
    // count down until cooldown ends
    cooldown_timer -= deltaT;
    if (cooldown_timer <= 0) {
      cooldown_timer = 0;
      state = CooldownState::UNTRIGGERED;
      restoreBlockedCommands();
      PLOG_DEBUG << "Cooldown for " << getName() << " expired";
    }
  } else if (state == CooldownState::ALLOW) {
    const bool cond = inCondition();
    if (start_type == CooldownStartType::CANCELABLE && !cond) {
      // Cancelable cooldowns reset completely if the condition drops before block starts.
      cooldown_timer = 0;
      state = CooldownState::UNTRIGGERED;
      PLOG_DEBUG << "Cooldown for " << getName() << " canceled before block started";
      return;
    }

    // Trigger mode always advances once ALLOW begins.
    // Cumulative and cancelable modes advance only while condition is true.
    if (start_type == CooldownStartType::TRIGGER || cond) {
      cooldown_timer += deltaT;
      PLOG_VERBOSE << "timer_on period = " << cooldown_timer;
    }
    if (cooldown_timer > time_on) {
      PLOG_DEBUG << "Cooldown for " << getName() << " started";
	    cooldown_timer = time_off;
	    state = CooldownState::BLOCK;
      snapshotBlockedCommands();
      // Turn off all the commands we're blocking.
      // This was formerly done through fakePipelinedEvent(), but since remapping is now done
      // before we see this event, I don't think it needs to be pipelined.
      for (auto& cmd : commands) {
        PLOG_DEBUG << "Disable " << cmd->getName();
        engine->setOff(cmd);
      }
    }
  }
}

bool CooldownModifier::tweak(DeviceEvent& event) {
  if (state == CooldownState::UNTRIGGERED && !trigger.empty()) {
    // If this event matches any of the commands in the trigger, check the condition and start the
    // timer if true
    for (auto& sig : trigger) {
      if (sig->getIndex() == event.index() && inCondition()) {
        PLOG_DEBUG << "Begin cooldown ALLOW period for " << sig->getName();
        state = CooldownState::ALLOW;
        break;
      }
    }
  }
  // Block events in the command list while in cooldown
  if (state == CooldownState::BLOCK) {
    for (auto& cmd : commands) {
      assert(cmd && cmd->getInput());
      std::shared_ptr<ControllerInput> sig = cmd->getInput();
      PLOG_VERBOSE << "Checking " << cmd->getName() << ", maps to " << ((sig) ? sig->getName() : "NULL");
      if (sig && sig->matches(event)) {
        blocked_command_values[cmd->getName()] = event.value;
        PLOG_DEBUG << "Blocked " << cmd->getName();
        return false;
      }
    }
  }
  return true;
}
