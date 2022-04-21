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
#include "tomlReader.hpp"

using namespace Chaos;

const std::string Modifier::name = "modifier";

//Touchpad* Modifier::touchpad;

std::unordered_map<std::string, std::shared_ptr<Modifier>> Modifier::mod_list;

void Modifier::initialize(toml::table& config) {
  
  timer.initialize();
  me = shared_from_this();
  pauseTimeAccumulator = 0;
  totalLifespan = -1;    // An erroneous value that if set should be positive
  lock_while_busy = true;
  
  description = config["description"].value_or("Description not available");
  
  group = config["group"].value_or("general");

#ifndef NDEBUG
  PLOG_DEBUG << "Common initialization for mod " << config["name"].value_or("NAME NOT FOUND") << std::endl;
  PLOG_DEBUG << " - description: " << description << std::endl;
  PLOG_DEBUG << " - type: " << config["type"].value_or("TYPE NOT FOUND") << std::endl;
  PLOG_DEBUG << " - group: " << group << std::endl;
#endif

  applies_to_all = TOMLReader::addToVectorOrAll<GameCommand>(config, "appliesTo", commands);

#ifndef NDEBUG
  PLOG_DEBUG << " - appliesTo: ";
  if (applies_to_all) {
    PLOG_DEBUG << "ALL" << std::endl;
  } else {
    for (auto& cmd : commands) {
      PLOG_DEBUG << cmd->getName() << " ";
    }
    PLOG_DEBUG << std::endl;
  }  
#endif
  
  TOMLReader::addToVector<GameCondition>(config, "condition", conditions);
  condition_test = TOMLReader::getConditionTest(config, "conditionTest");

#ifndef NDEBUG
  PLOG_DEBUG << " - condition: ";
  for (auto& cmd : commands) {
    PLOG_DEBUG << cmd->getName() << " ";
  }
  // TODO: implement automatic name reflection
  PLOG_DEBUG << " [condition test = ";
  switch (condition_test) {
    case ConditionCheck::ALL:
      PLOG_DEBUG << "ALL";
      break;
    case ConditionCheck::ANY:
      PLOG_DEBUG << "ANY";
      break;
    case ConditionCheck::NONE:
      PLOG_DEBUG << "NONE";
  }
  PLOG_DEBUG << "] " << std::endl;
#endif

  TOMLReader::addToVector<GameCondition>(config, "unless", unless_conditions);
  unless_test = TOMLReader::getConditionTest(config, "unlessTest");

#ifndef NDEBUG
  PLOG_DEBUG << " - unless condition: ";
  for (auto& cmd : commands) {
    PLOG_DEBUG << cmd->getName() << " ";
  }
  // TODO: implement automatic name reflection
  PLOG_DEBUG << " [Condition test = ";
  switch (unless_test) {
    case ConditionCheck::ALL:
      PLOG_DEBUG << "ALL";
      break;
    case ConditionCheck::ANY:
      PLOG_DEBUG << "ANY";
      break;
    case ConditionCheck::NONE:
      PLOG_DEBUG << "NONE";
  }
  PLOG_DEBUG << "] " << std::endl;
#endif

  // need a debug trace for these
  TOMLReader::buildSequence(config, "beginSequence", on_begin);

  TOMLReader::buildSequence(config, "finishSequence", on_finish);
  
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
	      PLOG_ERROR << "Modifier definition must be a table\n";
      	continue;
      }
      if (! modifier->contains("name")) {
	      PLOG_ERROR << "Modifier missing required 'name' field: " << *modifier << std::endl;
	      continue;
      }
      std::optional<std::string> mod_name = modifier->get("name")->value<std::string>();
      if (mod_list.count(*mod_name) == 1) {
	      PLOG_ERROR << "The modifier '" << *mod_name << " has already been defined\n";
	      continue;
      } 
      if (! modifier->contains("type")) {
	      PLOG_ERROR << "Modifier '" << *mod_name << " does not specify a type\n";
	      continue;
      }
      std::optional<std::string> mod_type = modifier->get("type")->value<std::string>();
      if (! hasType(*mod_type)) {
	      PLOG_ERROR << "Modifier '" << *mod_name << "' has unknown type '" << *mod_type << "'\n";
	      continue;
      }
      // Now we can finally create the mod. The constructors handle the rest of the configuration.
      try {
	      PLOG_VERBOSE << "Adding modifier '" << *mod_name << "' of type " << *mod_type << std::endl;
	      mod_list[*mod_name] = Modifier::create(*mod_type, *modifier);
      }
      catch (std::runtime_error e) {
	      PLOG_ERROR << "Modifier '" << *mod_name << "' not created: " << e.what() << std::endl;
      }
    }
  }
  if (mod_list.size() == 0) {
    PLOG_FATAL << "No modifiers were defined.\n";
    throw std::runtime_error("No modifiers defined");
  }
}

