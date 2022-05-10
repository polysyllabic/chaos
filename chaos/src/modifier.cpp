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
#include "modifier.hpp"
#include "configuration.hpp"

using namespace Chaos;

const std::string Modifier::mod_type = "modifier";

std::unordered_map<std::string, std::shared_ptr<Modifier>> Modifier::mod_list;

void Modifier::initialize(toml::table& config) {
  timer.initialize();
  parent = nullptr;
  pauseTimeAccumulator = 0;
  totalLifespan = -1;    // An erroneous value that if set should be positive
  lock_while_busy = true;
  
  description = config["description"].value_or("Description not available");

  // The mod always belongs to the group of its type  
  groups.insert(mod_type);
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

#ifndef NDEBUG
  PLOG_VERBOSE << "Common initialization for mod " << config["name"].value_or("NAME NOT FOUND");
  PLOG_VERBOSE << " - description: " << description;
  PLOG_VERBOSE << " - type: " << config["type"].value_or("TYPE NOT FOUND");
  PLOG_VERBOSE << " - groups: " << groups;
#endif

  applies_to_all = Configuration::addToVectorOrAll<GameCommand>(config, "appliesTo", commands);

 
  Configuration::addToVector<GameCondition>(config, "condition", conditions);
  condition_test = Configuration::getConditionTest(config, "conditionTest");

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

  Configuration::addToVector<GameCondition>(config, "unless", unless_conditions);
  unless_test = Configuration::getConditionTest(config, "unlessTest");

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

  // need a debug trace for these
  Configuration::buildSequence(config, "beginSequence", on_begin);

  Configuration::buildSequence(config, "finishSequence", on_finish);
  
}

// Handles the static initialization. We construct the list of mods from their TOML-file
// definitions.
void Modifier::buildModList(toml::table& config) {

  // Should have an array of tables, each one defining an individual modifier.
  toml::array* arr = config["modifier"].as_array();
  
  if (arr) {
    // Each node in the array should contain the definition for a modifier.
    // If there's a parsing error at this point, we skip that mod and keep going.
    for (toml::node& elem : *arr) {
      toml::table* modifier = elem.as_table();
      if (! modifier) {
	      PLOG_ERROR << "Modifier definition must be a table";
      	continue;
      }
      if (! modifier->contains("name")) {
	      PLOG_ERROR << "Modifier missing required 'name' field: " << modifier;
	      continue;
      }
      std::optional<std::string> mod_name = modifier->get("name")->value<std::string>();
      if (mod_list.count(*mod_name) == 1) {
	      PLOG_ERROR << "The modifier '" << *mod_name << " has already been defined";
	      continue;
      } 
      if (! modifier->contains("type")) {
	      PLOG_ERROR << "Modifier '" << *mod_name << " does not specify a type";
	      continue;
      }
      std::optional<std::string> mod_type = modifier->get("type")->value<std::string>();
      if (! hasType(*mod_type)) {
	      PLOG_ERROR << "Modifier '" << *mod_name << "' has unknown type '" << *mod_type << "'";
	      continue;
      }
      // Now we can finally create the mod. The constructors handle the rest of the configuration.
      try {
	      PLOG_VERBOSE << "Adding modifier '" << *mod_name << "' of type " << *mod_type;
	      std::shared_ptr<Modifier> m = Modifier::create(*mod_type, *modifier);
        mod_list.insert({*mod_name, m});
}
      catch (std::runtime_error e) {
	      PLOG_ERROR << "Modifier '" << *mod_name << "' not created: " << e.what();
      }
    }
  }
  if (mod_list.size() == 0) {
    PLOG_FATAL << "No modifiers were defined.";
    throw std::runtime_error("No modifiers defined");
  }
}

