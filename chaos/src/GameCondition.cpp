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
#include <algorithm>
#include <cmath>

#include <plog/Log.h>

#include "GameCondition.hpp"
#include "GameCommand.hpp"
#include "ControllerInput.hpp"
#include "TOMLUtils.hpp"

using namespace Chaos;

GameCondition::GameCondition(const std::string& condition_name) :
  name{condition_name} {}

void GameCondition::addWhile(std::shared_ptr<GameCommand> command) {
  // Translate command to the index of the device event for fast comparison
  PLOG_DEBUG << "Adding while condition for " << command->getName();
  while_conditions.push_back(command->getInput());
}

void GameCondition::addClearOn(std::shared_ptr<GameCommand> command) {
  // Translate command to the index of the device event for fast comparison
  PLOG_DEBUG << "Adding clear_on condition for " << command->getName();
  clear_on.push_back(command->getInput());
}

bool GameCondition::thresholdComparison(short value, short thresh, ThresholdType type) {
  
  assert(type != ThresholdType::DISTANCE && type != ThresholdType::DISTANCE_BELOW);
  PLOG_DEBUG << "threshold = " << thresh << "; value = " << value;
  switch (type) {
    case ThresholdType::ABOVE:
      return std::abs(value) >= thresh;
    case ThresholdType::BELOW:
      return std::abs(value) < thresh;
    case ThresholdType::GREATER:
      return value >= thresh;
    case ThresholdType::LESS:
      return value < thresh;
  }
  PLOG_ERROR << "Internal error. Should not reach here";
  return false;
}

bool GameCondition::distanceComparison(short x, short y, short thresh, ThresholdType type) {
  assert(type == ThresholdType::DISTANCE || type == ThresholdType::DISTANCE_BELOW);

  int d = x*x + y*y;
  PLOG_DEBUG << "x = " << x << "; y = " << y << "x^2+y^2 = " << d << "; dist^2 = "
             << thresh * thresh;
  if (type == ThresholdType::DISTANCE) {
    return (d >= thresh * thresh);
  }
  return (d < thresh * thresh);
}

short GameCondition::calculateThreshold(double proportion, std::vector<std::shared_ptr<ControllerInput>> conditions) {
  assert(proportion >= -1.0 && proportion <= 1.0);
  short t = 1;
  std::shared_ptr<ControllerInput> signal;

  // Translate the threshold to an integer based on the signal of the first command in the commands list
  if (conditions.empty()) {
    PLOG_ERROR << "Internal error: while_conditions empty and set_on is null";  
    return 1;
  }
  signal = conditions.front();
  // Proportions only make sense for axes.
  switch (signal->getType()) {
    case ControllerSignalType::BUTTON:
    case ControllerSignalType::THREE_STATE:
      break;
    case ControllerSignalType::HYBRID:
      // If the threshold proportion is 1, then look at the button; otherwise, we look at the fraction of the axis
      if (proportion == 1.0) {
        break;
      }
      // falls through
    default:
      // Axis of some sort: calculate the threshold value as a proportion of the extreme.
      t = (short) ((double) signal->getMax(TYPE_AXIS) * proportion);
      t = fmin(fmax(t, JOYSTICK_MIN), JOYSTICK_MAX);
  }
  return t;
}

void GameCondition::setThreshold(double proportion) {
  threshold = calculateThreshold(proportion, while_conditions);
  PLOG_DEBUG << "Threshold for " << name << " set to " << threshold;
}

void GameCondition::setClearThreshold(double proportion) {
  clear_threshold = calculateThreshold(proportion, clear_on);
  PLOG_DEBUG << "Clear-on threshold for " << name << " set to " << clear_threshold;
}

bool GameCondition::testCondition(std::vector<std::shared_ptr<ControllerInput>> conditions, short thresh, ThresholdType type) {
  if (type == ThresholdType::DISTANCE || type == ThresholdType::DISTANCE_BELOW) {
    if (conditions.size() != 2) {
      return false;
    }
    short x = conditions[0]->getState(true);
    short y = conditions[1]->getState(true);
    return (distanceComparison(x, y, thresh, type));
  }

  return std::all_of(conditions.begin(), conditions.end(),
                     [&](std::shared_ptr<ControllerInput> c) {
	                      return thresholdComparison(c->getState(thresh != 1), thresh, type); 
                      });

}

bool GameCondition::inCondition() {
  bool rval;
  if (persistent_state) {
    // Check if we've hit the clear condition
    rval = testCondition(clear_on, clear_threshold, clear_threshold_type);
    if (rval) {
      persistent_state = false;
    }
    return persistent_state;
  }
  // Return the conditions based on the current state of the controller
  rval = testCondition(while_conditions, threshold, threshold_type);
  if (rval && !isTransient()) {
    persistent_state = true;
  }
  return rval;
}
