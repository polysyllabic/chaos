/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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

#include "modifier.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

const std::string Modifier::name = "modifier";

Controller* Modifier::controller;
ChaosEngine* Modifier::engine;
Touchpad* Modifier::touchpad;

std::unordered_map<std::string, std::shared_ptr<Modifier>> Modifier::mod_list;

void Modifier::initialize(const toml::table& config) {
  PLOG_VERBOSE << "Starting parent initialization for mod\n";
  
  timer.initialize();
  me = shared_from_this();
  pauseTimeAccumulator = 0;
  totalLifespan = -1;    // An erroneous value that if set should be positive

  description = config["description"].value_or("Description not available");
  PLOG_VERBOSE << "Mod description: " << description << std::endl;
  
  group = config["group"].value_or(name);
  PLOG_VERBOSE << "Mod group: " << group << std::endl;

  // test for special "ALL" value (string, not array) before trying to build the map
  std::optional<std::string_view> applies = config["appliesTo"].value<std::string_view>();
  if (applies && *applies == "ALL") {
    applies_to_all = true;
    PLOG_VERBOSE << "Applies to all commands.\n";
  }
  else {
    TOMLReader::addToVector<GameCommand>(config, "appliesTo", commands);
  }
  TOMLReader::addToVector<GameState>(config, "gamestate", gamestates);
  
  any_state = false;
  no_state = false;
  std::optional<std::string_view> state_test = config["gamestate_test"].value<std::string_view>();
  if (state_test) {
    if (*state_test == "any") {
      any_state = true;
    } else if (*state_test == "none") {
      no_state = true;
    } else if (*state_test != "all") {
      throw std::runtime_error("Invalid value for 'gamestate_test'. Must be one of 'all', 'any', or 'none'.");
    }
  }
}

// Do all static initialization. Mostly this involves constructing the list of mods from their
// TOML-file definitions.
void Modifier::buildModList(toml::table& config) {

  touchpad = new Touchpad(config);
  
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
	PLOG_ERROR << "Modifier missing required 'name' field: " << *modifier << "\n";
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
	ButtonToAxis(event, to_console->getID(), to_console->toMin(), controller->getJoystickMin(),
		     controller->getJoystickMax());
	break;
      case GPInputType::THREE_STATE:
	ButtonToAxis(event, to_console->getID(), to_console->toMin(), -1, 1);
	break;
      case GPInputType::HYBRID:
	event.id = to_console->getID(event.type);
	// we need to insert a new event for the axis
	new_event.id = to_console->getHybridAxis();
	new_event.type = TYPE_AXIS;
	new_event.value = event.value ? controller->getJoystickMax() : controller->getJoystickMin();
	controller->applyEvent(new_event);
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
	event.value = controller->joystickLimit(controller->getJoystickMax()*event.value);
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
	event.value = controller->joystickLimit(controller->getJoystickMax()*event.value);
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
	event.value = controller->joystickLimit(-event.value/to_console->getRemapSensitivity());
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
    event.value = controller->joystickLimit(-event.value);
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
    controller->applyEvent(new_event);
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
    controller->applyEvent(new_event);
  }
}

void Modifier::PrepTouchpad(const DeviceEvent& event) {
  if (! touchpad->isActive() && event.value == 0) {
    touchpad->clearActive();
    PLOG_VERBOSE << "Begin touchpad use\n";
  } else if (touchpad->isActive() && event.value) {
    // We're stopping touchpad use. Zero out all remapped axes in use.
    disableTPAxis(GPInput::TOUCHPAD_X);
    disableTPAxis(GPInput::TOUCHPAD_Y);
    disableTPAxis(GPInput::TOUCHPAD_X_2);
    disableTPAxis(GPInput::TOUCHPAD_Y_2);
  }
  touchpad->setActive(!event.value);
}

void Modifier::TouchpadToAxis(DeviceEvent& event, uint8_t axis) {
  if (touchpad->isActive()) {
    event.type = TYPE_AXIS; // likely unnecessary
    event.id = axis;
    bool in_condition = touchpad->getCondition() == GPInput::NONE ? false :
      controller->getState(touchpad->getCondition());
    event.value = controller->joystickLimit(touchpad->toAxis(event, in_condition));
  }
}

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
 

void Modifier::updateGamestates(const DeviceEvent& event) {
  // Traverse the list of defined gamestates and see if the event matches an on or off event. If it
  // does, we update the game state accordingly
  if (!gamestates.empty()) {
    for (auto& gs : gamestates) {
      if (controller->matches(event, gs->getOn())) {
	gs->setState(true);
      } else if (controller->matches(event, gs->getOff())) {
	gs->setState(false);
      }
    }
  }
}

// We can check whether all, any, or none of the gamestates are true
bool Modifier::inGamestate() {
  if (!gamestates.empty()) {
    if (any_state) {
      return std::any_of(gamestates.begin(), gamestates.end(), [](std::shared_ptr<GameState> g) {
	return g->getState(); });
    } else if (no_state) {
      return std::none_of(gamestates.begin(), gamestates.end(), [](std::shared_ptr<GameState> g) {
	return g->getState(); });
    }
    else {
      return std::all_of(gamestates.begin(), gamestates.end(), [](std::shared_ptr<GameState> g) {
	return g->getState(); });
    }
  }
  // If no game states are defined, always true
  return true;
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

void Modifier::setController(Controller* contr) {
  controller = contr;
}

void Modifier::setEngine(ChaosEngine* eng) {
  engine = eng;
}

