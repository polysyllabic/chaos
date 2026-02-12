/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS
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
      "name", "description", "type", "groups", "applies_to", "force_on", "time_on", "time_off",
      "repeat", "cycle_delay", "block_while_busy", "begin_sequence", "finish_sequence", "unlisted"});
  initialize(config, e);

  if (commands.empty()) {
    throw std::runtime_error("No command(s) specified with 'applies_to'");
  }
  
  time_on = config["time_on"].value_or(0.0);
  time_off = config["time_off"].value_or(0.0);
  num_cycles = config["repeat"].value_or(1);
  cycle_delay = config["cycle_delay"].value_or(0.0);

  if (config.contains("force_on")) {
    const toml::array* val_list = config.get("force_on")->as_array();
    if (! val_list || ! val_list->is_homogeneous(toml::node_type::integer)) {
      throw std::runtime_error("force_on must be an array of integers");
    }
    for (auto& elem : * val_list) {
      std::optional<int> val = elem.value<int>();
      assert(val);
      force_on.push_back(*val);      
    }
  }
  
  if (config.contains("force_off")) {
    const toml::array* val_list = config.get("force_off")->as_array();
    if (! val_list || ! val_list->is_homogeneous(toml::node_type::integer)) {
      throw std::runtime_error("force_off must be an array of integers");
    }
    for (auto& elem : * val_list) {
      std::optional<int> val = elem.value<int>();
      assert(val);
      force_off.push_back(*val);
    }
  }

  // Allow "ALL" as a shortcut to avoid enumerating all signals
  std::optional<std::string> for_all = config["block_while_busy"].value<std::string>();
  lock_all = (for_all && *for_all == "ALL");
  if (!lock_all) {
    engine->addGameCommands(config, "block_while_busy", block_while);
  }

  PLOG_DEBUG << " - time_on: " << time_on << "; time_off: " << time_off << "; cycle_delay: " << cycle_delay;
  PLOG_DEBUG << " - repeat: " << repeat_count;
  if (block_while.empty()) {
    PLOG_DEBUG << " - block_while_busy: NONE";
  } else {
    for (auto& cmd : block_while) {
      PLOG_DEBUG << " - block_while_busy:" << cmd->getName();
    }
  }
}

void RepeatModifier::begin() {
  press_time = 0;
  repeat_count = 0;
  is_on = false;
}

void RepeatModifier::update() {
  press_time += timer.dTime();
  if (repeat_count < num_cycles) {
    if (press_time > time_on && is_on) {
      int i = 0;
      for (auto cmd : commands) {
        if (i < force_off.size()) {
          PLOG_DEBUG << "Setting " << cmd->getName() << " to " << force_off[i];
          engine->setValue(cmd, force_off[i]);
        } else {
          PLOG_DEBUG << "Turning " << cmd->getName() << " off";
          engine->setOff(cmd);
        }
        is_on = false;
        i++;
      }
      press_time = 0;
      repeat_count++;
    } else if (press_time > time_off && !is_on) {
      int i = 0;
      for (auto cmd : commands) {
        if (i < force_on.size()) {
          PLOG_DEBUG << "Setting " << cmd->getName() << " to " << force_on[i];
          engine->setValue(cmd, force_on[i]);
        } else {
          PLOG_DEBUG << "Turning " << cmd->getName() << " on";
          engine->setOn(cmd);
        }
        is_on = true;
        i++;
      }
      press_time = 0;
    }
  } else if (press_time > cycle_delay) {
    PLOG_DEBUG << "resetting repeat cycle";
    repeat_count = 0;
    press_time = 0;
  }
}

bool RepeatModifier::tweak(DeviceEvent& event) {
  if (is_on) {
    if (! force_on.empty()) {
      int i = 0;      
      for (auto& cmd : commands) {
        if (engine->eventMatches(event, cmd) && i < force_on.size()) {
          event.value = force_on[i];
          PLOG_DEBUG << "force " << cmd->getName() << " to " << event.value;
          return true;
        }
        i++;
      }
    }
    // Drop all events unconditionally if we've set lock_all
    if (lock_all) {
      return false;
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
  }
  return true;
}
