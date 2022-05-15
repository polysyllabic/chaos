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

#include "gameCondition.hpp"
#include "gameCommand.hpp"
#include "controllerInput.hpp"
#include "tomlUtils.hpp"
using namespace Chaos;

// Conditions are initialized after those game commands that are defined without conditions, and
// before those defined with conditions. This has the effect of preventing recursion in the
// conditions. Attempting to reference a game command with conditions in a condition will
// result in an error.
GameCondition::GameCondition(toml::table& config) {
  assert(config.contains("name"));
  name = config["name"].value_or("");

  PLOG_VERBOSE << "Initializing game condition " << config["name"];
  
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "persistent", "trueOn", "falseOn", "threshold", "thresholdType", "testType"});
  
  TOMLUtils::addToVector(config, "trueOn", true_on);

  if (true_on.empty()) {
    throw std::runtime_error("No commands defined for trueOn");
  }

  persistent = config["persistent"].value_or(false);

  if (persistent) {
    TOMLUtils::addToVector(config, "falseOn", true_off);
    if (true_off.empty()) {
      PLOG_WARNING << "No falseOn command set for persistent condition.";
    }
  } else {
    if (config.contains("falseOn")) {
      PLOG_WARNING << "falseOn commands not supported if persistent = false";
    }
  }

  threshold = config["threshold"].value_or(0.0);
  if (threshold < 0 || threshold > 1) {
    throw std::runtime_error("Condition threshold must be between 0 and 1");
  }

  std::optional<std::string_view> thtype = config["thresholdType"].value<std::string_view>();

  // Default type is magnitude
  threshold_type = ThresholdType::MAGNITUDE;
  if (thtype) {
    if (*thtype == "greater" || *thtype == ">") {
      threshold_type = ThresholdType::GREATER;
    } else if (*thtype == "greater_equal" || *thtype == ">=") {
      threshold_type = ThresholdType::GREATER_EQUAL;
    } else if (*thtype == "less" || *thtype == "<") {
      threshold_type = ThresholdType::LESS;
    } else if (*thtype == "less_equal" || *thtype == "<=") {
      threshold_type = ThresholdType::LESS_EQUAL;
    } else if (*thtype == "distance") {
    threshold_type = ThresholdType::DISTANCE;
    } else if (*thtype != "magnitude") {
      PLOG_WARNING << "Invalid thresholdType '" << *thtype;
    }
  }

  PLOG_VERBOSE << "Condition: " << config["name"] <<  "; " << ((thtype) ? *thtype : "magnitude") <<
    " threshold = " << threshold;
}

bool GameCondition::pastThreshold(std::shared_ptr<GameCommand> command) {
  // This function tests only one signal at a time. It's a programming error to call us
  // if the type is DISTANCE
  assert(threshold_type != ThresholdType::DISTANCE);

  // getState looks at the active state of the remapped signal. We also need to check the threshold
  // for that remapped signal.
  short curval = command->getState();
  short threshold = getSignalThreshold(command);
  switch (threshold_type) {
    case ThresholdType::GREATER:
      return curval > threshold;
    case ThresholdType::GREATER_EQUAL:
      return curval >= threshold;
    case ThresholdType::LESS:
      return curval < threshold;
    case ThresholdType::LESS_EQUAL:
      return curval <= threshold;
  }
  // default, check absolute value
  return std::abs(curval) >= threshold;
}

short int GameCondition::getSignalThreshold(std::shared_ptr<GameCommand> signal) {
  // Check the threshold for the remapped control in case signals are swapped between signal classes
  std::shared_ptr<ControllerInput> remapped = signal->getRemappedSignal();
  short extreme = (threshold_type == ThresholdType::LESS || threshold_type == ThresholdType::LESS_EQUAL) ?
                  remapped->getMin(TYPE_AXIS) : remapped->getMax(TYPE_AXIS);

  // Proportions don't make sense for buttons or the dpad, so if we've remapped from an axis to one
  // of these and have a proportion < 1, we ignore that and just use the extreme value.
  ControllerSignalType t = remapped->getType();
  if (t == ControllerSignalType::BUTTON || t == ControllerSignalType::THREE_STATE) {
    return extreme;
  }
  // otherwise we calculate the threshold value as a proportion of the extreme.
  return (short) extreme * threshold;
}

void GameCondition::updateState() {
  if (persistent) {
    if (testConditions(true_on) && !state ) {
      state = true;
    } else if (testConditions(true_off) && state) {
      state = false;
    }
  }
}

bool GameCondition::testConditions(std::vector<std::shared_ptr<GameCommand>> command_list) {
  assert(!command_list.empty());

  if (threshold_type == ThresholdType::DISTANCE) {
    assert(command_list.size() == 2);
    short x = command_list[0]->getState();
    short y = command_list[1]->getState();
    return x*x + y*y > std::pow(getSignalThreshold(command_list[0]),2);
  }

  // We can check whether all, any, or none of the gamestates are true
  if (condition_type == ConditionCheck::ANY) {
    return std::any_of(command_list.begin(), command_list.end(), [&](std::shared_ptr<GameCommand> c) {
     return pastThreshold(c); });
  } else if (condition_type == ConditionCheck::NONE) {
    return std::none_of(command_list.begin(), command_list.end(), [&](std::shared_ptr<GameCommand> c) {
	    return pastThreshold(c); });
    }
  return std::all_of(command_list.begin(), command_list.end(), [&](std::shared_ptr<GameCommand> c) {
	  return pastThreshold(c); });
}

bool GameCondition::inCondition() {
  assert(!true_on.empty());

  // The current condition of a persistent condition is set in updateState
  if (persistent) {
    return state;
  }
  return testConditions(true_on);
}
