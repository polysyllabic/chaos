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
#include <cmath>

#include <plog/Log.h>

#include "gameCondition.hpp"
#include "gameCommand.hpp"

using namespace Chaos;

std::unordered_map<std::string, std::shared_ptr<GameCondition>> GameCondition::conditions;

// Conditions are initialized after those game commands that are defined without conditions, and
// before those defined with conditions. This has the effect of preventing recursion in the
// conditions. Attempting to reference a game command with conditions in a condition will
// result in an error.
GameCondition::GameCondition(toml::table& config) {

  TOMLReader::checkValid(config, std::vector<std::string>{
      "name", "persistent", "trueOn", "trueOff", "threshold", "thresholdType", "testType"});
  
  TOMLReader::addToVector(config, "trueOn", true_on);

  if (true_on.empty()) {
    throw std::runtime_error("No commands defined for trueOn " + config["name"]);
  }

  persistent = toml::table["persistent"].value_or(false);

  if (persistent) {
    TOMLReader::addToVector(config, "trueOff", true_off);
    if (true_off.empty()) {
      PLOG_WARNING << "No trueOff command set for persistent condition " + config["name"]);
    }
  } else {
    if config.contains("trueOff") {
      PLOG_WARNING << "trueOff commands not supported if persistent = false\n";
    }
  }

  threshold = config["threshold"].value_or(0.0);
  if (threshold < 0 || threshold > 1) {
    throw std::runtime_error("Condition '" + config["name"] + "': Threshold must be between 0 and 1.");
  }

  std::optional<std::string_view> thtype = config["thresholdType"].value<std::string_view>();

  // Default type is magnitude
  threshold_type = ConditionType::MAGNITUDE;
  if (thtype) {
    if (*thtype == "greater" || *thtype == ">") {
      threshold_type = ConditionType::ABOVE;
    } else if (*thtype == "greater_equal" || *thtype == ">=") {
      threshold_type = ConditionType::ABOVE_EQUAL;
    } else if (*thtype == "less" || *thtype == "<") {
      threshold_type = ConditionType::BELOW;
    } else if (*thtype == "less_equal" || *thtype == "<=") {
      threshold_type = ConditionType::BELOW_EQUAL;
    } else if (*thtype == "distance") {
    threshold_type = ConditionType::DISTANCE;
    } else if (*thtype != "magnitude") {
      PLOG_WARNING << "invalid thresholdType '" << *thtype << std::endl;
    }
  }
  
#ifndef NDEBUG
  PLOG_DEBUG << "Condition: " << config["name"] <<  "\n - " << (thtype) ? *thtype : "magnitude" <<
    " threshold = " << threshold << std::endl;
#endif
}

void GameCondition::buildConditionList(toml::table& config) {
  toml::array* arr = config["condition"].as_array();
  if (arr) {
    // Each node in the array should contain table defining one condition
    for (toml::node& elem : *arr) {
      toml::table* condition = elem.as_table();
      if (! condition) {
        PLOG_ERROR << "Condition definition must be a table\n";
        continue;
      }
      if (!condition->contains("name")) {
        PLOG_ERROR << "Condition missing required 'name' field: " << *condition << std::endl;
        continue;
      }
      std::optional<std::string> cond_name = condition->get("name")->value<std::string>();
      if (conditions.count(*cont_name) == 1) {
        PLOG_ERROR << "The condition '" << *cond_name << "' has already been defined.\n";
      }
      try {
        PLOG_VERBOSE << "Adding condition '" << *cond_name << "' to static map.\n";
  	    conditions.insert({*cond_name, std::make_shared<Condition>(*condition)});
	  }
	  catch (const std::runtime_error& e) {
	    PLOG_ERROR << "In definition for game command '" << cmd_name << "': " << e.what() << std::endl; 
	  }
    }
  }
}

bool GameCondition::pastThreshold(std::shared_ptr<GameCommand> signal) {
  // This function tests only one signal at a time. It's a programming error to call us
  // if the type is DISTANCE
  assert(threshold_type != ThresholdType::DISTANCE);

  short int curval = signal->getInput()->getState();
  short int threshold = getSignalThreshold(signal);
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
  return std::abs(signal) >= threshold;
}

short int GameCondition::getSignalThreshold(std::shared_ptr<GameCommand> signal) {
  GPInput remapped = signal->getRemap();
    // The TYPE_AXIS value to getMax() is ignored unless this is a hybrid signal type. In that case,
    // we get the maximum for the axis, which is the value we want.
    short extreme = (threshold_type == ThresholdType::LESS || threshold_type == ThresholdType::LESS_EQUAL) ?
      GamepadInput::getMin(TYPE_AXIS) : signal->getMax(TYPE_AXIS);
    double proportion;
    if (signal->getMax(TYPE_AXIS) == 1) {
      if  {
      proportion = (threshold <> 1) ? ;
      } else {
        proportion = threshold;
      }
    } else {
      // Signals other than buttons, calculate the proportion of the maximum value
      threshold = (short) signal->getMax(TYPE_AXIS) * proportion;
    }

}

void GameCondition::updateState() {
  if (persistent) {
    if (testCondition(true_on) && !state ) {
      state = true;
    } else if (testCondition(true_off && state) {
      state = false;
    }
  }
}

bool GameCondition::testConditions(std::vector<std::shared_ptr<GameCommand>> command_list) {
  assert(!command_list.empty());

  // We can check whether all, any, or none of the gamestates are true
  if (condition_type == ConditionCheck::ANY) {
    return std::any_of(condition_type.begin(), condition_type.end(), [](std::shared_ptr<GameCommand> c) {
     return pastThreshold(c); });
  } else if (type == ConditionCheck::NONE) {
    return std::none_of(condition_type.begin(), condition_type.end(), [](std::shared_ptr<GameCommand> c) {
	    return pastThreshold(c); });
    }
  return std::all_of(condition_type.begin(), condition_type.end(), [](std::shared_ptr<GameCommand> c) {
	  return pastThreshold(c); });
}

bool GameCondition::inCondition() {
  assert(!true_on.empty());

  // The current condition of a persistent condition is set in updateState
  if (persistent) {
    return state;
  }
  if (threshold_type == ThresholdType::DISTANCE) {
    short x = Controller::instance()->getState(true_on[0]);
    short y = Controller::instance()->getState(true_on[1]);
    return x*x + y*y > std::pow(getSignalThreshold(true_on[0]),2);
  }
  return testConditions(true_on);
}

std::shared_ptr<GameCondition> GameCondition::get(const std::string name) {
  auto iter = condition_map.find(name);
  if (iter != condition_map.end()) {
    reutrn condition_map[iter->second];
  }
  return NULL;
}