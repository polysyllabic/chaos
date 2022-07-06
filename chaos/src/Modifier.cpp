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
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <string_view>
#include <stdexcept>
#include <algorithm>

#include <plog/Log.h>

#include "config.hpp"
#include "enumerations.hpp"
#include "Modifier.hpp"
#include "TOMLUtils.hpp"
#include "EngineInterface.hpp"
#include "GameCondition.hpp"
#include "Sequence.hpp"

using namespace Chaos;


void Modifier::initialize(toml::table& config, EngineInterface* e) {
  engine = e;
  parent = nullptr;
  totalLifespan = -1;    // An erroneous value that if set should be positive
  lock_while_busy = true;
  allow_recursion = true;
  name = config["name"].value_or("NAME NOT FOUND");

  description = config["description"].value_or("Description not available");

  // The mod always belongs to the group of its type  
  groups.insert(getModType());
  if (config.contains("groups")) {
    const toml::array* group_list = config.get("groups")->as_array();
    if (group_list && group_list->is_homogeneous(toml::node_type::string)) {
      // We insert to an unordered set, so duplicates are automatically filtered
      for (auto& g : *group_list) {
        std::optional<std::string> gname = g.value<std::string>();
        assert(gname);
        groups.insert(*gname);
      }
    } else {
      PLOG_ERROR << "The group list must be an array of strings";
    }
  } 

  PLOG_VERBOSE << "Common initialization for mod " << name;
  PLOG_VERBOSE << " - description: " << description;
  PLOG_VERBOSE << " - type: " << config["type"].value_or("TYPE NOT FOUND");
  PLOG_VERBOSE << " - groups: " << groups;

  std::optional<std::string> for_all = config["appliesTo"].value<std::string>();
  applies_to_all = (for_all && *for_all == "ALL");
  if (! applies_to_all) {
    engine->addGameCommands(config, "appliesTo", commands);
  }
 
  engine->addGameConditions(config, "condition", conditions);
  condition_test = getConditionTest(config, "conditionTest");

#ifndef NDEBUG
  switch (condition_test) {
    case ConditionCheck::ALL:
      PLOG_VERBOSE << "Condition test = ALL";
      break;
    case ConditionCheck::ANY:
      PLOG_VERBOSE << "Condition test = ANY";
      break;
    case ConditionCheck::NONE:
      PLOG_VERBOSE << "Condition test = NONE";
  }
#endif

  engine->addGameConditions(config, "unless", unless_conditions);
  unless_test = getConditionTest(config, "unlessTest");

#ifndef NDEBUG
  switch (unless_test) {
    case ConditionCheck::ALL:
      PLOG_VERBOSE << "Condition test = ALL";
      break;
    case ConditionCheck::ANY:
      PLOG_VERBOSE << "Condition test = ANY";
      break;
    case ConditionCheck::NONE:
      PLOG_VERBOSE << "Condition test = NONE";
  }
#endif

  on_begin  = engine->createSequence(config, "beginSequence", false);
  on_finish = engine->createSequence(config, "finishSequence", false);  
}

// The chaos engine calls the underscored routines directly, giving us a chance to perform general
// actions for all mods. From there we dispatch to the appropriate child routine by invoking the
// virtual functions.
void Modifier::_begin() {
  timer.initialize();
  pauseTimeAccumulator = 0;
  for (auto& cond : conditions) {
    cond->reset();
  }
  for (auto& cond : unless_conditions) {
    cond->reset();
  }
  begin();
  sendBeginSequence();
}

// Default implementations of virtual functions do nothing
void Modifier::begin() {}

void Modifier::_update(bool wasPaused) {
  timer.update();
  if (wasPaused) {
    pauseTimeAccumulator += timer.dTime();
  }
  update();
}

void Modifier::update() {}

void Modifier::_finish() {
  sendFinishSequence();
  PLOG_DEBUG << "Calling virtual finish function for mod " << name;
  finish();
}

void Modifier::finish() {}

void Modifier::_apply() {
  apply();
}

void Modifier::apply() {}

bool Modifier::_tweak(DeviceEvent& event) {
  // Traverse the list of persistent game conditions and see if the event matches an on or off
  // event. If it does, we update the game state accordingly
  if (!conditions.empty()) {
    for (auto& c : conditions) {
      c->updateState(event);
    }
  }
  if (!unless_conditions.empty()) {
    for (auto& c : unless_conditions) {
      c->updateState(event);
    }
  }
  return tweak(event);
}

bool Modifier::tweak(DeviceEvent& event) {
   return true;
}

void Modifier::sendBeginSequence() { 
  PLOG_DEBUG << "Checking beginning sequence for " << name;
  if (! on_begin->empty()) {
    PLOG_DEBUG << "Sending beginning sequence for " << name;
    in_sequence = lock_while_busy;
    on_begin->send();
    in_sequence = false;
  }
}

void Modifier::sendFinishSequence() { 
  if (! on_finish->empty()) {
    PLOG_DEBUG << "Sending finishing sequence for " << name;
    in_sequence = lock_while_busy;
    on_finish->send();
    in_sequence = false;
  }
}


ConditionCheck Modifier::getConditionTest(const toml::table& config, const std::string& key) {
  std::optional<std::string_view> ttype = config[key].value<std::string_view>();

  // Default type is magnitude
  ConditionCheck rval = ConditionCheck::ANY;
  if (ttype) {
    if (*ttype == "any") {
      rval = ConditionCheck::ANY;
    } else if (*ttype == "none") {
      rval = ConditionCheck::NONE;
    } else if (*ttype != "all") {
      PLOG_WARNING << "Invalid ConditionTest '" << *ttype << "': using 'all' instead.";
    }
  }
  return rval;
}

bool Modifier::testConditions(std::vector<std::shared_ptr<GameCondition>> condition_list, ConditionCheck type) {
  assert(!condition_list.empty());

  // We can check whether all, any, or none of the gamestates are true
  if (type == ConditionCheck::ANY) {
    return std::any_of(condition_list.begin(), condition_list.end(), [](std::shared_ptr<GameCondition> c) {
	    return c->inCondition(); });
  } else if (type == ConditionCheck::NONE) {
    return std::none_of(condition_list.begin(), condition_list.end(), [](std::shared_ptr<GameCondition> c) {
	    return c->inCondition(); });
    }
  return std::all_of(condition_list.begin(), condition_list.end(), [](std::shared_ptr<GameCondition> c) {
	  return c->inCondition(); });
}

bool Modifier::inCondition() {
  // Having no conditions defined is equivalent to an always-true condition.
  if (conditions.empty()) {
    return true;
  }
  return testConditions(conditions, condition_test);
}

bool Modifier::inUnless() {
  // Having no unless conditions defined is equivalent to an always-false condition.
  if (unless_conditions.empty()) {
    return false;
  }
  return testConditions(unless_conditions, unless_test);
}

Json::Value Modifier::toJsonObject() {
  Json::Value result;
  result["name"] = getName();
  result["desc"] = getDescription();
  result["groups"] = getGroups();
  result["lifespan"] = lifespan();
  return result;
}

Json::Value Modifier::getGroups() {
  Json::Value group_array;
  for (auto const& g : groups) {
    group_array.append(g);
  }
  return group_array;
}

