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
#include <algorithm>
#include <cmath>

#include <plog/Log.h>

#include "GameCondition.hpp"
#include "GameCommandTable.hpp"
#include "GameCommand.hpp"
#include "ControllerInput.hpp"
#include "TOMLUtils.hpp"

using namespace Chaos;

GameCondition::GameCondition(const std::string& condition_name) :
  name{condition_name},
  threshold{1},
  threshold_type{ThresholdType::MAGNITUDE} {}

void GameCondition::addCondition(std::shared_ptr<GameCommand> command) {
  // Translate command to the index of the device event for fast comparison
  PLOG_DEBUG << "Adding condition for " << command->getName();
  while_conditions.push_back(command->getInput());
}

bool GameCondition::thresholdComparison(short value) {
  
  assert(threshold_type != ThresholdType::DISTANCE);
  switch (threshold_type) {
    case ThresholdType::GREATER:
      return value > threshold;
    case ThresholdType::GREATER_EQUAL:
      return value >= threshold;
    case ThresholdType::LESS:
      return value < threshold;
    case ThresholdType::LESS_EQUAL:
      return value <= threshold;
  }
  // default, check absolute value
  return std::abs(value) >= threshold;
}

void GameCondition::setThreshold(double proportion) {
  assert(proportion >= 0.0 && proportion <= 1.0);

  // Translate the threshold to an integer based on the signal of the first command in the commands list
  if (while_conditions.empty()) {
    PLOG_ERROR << "Can't set threshold until while_conditions is set";
    return;
  }
  std::shared_ptr<ControllerInput> signal = while_conditions.front();

  bool neg_extreme = (threshold_type == ThresholdType::LESS || threshold_type == ThresholdType::LESS_EQUAL);

  // Proportions only make sense for axes.
  switch (signal->getType()) {
    case ControllerSignalType::BUTTON:
      threshold = 1;
      break;
    case ControllerSignalType::THREE_STATE:
      // For buttons/tri-state, a proportion makes no sense
      threshold = (neg_extreme) ? -1 : 1;
      break;
    case ControllerSignalType::HYBRID:
      // If the threshold is 1, then look at the button; otherwise, we look at the fraction of the axis
      if (proportion == 1.0) {
        threshold = 1;
      }
      // falls through
    default:
      // Axis of some sort: calculate the threshold value as a proportion of the extreme.
      short extreme = (neg_extreme) ? signal->getMin(TYPE_AXIS) : signal->getMax(TYPE_AXIS);
      threshold = (short) ((double) extreme * proportion);
  }
  PLOG_DEBUG << "Threshold for " << name << " set to " << threshold;
}

// Test real-time condition of the controller
bool GameCondition::inCondition() {
  assert(!while_conditions.empty());

  if (threshold_type == ThresholdType::DISTANCE) {
    if (while_conditions.size() != 2) {
      return false;
    }    
    short x = while_conditions[0]->getState();
    short y = while_conditions[1]->getState();
    PLOG_DEBUG << "x = " << x << "; y = " << y << "x^2+y^2 = " << (x*x +y*y) << "; dist^2 = " << threshold * threshold;    
    return (x*x + y*y >= threshold * threshold);
  }

  return std::all_of(while_conditions.begin(), while_conditions.end(), [&](std::shared_ptr<ControllerInput> c) {
	  return thresholdComparison(c->getState()); });

}

bool GameCondition::pastThreshold(DeviceEvent& event) {
  return thresholdComparison(event.value);
}

bool GameCondition::matchEvent(DeviceEvent& event) {
  return (event.index() == while_conditions.front()->getIndex());
}
