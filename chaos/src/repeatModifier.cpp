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
#include "repeatModifier.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

const std::string RepeatModifier::name = "repeat";

RepeatModifier::RepeatModifier(toml::table& config) {
  TOMLReader::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "appliesTo", "disableOnStart", "disableOnFinish",
      "timeOn", "timeOff", "repeat", "cycleDelay", "blockWhileBusy", "startSequence", "finishSequence"});
  initialize(config);

  if (commands.empty()) {
    throw std::runtime_error("No command(s) specified with 'appliesTo'");
  }
  if (commands.size() > 1) {
    PLOG_WARNING << "More than one command in 'appliesTo' -- extra commands will be ignored.";
  }
  signal = commands[0];
  
  time_on = config["timeOn"].value_or(0.0);
  time_off = config["timeOff"].value_or(0.0);
  repeat_count = config["repeat"].value_or(1);
  cycle_delay = config["cycleDelay"].value_or(0.0);
  force_on = config["forceOn"].value_or(false);

  TOMLReader::addToVector<GameCommand>(config, "blockWhileBusy", block_while);

#ifdef NDEBUG
  PLOG_DEBUG << " - timeOn: " << time_on << std::endl;
  PLOG_DEBUG << " - timeOff: " << time_off << std::endl;
  PLOG_DEBUG << " - repeat: " << repeat_count << std::endl;
  PLOG_DEBUG << " - cycleDelay: " << cycleDelay << std::endl;
  PLOG_DEBUG << " - forceOn: " << force_on ? "true" : "false" << std::endl;
  PLOG_DEBUG << " - blockWhileBusy: ";
  if (block_while.empty()) {
    PLOG_DEBUG << "NONE";
  } else {
    for (auto& cmd : block_while) {
      PLOG_DEBUG << GamepadInput::getName(cmd->getBinding()) << " ";
    }
  }
  PLOG_DEBUG << std:: endl;  
#endif
  
}

void RepeatModifier::begin() {
  if (disable_on_begin) {
    Controller::instance().setOff(signal);
  }
  press_time = 0;
  repeat_count = 0;
  // Additional signals that we set to arbitrary values
  setBeginSequence();
}

void RepeatModifier::update() {

  press_time += timer.dTime();
  if (repeat_count < num_cycles) {
    if (press_time > time_on && Controller::instance().getState(signal)) {
      Controller::instance().setOff(signal);
      press_time = 0;
      repeat_count++;
    } else if (press_time > time_off && !Controller::instance().getState(signal)) {
      Controller::instance().setOn(signal);
      press_time = 0;
    }
  } else if (press_time > cycle_delay) {
    repeat_count = 0;
    press_time = 0;
  }
}

bool RepeatModifier::tweak(DeviceEvent& event) {
  if (force_on) {
    if (Controller::instance().matches(event, signal)) {
      event.value = signal->getInput()->getMax((ButtonType) event.type);
    }
  }
  // Drop any events in the blockWhile list
  if (! block_while.empty()) {
    for (auto& cmd : block_while) {
      if (Controller::instance().matches(event, cmd)) {
	      return false;
      }
    }
  }
  return true;
}

void RepeatModifier::finish() {
  if (disable_on_finish) {
    Controller::instance().setOff(signal);
  }
  // Additional signals that we set to arbitrary values
  sendFinishSequence();
}