bool Modifier::remapEvent(DeviceEvent& event) {
  DeviceEvent new_event;

  // Get the object with information about the incomming signal
  std::shared_ptr<ControllerInput> from_controller = ControllerInput::get(event);  
  assert (from_controller);
  ControllerSignal destination = from_controller->getRemap();

  // If this signal isn't remapped, we're done
  if (destination == ControllerSignal::NONE) {
    return true;
  }

  // Special handling of signals goes here
  // Touchpad active requires special setup and tear-down
  if (from_controller->getSignal() == ControllerSignal::TOUCHPAD_ACTIVE) {
    PrepTouchpad(event);
  }
  
  // Get the object for the signal we're sending to the console.
  std::shared_ptr<ControllerInput> to_console = ControllerInput::get(destination);
  assert (to_console);

  // When mapping from any type of axis to buttons/hybrids and the choice of button is determined
  // by the sign of the value. If the negative remap is a button, the positive will be a button also,
  // so it's safe to check that.
  if (event.value < 0 && event.id == TYPE_AXIS && to_console->getButtonType() == TYPE_BUTTON) {
    destination = from_controller->getNegRemap();
    to_console = ControllerInput::get(destination);
    assert (to_console);
  }

  // If we are remapping to NOTHING, we drop this signal.
  if (destination == ControllerSignal::NOTHING) {
    PLOG_VERBOSE << "dropping remapped event";
    return false;
  }

  // Now handle cases that require additional actions
  switch (from_controller->getType()) {
  case ControllerSignalType::HYBRID:
    // Going from hybrid to button, we drop the axis component
    if (to_console->getType() == ControllerSignalType::BUTTON && event.type == TYPE_AXIS) {
 	    return false;
    }
  case ControllerSignalType::BUTTON:
    // From button to hybrid, we need to insert a new event for the axis
    if (to_console->getType() == ControllerSignalType::HYBRID) {
   	  new_event.id = to_console->getHybridAxis();
      new_event.type = TYPE_AXIS;
   	  new_event.value = event.value ? JOYSTICK_MAX : JOYSTICK_MIN;
      Controller::instance().applyEvent(new_event);
    }
    break;
  case ControllerSignalType::AXIS:
    // From axis to button, we need a new event to zero out the second button when the value is
    // below the threshold. The first button will get a 0 value from the changed regular value
    // At this point, to_console has been set with the negative remap value.
    if (to_console->getButtonType() == TYPE_BUTTON && abs(event.value) < from_controller->getThreshold()) {
      new_event.id = to_console->getID();
      new_event.value = 0;
      new_event.type = TYPE_BUTTON;
      Controller::instance().applyEvent(new_event);
    }
  }

  // Update the event
  event.type = to_console->getButtonType();
  event.id = to_console->getID(event.type);
  event.value = from_controller->remapValue(event.value);
  return true;
}

// If the touchpad axis is currently being remapped, send a 0 signal to the remapped axis
void Modifier::disableTPAxis(ControllerSignal tp_axis) {
  DeviceEvent new_event{0, 0, TYPE_AXIS, AXIS_RX};
  std::shared_ptr<ControllerInput> tp = ControllerInput::get(tp_axis);
  assert (tp && tp->getType() == ControllerSignalType::TOUCHPAD);

  if (tp->getRemap() != ControllerSignal::NONE) {
    std::shared_ptr<ControllerInput> r = ControllerInput::get(tp->getRemap());
    assert(r);
    new_event.id = r->getID();
    Controller::instance().applyEvent(new_event);
  }
}

void Modifier::PrepTouchpad(const DeviceEvent& event) {
  if (! Touchpad::instance().isActive() && event.value == 0) {
    Touchpad::instance().clearActive();
    PLOG_DEBUG << "Begin touchpad use";
  } else if (Touchpad::instance().isActive() && event.value) {
    PLOG_DEBUG << "End touchpad use";
    // We're stopping touchpad use. Zero out all remapped axes in use.
    disableTPAxis(ControllerSignal::TOUCHPAD_X);
    disableTPAxis(ControllerSignal::TOUCHPAD_Y);
    disableTPAxis(ControllerSignal::TOUCHPAD_X_2);
    disableTPAxis(ControllerSignal::TOUCHPAD_Y_2);
  }
  Touchpad::instance().setActive(!event.value);
}

// The chaos engine calls this routine directly, and we dispatch to the appropriate
// update routine here.
void Modifier::_update(bool wasPaused) {
  timer.update();
  if (wasPaused) {
    pauseTimeAccumulator += timer.dTime();
  }
  update();
}

// Default implementations of virtual functions do nothing
void Modifier::begin() {}

void Modifier::update() {}

void Modifier::finish() {}

void Modifier::apply() {}

bool Modifier::tweak( DeviceEvent& event ) {
  return true;
}

void Modifier::sendBeginSequence() { 
  if (! on_begin.empty()) {
    in_sequence = lock_while_busy;
    on_begin.send();
    in_sequence = false;
  }
}

void Modifier::sendFinishSequence() { 
  if (! on_finish.empty()) {
    in_sequence = lock_while_busy;
    on_finish.send();
    in_sequence = false;
  }
}

// Traverse the list of persistent game conditions and see if the event matches an on or off
// event. If it does, we update the game state accordingly
void Modifier::updatePersistent() {
  if (!conditions.empty()) {
    for (auto& c : conditions) {
      c->updateState();
    }
  }
  if (!unless_conditions.empty()) {
    for (auto& c : unless_conditions) {
      c->updateState();
    }
  }
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

std::string Modifier::getModList() {
  Json::Value root;
  Json::Value& data = root["mods"];
//  int i = 0;
  for (auto const& [key, val] : mod_list) {
    //data[i++] = val->toJsonObject();

    data.append(val->toJsonObject());
  }
	
  Json::StreamWriterBuilder builder;	
  return Json::writeString(builder, root);
}

