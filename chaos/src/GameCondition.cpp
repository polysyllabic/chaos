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

// Conditions are initialized after those game commands that are defined without conditions, and
// before those defined with conditions. This has the effect of preventing recursion in the
// conditions. Attempting to reference a game command with conditions in a condition will
// result in an error.
GameCondition::GameCondition(toml::table& config, GameCommandTable& commands) {
  assert(config.contains("name"));
  name = config["name"].value_or("");

  PLOG_VERBOSE << "Initializing game condition " << config["name"];
  
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "persistent", "trueOn", "falseOn", "threshold", "thresholdType", "testType"});
  
  addToVector(config, "trueOn", true_on, commands);

  if (true_on.empty()) {
    throw std::runtime_error("No commands defined for trueOn");
  }

  persistent = config["persistent"].value_or(false);

  if (persistent) {
    addToVector(config, "falseOn", true_off, commands);
    if (true_off.empty()) {
      throw std::runtime_error("No falseOn command set for persistent condition.");
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

void GameCondition::addToVector(const toml::table& config, const std::string& key,
                                std::vector<std::shared_ptr<GameCommand>>& vec,
                                GameCommandTable& commands) {
      
  if (config.contains(key)) {
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error(key + " must be an array of strings");
   	}
	
    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
   	  std::shared_ptr<GameCommand> item = commands.getCommand(*cmd);
      if (item) {
        vec.push_back(item);
        PLOG_VERBOSE << "Added '" + *cmd + "' to the " + key + " vector.";
      } else {
        throw std::runtime_error("Unrecognized command: " + *cmd + " in " + key);
     	}
    }
  }      
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
