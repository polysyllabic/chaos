/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS
 * file at the top-level directory of this distribution for details of the
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
#include <vector>
#include <plog/Log.h>

#include "SequenceModifier.hpp"
#include "EngineInterface.hpp"
#include "ControllerInput.hpp"
#include "TOMLUtils.hpp"

using namespace Chaos;

const std::string SequenceModifier::mod_type = "sequence";

SequenceModifier::SequenceModifier(toml::table& config, EngineInterface* e) {

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "begin_sequence", "finish_sequence",
      "block_while_busy", "repeat_sequence", "trigger", "while", "while_operation",
      "start_delay", "cycle_delay", "unlisted"});

  initialize(config, e);
  
  repeat_sequence = e->createSequence(config, "repeat_sequence", false);

  std::optional<std::string> for_all = config["block_while_busy"].value<std::string>();
  // Allow "ALL" as a shortcut to avoid enumerating all signals
  lock_all = (for_all && *for_all == "ALL");
  if (! lock_all) {
    engine->addGameCommands(config, "block_while_busy", block_while);
  }

  engine->addGameCommands(config, "trigger", trigger);

  start_delay = config["start_delay"].value_or(0.0);
  repeat_delay = config["cycle_delay"].value_or(0.0);

  PLOG_VERBOSE << "- start_delay: " << start_delay << "; cycle_delay: " << repeat_delay;
}

void SequenceModifier::begin() {
  sequence_time = 0;
  sequence_step = 0;
  sequence_state = SequenceState::UNTRIGGERED;
}

void SequenceModifier::update() {
  // Skip if there's no repeated sequence
  if (!repeat_sequence || repeat_sequence->empty()) {
    return;
  }
  sequence_time += timer.dTime();
  
  switch (sequence_state) {
  case SequenceState::UNTRIGGERED:
    if (trigger.empty() && inCondition()) {
      // If there is no trigger, start sequence immediately upon the condition being true
      sequence_state = SequenceState::STARTING;
    }   
    return;
  case SequenceState::STARTING:
    // In the starting state, we wait for any initial delay before the sequence starts
    if (sequence_time >= start_delay) {
      PLOG_DEBUG << "Waited " << start_delay << " seconds to start sequence";
      sequence_state = SequenceState::IN_SEQUENCE;
      sequence_time = 0;
    }
    break;
case SequenceState::IN_SEQUENCE:
    // The sequence of actions here are not exclusive (other things can be happening while these
    // commands are in train. 
    if (repeat_sequence->sendParallel(sequence_time)) {
      PLOG_DEBUG << "Sent complete sequence";
      sequence_state = SequenceState::ENDING;
      sequence_time = 0;
    }
    break;
  case SequenceState::ENDING:
    // After the sequence, we wait for the delay amount before resetting the trigger
    if (sequence_time >= repeat_delay) {    
      PLOG_DEBUG << "Resetting trigger at sequence_time = " << sequence_time << "; repeat_delay = " << repeat_delay;
      sequence_state = SequenceState::UNTRIGGERED;
      sequence_time = 0;
      sequence_step = 0;
    }
  }
}

bool SequenceModifier::tweak(DeviceEvent& event) {

  if (sequence_state == SequenceState::UNTRIGGERED && !trigger.empty()) {
    // If any of the commands in the trigger come in, check for the condition, and if true,
    // start the sequence.
    for (auto& sig : trigger) {
      if (sig->getIndex() == event.index() && inCondition()) {
        sequence_state = SequenceState::STARTING;
        break;
      }
    }
  }
  // While in sequence, drop selected signals
  if (sequence_state == SequenceState::IN_SEQUENCE) {
    if (lock_all) {
      return false;
    } else {
      for (auto& cmd : block_while) {
        if (engine->eventMatches(event, cmd)) {
          PLOG_DEBUG << "blocked " << cmd->getName() << " value " << event.value;
          return false;
        }
      }
    }
  }
  return true;
}
