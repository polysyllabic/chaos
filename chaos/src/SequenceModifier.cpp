/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
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
#include "TOMLUtils.hpp"

using namespace Chaos;

const std::string SequenceModifier::mod_type = "sequence";

SequenceModifier::SequenceModifier(toml::table& config, EngineInterface* e) {

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "beginSequence", "finishSequence",
      "blockWhileBusy", "repeatSequence", "condition",
      "start_delay", "cycle_delay", "unlisted"});

  initialize(config, e);
  
  repeat_sequence = e->createSequence(config, "repeatSequence", false);

  std::optional<std::string> for_all = config["blockWhileBusy"].value<std::string>();
  // Allow "ALL" as a shortcut to avoid enumerating all signals
  lock_all = (for_all && *for_all == "ALL");
  if (! lock_all) {
    engine->addGameCommands(config, "blockWhileBusy", block_while);
  }

  start_delay = dseconds(config["start_delay"].value_or(0.0));
  repeat_delay = dseconds(config["cycle_delay"].value_or(0.0));

  PLOG_VERBOSE << "- start_delay: " << start_delay.count() << "; cycle_delay: " << repeat_delay.count();
}

void SequenceModifier::begin() {
  sequence_time = dseconds::zero();
  sequence_step = 0;
  sequence_state = SequenceState::UNTRIGGERED;
}

void SequenceModifier::update() {
  // Abort if there's no repeated sequence
  if (repeat_sequence->empty()) {
    return;
  }
  sequence_time += timer.dTime();
  
  switch (sequence_state) {
  case SequenceState::UNTRIGGERED:
    // If there is no condition, this will also return true
    if (inCondition()) {
      // go straight into the sequence if no delay
      sequence_state = (start_delay == dseconds::zero()) ? SequenceState::IN_SEQUENCE :
                                            SequenceState::STARTING;
      PLOG_DEBUG << "Condition met after waiting " << sequence_time.count() << " seconds";
      sequence_time = dseconds::zero();
    }
  case SequenceState::STARTING:
    // In the starting state, we wait for any initial delay before the sequence starts
    if (sequence_time >= start_delay) {
      PLOG_DEBUG << "Waiting " << start_delay.count() << " seconds to start sequence";
      sequence_state = SequenceState::IN_SEQUENCE;
      sequence_time = dseconds::zero();
    }
    break;
case SequenceState::IN_SEQUENCE:
    // The sequence of actions here are not exclusive (other things can be happening while these
    // commands are in train. 
    if (repeat_sequence->sendParallel(sequence_time)) {
      PLOG_DEBUG << "Sent complete sequence";
      sequence_state = SequenceState::ENDING;
      sequence_time = dseconds::zero();
    }
    break;
  case SequenceState::ENDING:
    // After the sequence, we wait for the delay amount before resetting the trigger
    PLOG_DEBUG << "sequence_time=" << sequence_time.count() << "; repeat_delay=" << repeat_delay.count();
    if (sequence_time >= repeat_delay) {
      PLOG_DEBUG << "Resetting trigger";
      sequence_state = SequenceState::UNTRIGGERED;
      sequence_time = dseconds::zero();
      sequence_step = 0;
            
    }
  }
}

bool SequenceModifier::tweak(DeviceEvent& event) {

  if (sequence_state == SequenceState::UNTRIGGERED) {
    // If there is no condition, this will also return true
    if (inCondition()) {
      // go straight into the sequence if no delay
      sequence_state = (start_delay == dseconds::zero()) ? SequenceState::IN_SEQUENCE :
                                            SequenceState::STARTING;
      PLOG_DEBUG << "Condition met after waiting " << sequence_time.count() << " seconds";
      sequence_time = dseconds::zero();
    }
  }
  // While in sequence, drop selected signals
  if (sequence_state == SequenceState::IN_SEQUENCE) {
    if (lock_all) {
      return false;
    } else {
      for (auto& cmd : block_while) {
        if (engine->eventMatches(event, cmd)) {
          return false;
        }
      }
    }
  }
  return true;
}
