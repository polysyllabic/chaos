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
#include <algorithm>
#include <plog/Log.h>
#include "ConditionTrigger.hpp"
#include "GameCommand.hpp"
#include "ControllerInput.hpp"

using namespace Chaos;

ConditionTrigger::ConditionTrigger(const std::string& condition_name) : name{condition_name} {}

void ConditionTrigger::reset() {
  trigger_state = false;
}

// This routine should be called from _tweak, so child modifiers only need to check isTriggered.
void ConditionTrigger::updateState(DeviceEvent& event) {
  if (clear_on && trigger_state) {
    // Once triggered, only an explicit clear_on match will reset
    if (clear_on->matchEvent(event) && clear_on->pastThreshold(event)) {
      PLOG_DEBUG << "clear_on condition met: " << (int) event.type << "." << (int) event.id << ": " << event.value;
      trigger_state = false;
    } 
    return;
  }
  if (trigger_on && trigger_on->matchEvent(event)) {
    bool state = trigger_on->pastThreshold(event);
    bool in_while = true;
    if (!while_conditions.empty()) {
      in_while = std::all_of(while_conditions.begin(), while_conditions.end(), [&](std::shared_ptr<GameCondition> c) {
	      return c->inCondition(); });
    }
    bool in_unless = false;
    if (!unless_conditions.empty()) {
      in_unless = std::all_of(unless_conditions.begin(), unless_conditions.end(), [&](std::shared_ptr<GameCondition> c) {
	      return c->inCondition(); });
    }
    // If a clear_on exists, we only get here if trigger_state is false, so if the next statement results in a false,
    // we won't be resetting a persistent trigger.
    bool temp = state && in_while && !in_unless;
    if (temp != trigger_state) {
      PLOG_DEBUG << "Trigger " << name << " changed to " << temp;
    }
    trigger_state = temp;
  }
}

void ConditionTrigger::addWhileCondition(std::shared_ptr<GameCondition> condition) {
  PLOG_DEBUG << "Adding while condition for " << condition->getName();
  while_conditions.push_back(condition);
}

void ConditionTrigger::addUnlessCondition(std::shared_ptr<GameCondition> condition) {
  PLOG_DEBUG << "Adding unless condition for " << condition->getName();
  unless_conditions.push_back(condition);
}

void ConditionTrigger::setTriggerOn(std::shared_ptr<GameCondition> condition) {
  trigger_on = condition;
}

void ConditionTrigger::setClearOn(std::shared_ptr<GameCondition> condition) {
  clear_on = condition;
}
