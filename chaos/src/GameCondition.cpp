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

GameCondition::GameCondition(toml::table& config, GameCommandTable& commands) {
  assert(config.contains("name"));
  name = config["name"].value_or("");

  PLOG_VERBOSE << "Initializing game condition " << config["name"];
  
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "while", "while_not", "trigger_on", "clear_on", "threshold", "threshold_type",
      "test_type"});
  
  // commands.addToMap(config, "trigger_on", trigger_on);
  std::optional<std::string> cmd_name = config["trigger_on"].value<std::string>();
  trigger_on = (cmd_name) ? commands.getCommand(*cmd_name) : nullptr;

  // commands.addToMap(config, "clear_on", clear_on);
  cmd_name = config["clear_on"].value<std::string>();
  clear_on = (cmd_name) ? commands.getCommand(*cmd_name) : nullptr;

  commands.addToVector(config, "while", while_conditions);
  commands.addToVector(config, "while_not", while_not_conditions);

  if (! trigger_on) {
    if (while_conditions.empty() && while_not_conditions.empty()) {
      throw std::runtime_error("At least one of 'while', 'while_not' or 'trigger_on' must be defined.");
    }
    if (clear_on) {
      throw std::runtime_error("To use 'clear_on', you must also define 'trigger_on'.");
    }
  }

  threshold = config["threshold"].value_or(1.0);
  if (threshold < 0 || threshold > 1) {
    throw std::runtime_error("Condition threshold must be between 0 and 1");
  }

  std::optional<std::string_view> thtype = config["threshold_type"].value<std::string_view>();

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
      PLOG_WARNING << "Invalid threshold_type '" << *thtype;
    }
  }

  PLOG_DEBUG << "Condition: " << config["name"] <<  "; " << ((thtype) ? *thtype : "magnitude") <<
    " threshold = " << threshold;
  PLOG_DEBUG << "-- trigger on: " << ((trigger_on) ? trigger_on->getName() : "NONE") << " clear on: " <<
    ((clear_on) ? clear_on->getName() : "NONE");
}

void GameCondition::reset() {
  if (! trigger_on) {
    persistent_state = true;
  } else {
    persistent_state = false;
    // for (auto& it : trigger_on) {
    //   it.second = false;
    //}
  }
  //if (!clear_on.empty()) {
  //  for (auto& it : clear_on) {
  //    it.second = false;
  //  }
  //}
}

bool GameCondition::thresholdComparison(short value, short threshold) {
  // This function tests only one signal at a time. It's a programming error to call us
  // if the type is DISTANCE
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

bool GameCondition::pastThreshold(std::shared_ptr<GameCommand> command) {

  // By calling getState from the command, we find the active state of the remapped signal. We also
  // need to check the threshold for that remapped signal.
  short threshold = getSignalThreshold(command);
  short curval = command->getState();
  PLOG_VERBOSE << "threshold for " << command->getName() << ": " << threshold << "; state = " << curval;
  return thresholdComparison(curval, threshold);
}

short int GameCondition::getSignalThreshold(std::shared_ptr<GameCommand> command) {
  // Check the threshold for the remapped control in case signals are swapped between signal classes
  std::shared_ptr<ControllerInput> signal = command->getInput();
  return getSignalThreshold(signal);
}

short int GameCondition::getSignalThreshold(std::shared_ptr<ControllerInput> input) {
  short extreme = (threshold_type == ThresholdType::LESS || threshold_type == ThresholdType::LESS_EQUAL) ?
                  input->getMin(TYPE_AXIS) : input->getMax(TYPE_AXIS);

  // Proportions only make sense for axes. If we've remapped from an axis to one of these and have
  // a proportion < 1, we ignore that and just use the extreme value. For the hybrid triggers, we look at the button signal.
  ControllerSignalType t = input->getType();
  switch (t) {
    case ControllerSignalType::BUTTON:
    case ControllerSignalType::THREE_STATE:
      // For buttons/tri-state, a proportion makes no sense
      return extreme;
    case ControllerSignalType::HYBRID:
      // If the threshold is 1, then look at the button; otherwise, we look at the fraction of the axis
      if (threshold == 1) {
        return 1;
      }
      // falls through
    default:
      // Axis of some sort: calculate the threshold value as a proportion of the extreme.
      return (short) ((double) extreme * threshold);
  }
}

// This routine is called from _tweak, and so it does not need to be invoked by the child modifiers
// directly.
void GameCondition::updateState(DeviceEvent& event) {
  if (trigger_on) {
    if (event.type == trigger_on->getInput()->getButtonType() && 
        event.id == trigger_on->getInput()->getID()) {
      int threshold = getSignalThreshold(trigger_on->getInput());
      bool state = thresholdComparison(event.value, threshold); 
      if (clear_on) {
        if (state) {
          // If clear_on is set, we leave the trigger set until we get the clear condition
          persistent_state = true;
        }
      } else { 
        // If no clear_on is set, we reset the trigger as soon as it fails to meet the condition
        persistent_state = state;
      }
    }
  }
  if (clear_on) {
    if (event.type == clear_on->getInput()->getButtonType() && event.id == clear_on->getInput()->getID()) {
      int threshold = getSignalThreshold(clear_on->getInput());
      bool state = thresholdComparison(event.value, threshold);
      persistent_state = persistent_state && state;
    }
  }
}


// Test real-time condition of the controller
bool GameCondition::testConditions(std::vector<std::shared_ptr<GameCommand>> command_list, bool unless) {
  assert(!command_list.empty());

  if (threshold_type == ThresholdType::DISTANCE) {
    assert(command_list.size() == 2);
    short x = command_list[0]->getState();
    short y = command_list[1]->getState();
    int square_dist = std::pow(getSignalThreshold(command_list[0]),2);
    PLOG_DEBUG << "x = " << x << "; y = " << y << "x^2+y^2 = " << (x*x +y*y) << "; dist^2 = " << square_dist;
    bool past = x*x + y*y >= square_dist;
    return (past != unless);
  }

  if (unless) {
    // Return true if none of the conditions are past the threshold. In other words, if any condition
    // is past the threshold, the condition will be false.
    return std::none_of(command_list.begin(), command_list.end(), [&](std::shared_ptr<GameCommand> c) {
	    return pastThreshold(c); });
  }
  // return true if all the conditions are past the threshold
  return std::all_of(command_list.begin(), command_list.end(), [&](std::shared_ptr<GameCommand> c) {
	  return pastThreshold(c); });
}

bool GameCondition::inCondition() {
  bool while_state = true;
  bool unless_state = true;
  if (! while_conditions.empty()) {
    while_state = testConditions(while_conditions, false);
  } 
  //if (! while_not_conditions.empty()) {
  //  unless_state = testConditions(while_not_conditions, true);
  //}
  PLOG_VERBOSE << "persistent = " << persistent_state << "; while = " << while_state << "; unless = " << unless_state;
  return persistent_state && while_state && unless_state;
}
