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

#include "sequenceModifier.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

const std::string SequenceModifier::name = "sequence";

SequenceModifier::SequenceModifier(toml::table& config) {

  TOMLReader::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "disableOnStart", "startSequence",
      "blockWhileBusy", "repeatSequence", "disableOnFinish", "finishSequence",
      "trigger", "triggerThreshold", "triggerType", 
      "startDelay", "cycleDelay"});

  initialize(config);

  // The following keys are set in initialize() and don't need chcking here:
  // name, description, type, groups, appliesTo, disableOnStart, startSequence,
  // disableOnFinish, finishSequence
  
  TOMLReader::buildSequence(config, "repeatSequence", on_begin);

  lock_while_busy = TOMLReader::addToVectorOrAll<GameCommand>(config, "blockWhileBusy", block_while);

  start_delay = config["startDelay"].value_or(0.0);
  cycle_delay = config["cycleDelay"].value_or(0.0);

#ifndef NDEBUG
  PLOG_DEBUG << " - startDelay: " << start_delay << std::endl;
  PLOG_DEBUG << " - cycleDelay: " << cycle_delay << std::endl;
#endif
  
  TOMLReader::addToVector<GameCommand>(config, "trigger", triggers);
  distance_threshold = config["distanceThreshold"].value_or(false);
  double threshold_fraction = config["triggerThreshold"].value_or(0.0);
  if (! triggers.empty()) {
    
  }
}

void SequenceModifier::begin() {
  last_state = 0;
  sequence_time = 0;
  in_sequence = UNTRIGGERED;

  // zero out all signals that need it
  if (disable_on_begin && ! commands.empty()) {
    for (auto& cmd : commands) {
      Controller::instance().setOff(cmd);
    }
  }
  sendBeginSequence();
}

void SequenceModifier::update() {
  // Abort if there's no repeated sequence
  if (repeat_sequence.empty()) {
    return;
  }
  sequence_time += timer.dTime();
  
  switch (in_sequence) {
  case UNTRIGGERED:
    if (triggers.empty()) {
      // No trigger. Start immediately
      in_sequence = STARTING;
    } else {
      // check for a trigger condition
      if (magnitude_trigger) {

      } else {
        for (auto& t : triggers) {
          short current = Controller::instance().getState(t);
          if () {
            
          }

        }
      }
    }
  case STARTING:
    if (sequence_time >= start_delay) {
      in_sequence = IN_SEQUENCE;
      sequence_time = 0;
    }
  case IN_SEQUENCE:
    DeviceEvent event = repeat_sequence.getEvent(sequence_time);
    if (event.time == 0 && event.value == 0 && event.id == 255 && event.type == 255) {
      in_sequence = ENDING;
      sequence_time = 0;
    } else {
      if (Controller::instance().getState(event.id, event.type) == last_state) {
        // should the time in this event be set to zero?
        Controller::instance().applyEvent(event);
      }
    }
    break;
  case ENDING:
    if (sequence_time >= repeat_delay) {
      in_sequence = UNTRIGGERED;
      sequence_time = 0;
    }
  }
}

void SequenceModifier::finish() {
  // zero out all signals that need it
  if (disable_on_finish && ! commands.empty()) {
    for (auto& cmd : commands) {
      Controller::instance().setOff(cmd);
    }
  }
  sendFinishSequence();
}

bool SequenceModifier::tweak(DeviceEvent& event) {
  return !in_sequence;
}
