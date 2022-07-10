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
#include "RepeatModifier.hpp"
#include "TOMLUtils.hpp"
#include "GameCommand.hpp"
#include "ControllerInput.hpp"

using namespace Chaos;

const std::string RepeatModifier::mod_type = "repeat";

RepeatModifier::RepeatModifier(toml::table& config, EngineInterface* e) {
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "appliesTo", "disableOnStart", "disableOnFinish", "forceOn",
      "timeOn", "timeOff", "repeat", "cycleDelay", "blockWhileBusy", "beginSequence", "finishSequence", "unlisted"});
  initialize(config, e);

  if (commands.empty()) {
    throw std::runtime_error("No command(s) specified with 'appliesTo'");
  }
  if (commands.size() > 1) {
    PLOG_WARNING << "More than one command in 'appliesTo' -- extra commands will be ignored.";
  }
  
  time_on = dseconds(config["timeOn"].value_or(0.0));
  time_off = dseconds(config["timeOff"].value_or(0.0));
  repeat_count = config["repeat"].value_or(1);
  cycle_delay = dseconds(config["cycleDelay"].value_or(0.0));
  force_on = config["forceOn"].value_or(false);

    // Allow "ALL" as a shortcut to avoid enumerating all signals
  std::optional<std::string> for_all = config["blockWhileBusy"].value<std::string>();
  lock_all = (for_all && *for_all == "ALL");
  if (for_all && !lock_all) {
    engine->addGameCommands(config, "blockWhileBusy", block_while);
  }

  PLOG_DEBUG << " - timeOn: " << time_on.count() << "; timeOff: " << time_off.count() << "; cycleDelay: " << cycle_delay.count();
  PLOG_DEBUG << " - repeat: " << repeat_count << "; forceOn: " << (force_on ? "true" : "false");
  if (block_while.empty()) {
    PLOG_DEBUG << " - blockWhileBusy: NONE";
  } else {
    for (auto& cmd : block_while) {
      PLOG_DEBUG << " - blockWhileBusy:" << cmd->getName();
    }
  }
}

void RepeatModifier::begin() {
  press_time = dseconds::zero();
  repeat_count = 0;
}

void RepeatModifier::update() {
  press_time += timer.dTime();
  if (repeat_count < num_cycles) {
    if (press_time > time_on) {
      for (auto cmd : commands) {
        PLOG_DEBUG << "Turning " << cmd->getName() << " off";
        engine->setOff(cmd);
      }
      press_time = dseconds::zero();
      repeat_count++;
    } else if (press_time > time_off) {
      for (auto cmd : commands) {
        PLOG_DEBUG << "Turning " << cmd->getName() << " on";
        engine->setOn(cmd);
      }
      press_time = dseconds::zero();
    }
  } else if (press_time > cycle_delay) {
    PLOG_DEBUG << "resetting repeat cycle";
    repeat_count = 0;
    press_time = dseconds::zero();
  }
}

bool RepeatModifier::tweak(DeviceEvent& event) {
  if (force_on) {
    for (auto& cmd : commands) {
      if (engine->eventMatches(event, cmd)) {
        event.value = cmd->getInput()->getMax((ButtonType) event.type);
        PLOG_DEBUG << "force " << cmd->getName() << " to " << event.value;
      }
    }
  }
  // Drop any events in the blockWhile list
  if (! block_while.empty()) {
    for (auto& cmd : block_while) {
      if (engine->eventMatches(event, cmd)) {
        PLOG_DEBUG << "blocking " << cmd->getName();
	      return false;
      }
    }
  }
  return true;
}
