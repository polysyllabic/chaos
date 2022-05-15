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

RepeatModifier::RepeatModifier(toml::table& config) {
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "appliesTo", "disableOnStart", "disableOnFinish", "forceOn",
      "timeOn", "timeOff", "repeat", "cycleDelay", "blockWhileBusy", "beginSequence", "finishSequence", "unlisted"});
  initialize(config);

  if (commands.empty()) {
    throw std::runtime_error("No command(s) specified with 'appliesTo'");
  }
  if (commands.size() > 1) {
    PLOG_WARNING << "More than one command in 'appliesTo' -- extra commands will be ignored.";
  }
  
  time_on = config["timeOn"].value_or(0.0);
  time_off = config["timeOff"].value_or(0.0);
  repeat_count = config["repeat"].value_or(1);
  cycle_delay = config["cycleDelay"].value_or(0.0);
  force_on = config["forceOn"].value_or(false);

  TOMLUtils::addToVector<GameCommand>(config, "blockWhileBusy", block_while);

#ifdef NDEBUG
  PLOG_DEBUG << " - timeOn: " << time_on << "; timeOff: " << time_off << "; cycleDelay: " << cycleDelay;
  PLOG_DEBUG << " - repeat: " << repeat_count << "; forceOn: " << (force_on ? "true" : "false");
  if (block_while.empty()) {
    PLOG_DEBUG << " - blockWhileBusy: NONE";
  } else {
    for (auto& cmd : block_while) {
      PLOG_DEBUG << " - blockWhileBusy:" << cmd->getName();
    }
  }
#endif
  
}

void RepeatModifier::begin() {
  press_time = 0;
  repeat_count = 0;
  // Signals that we set to arbitrary values
  sendBeginSequence();
}

void RepeatModifier::update() {

  press_time += timer.dTime();
  if (repeat_count < num_cycles) {
    if (press_time > time_on && commands[0]->getState()) {
      controller.setOff(commands[0]);
      press_time = 0;
      repeat_count++;
    } else if (press_time > time_off && ! commands[0]->getState()) {
      controller.setOn(commands[0]);
      press_time = 0;
    }
  } else if (press_time > cycle_delay) {
    repeat_count = 0;
    press_time = 0;
  }
}

bool RepeatModifier::tweak(DeviceEvent& event) {
  if (force_on) {
    if (controller.matches(event, commands[0])) {
      event.value = commands[0]->getInput()->getMax((ButtonType) event.type);
    }
  }
  // Drop any events in the blockWhile list
  if (! block_while.empty()) {
    for (auto& cmd : block_while) {
      if (controller.matches(event, cmd)) {
	      return false;
      }
    }
  }
  return true;
}

void RepeatModifier::finish() {
  sendFinishSequence();
}