bool Modifier::remapEvent(DeviceEvent& event) {
  DeviceEvent new_event;

  // Get the object with information about the incomming signal
  std::shared_ptr<GamepadInput> from_controller = GamepadInput::get(event);  
  assert (from_controller);

  // If this signal isn't remapped, we're done
  if (from_controller->getRemap() == GPInput::NONE) {
    return true;
  }

  // Special handling of signals goes here
  // touchpad active requires special setup and tear-down
  if (from_controller->getSignal() == GPInput::TOUCHPAD_ACTIVE) {
    PrepTouchpad(event);
  }
  
  // After special signals are processed, any remaining remap value of NOTHING mean we should drop
  // this signal.
  if (from_controller->getRemap() == GPInput::NOTHING) {
    return false;
  }

  // Get the object for the signal we're sending to the console.
  std::shared_ptr<GamepadInput> to_console = GamepadInput::get(from_controller->getRemap());
  assert (to_console);
  
  if (from_controller->getType() == to_console->getType()) {
    // If remapping occurs within the same input type, we can change the id without worrying
    // about the value. If it's inverted, that will be done at the end of the routine.
    event.id = to_console->getID(event.type);
  } else {
    // remapping between input classes
    switch (from_controller->getType()) {
      
    case GPInputType::BUTTON:
      switch (to_console->getType()) {
      case GPInputType::AXIS:
	      ButtonToAxis(event, to_console->getID(), to_console->toMin(), JOYSTICK_MIN, JOYSTICK_MAX);
        	break;
      case GPInputType::THREE_STATE:
      	ButtonToAxis(event, to_console->getID(), to_console->toMin(), -1, 1);
	        break;
      case GPInputType::HYBRID:
      	event.id = to_console->getID(event.type);
	      // we need to insert a new event for the axis
      	new_event.id = to_console->getHybridAxis();
	      new_event.type = TYPE_AXIS;
      	new_event.value = event.value ? JOYSTICK_MAX : JOYSTICK_MIN;
	      Controller::instance().applyEvent(new_event);
	      break;
      default:
      	PLOG_WARNING << "Unhandled remap type from BUTTON: index = " << to_console->getIndex() << std::endl;
      }
      break;
    case GPInputType::HYBRID:
      // going from hybrid to button, we drop the axis component and simply change the button id
      if (to_console->getType() == GPInputType::BUTTON) {
	      if (event.type == TYPE_AXIS) {
      	  return false;
	      }
      	event.id = to_console->getID();
      }
      break;
    case GPInputType::THREE_STATE:

      switch (to_console->getType()) {
	
      case GPInputType::AXIS:
	      // Scale the axis value to min/max and change to the real ID
      	event.value = Controller::instance().joystickLimit(JOYSTICK_MAX*event.value);
	      event.id = to_console->getID();
      	break;
	
        case GPInputType::BUTTON:
      	{
	        event.type = TYPE_BUTTON;
      	  std::shared_ptr<GamepadInput> negremap = GamepadInput::get(from_controller->getNegRemap());
	        assert(negremap);

      	  AxisToButtons(event, to_console->getID(), negremap->getID(), 0);	
	        break;
      	}
      default:
	      PLOG_WARNING << "Unhandled remap type from THREE_STATE: index = " << to_console->getIndex() << std::endl;
      }
      break;

    case GPInputType::AXIS:
      switch (to_console->getType()) {
	
      case GPInputType::THREE_STATE:
      	// Scale the axis value to min/max and change to the real ID
	      event.id = to_console->getID();
      	event.value = Controller::instance().joystickLimit(JOYSTICK_MAX*event.value);
	      break;

      case GPInputType::BUTTON:
	    {
    	  std::shared_ptr<GamepadInput> negremap = GamepadInput::get(from_controller->getNegRemap());
	      assert(negremap);
    	  AxisToButtons(event, to_console->getID(), negremap->getID(), from_controller->getRemapThreshold());
	      break;
    	}
      default:
	      PLOG_WARNING << "Unhandled remap type from AXIS: " << to_console->getIndex() << std::endl;
      }
      break;
	
    case GPInputType::ACCELEROMETER:
      // In the original version, we injected a fake pipeline event with the axis and left the
      // accelerometer event unchanged. The means that a later mod that uses motion control will
      // receive the accelerometer and drop the injected event. Since we're altering the signal
      // before the mods touch it, that's not necessary. If both mods are active, the one applied
      // later will overwrite the remap table anyway.
      if (to_console->getType() == GPInputType::AXIS) {
	      assert(to_console->getRemapSensitivity());
      	event.id = to_console->getID();
	      event.value = Controller::instance().joystickLimit(-event.value/to_console->getRemapSensitivity());
      } else {
      	PLOG_WARNING << "Unhandled remap type from ACCELEROMETER: index = " << to_console->getIndex() << std::endl;
      }
      break;
    case GPInputType::GYROSCOPE:
      PLOG_WARNING << "Remapping the gyroscope is not currently supported\n";
      break;
    case GPInputType::TOUCHPAD:
      if (to_console->getType() == GPInputType::AXIS) {
	      TouchpadToAxis(event, to_console->getID());
      } else {
      	PLOG_WARNING << "Unhandled remap type from TOUCHPAD: index = " << to_console->getIndex() << std::endl;
      }
      break;
    }
  }
  // Apply any inversion. This step only makes sense for axes.
  if (event.type == TYPE_AXIS && from_controller->remapInverted()) {
    event.value = Controller::instance().joystickLimit(-event.value);
  }
  return true;
}

void Modifier::AxisToButtons(DeviceEvent& event, uint8_t positive, uint8_t negative, int threshold) {
  assert(event.type == TYPE_AXIS);
  DeviceEvent new_event;

  event.type = TYPE_BUTTON;	
  // Different buttons selected depending on the sign of the value
  if (event.value > threshold) {
    event.id = positive;
    event.value = 1;
  } else if (event.value < -threshold) {
    event.id = negative;
    event.value = 1;
  } else {
    // use this event to zero the positive button
    event.id = positive;
    event.value = 0;
    // insert another event to zero the negative button
    new_event.id = negative;
    new_event.value = 0;
    new_event.type = TYPE_BUTTON;
    Controller::instance().applyEvent(new_event);
  }
}

void Modifier::ButtonToAxis(DeviceEvent& event, uint8_t axis, bool to_min, int min_val, int max_val) {
  assert(event.type == TYPE_BUTTON);
  
  event.type = TYPE_AXIS;
  event.id = axis;
  if (event.value) {
    event.value = to_min ? min_val : max_val;
  }
}

// If the touchpad axis is currently being remapped, send a 0 signal to the remapped
// axis
void Modifier::disableTPAxis(GPInput tp_axis) {
  DeviceEvent new_event{0, 0, TYPE_AXIS, AXIS_RX};
  std::shared_ptr<GamepadInput> tp = GamepadInput::get(tp_axis);
  assert (tp && tp->getType() == GPInputType::TOUCHPAD);
  if (tp->getRemap() != GPInput::NONE) {
    new_event.id = tp->getRemap();
    Controller::instance().applyEvent(new_event);
  }
}

void Modifier::PrepTouchpad(const DeviceEvent& event) {
  if (! Touchpad::instance().isActive() && event.value == 0) {
    Touchpad::instance().clearActive();
    PLOG_VERBOSE << "Begin touchpad use\n";
  } else if (Touchpad::instance().isActive() && event.value) {
    // We're stopping touchpad use. Zero out all remapped axes in use.
    disableTPAxis(GPInput::TOUCHPAD_X);
    disableTPAxis(GPInput::TOUCHPAD_Y);
    disableTPAxis(GPInput::TOUCHPAD_X_2);
    disableTPAxis(GPInput::TOUCHPAD_Y_2);
  }
  Touchpad::instance().setActive(!event.value);
}

void Modifier::TouchpadToAxis(DeviceEvent& event, uint8_t axis) {
  if (Touchpad::instance().isActive()) {
    event.type = TYPE_AXIS; // likely unnecessary
    event.id = axis;
    
    bool in_condition = Touchpad::instance().inCondition();
    event.value = Controller::instance().joystickLimit(Touchpad::instance().toAxis(event, in_condition));
  }
}

// The chaos engine calls this routine directly, and we dispatch to the appropriate
// update routine here. update() is virtual.
void Modifier::_update(bool isPaused) {
  timer.update();
  if (isPaused) {
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

Json::Value Modifier::toJsonObject(const std::string& name) {
  Json::Value result;
  result["name"] = name;
  result["desc"] = getDescription();
  result["lifespan"] = lifespan();
  return result;
}

std::string Modifier::getModList() {
  Json::Value root;
  Json::Value& data = root["mods"];
  int i = 0;
  for (auto const& [key, val] : mod_list) {
    data[i++] = val->toJsonObject(key);
  }
	
  Json::StreamWriterBuilder builder;
	
  return Json::writeString(builder, root);
}

