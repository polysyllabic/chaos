/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
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
#include <optional>
#include <string_view>

#include <json/json.h>
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include "game.hpp"
#include "tomlUtils.hpp"
#include "gameCommand.hpp"
#include "gameCondition.hpp"
#include "modifier.hpp"
#include "gameMenu.hpp"

using namespace Chaos;

bool Game::loadConfigFile(const std::string& configfile) {
  parse_errors = 0;
  toml::table configuration;
  try {
    configuration = toml::parse_file(configfile);
  }
  catch (const toml::parse_error& err) {
    PLOG_ERROR << "Parsing the configuration file failed:" << err << std::endl;
    ++parse_errors;
    return false;
  }
  
  // Check version tag to make sure we're looking at a chaos TOML file
  std::optional<std::string_view> version = configuration["chaos_toml"].value<std::string_view>();
  if (!version) {
    PLOG_FATAL << "Could not find the version id (chaos_toml). Are you sure this is the right TOML file?";
    ++parse_errors;
    return false;
  }
  
  // Get the basic game information. We send these to the interface. 
  name = configuration["game"].value_or<std::string>("Unknown Game");
  PLOG_INFO << "Playing " << name;

  active_modifiers = configuration["mod_defaults"]["active_modifiers"].value_or(3);
  if (active_modifiers < 1) {
    PLOG_WARNING << "You asked for " << active_modifiers << ". There must be at least one.";
    active_modifiers = 1;
  } else if (active_modifiers > 5) {
    PLOG_WARNING << "Having too many active modifiers may cause undesirable side-effects.";
  }
  PLOG_INFO << "Active modifiers: " << active_modifiers;

  time_per_modifier = configuration["mod_defaults"]["time_per_modifier"].value_or(180.0);
  if (time_per_modifier < 10) {
    PLOG_WARNING << "Minimum active time for modifiers is 10 seconds.";
    time_per_modifier = 10;
  }
  PLOG_INFO << "Time per modifier: " << time_per_modifier << " seconds";

  // If we're loading a game after initialization, empty the previous data
  if (command_map.size() > 0) {
    PLOG_VERBOSE << "Clearing existing GameCommand data.";
    command_map.clear();
  }

  // Process those commands that do not have conditions
  buildCommandMap(configuration, false);

  // Process all conditions
  buildConditionList(configuration);

  // Process those commands that have conditions
  // Splitting the command-map initialization this way makes it easy to detect commands
  // that try to reference undefined conditions.
  buildCommandMap(configuration, true);

  // Initialize defined sequences and static parameters for sequences
  buildSequenceList(configuration);

  // Initialize the menu system
  menu = GameMenu(configuration);

  // Initialize the remapping table
  initializeRemap(configuration);

  // Create the modifiers
  buildModList(configuration);
  return true;
}

void Game::buildCommandMap(toml::table& config, bool process_conditions) {  
  // We should have an array of tables. Each table defines one command binding.
  toml::array* arr = config["command"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      if (toml::table* command = elem.as_table()) {
         // If processing conditions, skip the commands definitions that do not contain conditions,
         // and vice-versa
        if (process_conditions ^ (command->contains("condition") || command->contains("unless"))) {
          continue;
        }
	      if (! command->contains("name")) {
	        PLOG_ERROR << "Command definition missing required 'name' field: " << command;          
          ++parse_errors;
          continue;
        }
        std::string cmd_name = command->get("name")->value_or("");
	      if (command_map.count(cmd_name) == 1) {
	        PLOG_ERROR << "Duplicate command definition for '" << cmd_name << "'. Later one will be ignored.";
          ++parse_errors;
          continue;
	      }
	      PLOG_VERBOSE << "Inserting '" << cmd_name << "' into game command list.";
	      try {
      	  command_map.insert({cmd_name, std::make_shared<GameCommand>(*command)});
	      }
	      catch (const std::runtime_error& e) {
          ++parse_errors;
	        PLOG_ERROR << "In definition for game command '" << cmd_name << "': " << e.what();
	      }
	    }
    }
  }
  if (process_conditions && command_map.size() == 0) {
    ++parse_errors;
    PLOG_ERROR << "No game commands were defined.";
  }
}

void Game::buildConditionList(toml::table& config) {
  toml::array* arr = config["condition"].as_array();
  if (arr) {
    // Each node in the array should contain table defining one condition
    for (toml::node& elem : *arr) {
      toml::table* condition = elem.as_table();
      if (! condition) {
          ++parse_errors;
        PLOG_ERROR << "Condition definition must be a table";
        continue;
      }
      if (!condition->contains("name")) {
          ++parse_errors;
        PLOG_ERROR << "Condition missing required 'name' field: " << condition;
        continue;
      }
      std::optional<std::string> cond_name = condition->get("name")->value<std::string>();
      if (condition_map.count(*cond_name) == 1) {
          ++parse_errors;
        PLOG_ERROR << "The condition '" << *cond_name << "' has already been defined.";
      }
      try {
        PLOG_VERBOSE << "Adding condition '" << *cond_name << "' to static map.";
  	    condition_map.insert({*cond_name, std::make_shared<GameCondition>(*condition)});
	    }
	    catch (const std::runtime_error& e) {
        ++parse_errors;
	      PLOG_ERROR << "In definition for condition '" << *cond_name << "': " << e.what(); 
	    }
    }
  }
}

void Game::buildSequenceList(toml::table& config) {
  // global parameters for sequences
  Sequence::setPressTime(config["controller"]["button_press_time"].value_or(0.0625));
  Sequence::setReleaseTime(config["controller"]["button_release_time"].value_or(0.0625));

  // initialize pre-defined sequences
  toml::array* arr = config["sequence"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      toml::table* seq = elem.as_table();
      if (!seq) {
        ++parse_errors;
        PLOG_ERROR << "Each sequence definition must be a table";
        continue;
      }
      std::optional<std::string> seq_name = (*seq)["name"].value<std::string>();
      if (! seq_name) {
        ++parse_errors;
        PLOG_ERROR << "Sequence definition missing required name field";
        continue;
      }
      if (! seq->contains("sequence")) {
        ++parse_errors;
        PLOG_ERROR << "Sequence definition missing required sequence field";
        continue;
      }
      try {
        PLOG_VERBOSE << "Adding pre-defined sequence '" << *seq_name << "'";
        std::shared_ptr<Sequence> s = std::make_shared<Sequence>(*seq, "sequence");
       	sequence_map.insert({*seq_name, s});
      }
      catch (const std::runtime_error& e) {
        ++parse_errors;
        PLOG_ERROR << "In definition for Sequence '" << *seq_name << "': " << e.what(); 
      }
    }
  } else {
    PLOG_WARNING << "No pre-defined sequences found.";
  }
}


// Handles the static initialization. We construct the list of mods from their TOML-file
// definitions.
void Game::buildModList(toml::table& config) {

  // Should have an array of tables, each one defining an individual modifier.
  toml::array* arr = config["modifier"].as_array();
  
  if (arr) {
    // Each node in the array should contain the definition for a modifier.
    // If there's a parsing error at this point, we skip that mod and keep going.
    for (toml::node& elem : *arr) {
      toml::table* modifier = elem.as_table();
      if (! modifier) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier definition must be a table";
      	continue;
      }
      if (! modifier->contains("name")) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier missing required 'name' field: " << modifier;
	      continue;
      }
      std::optional<std::string> mod_name = modifier->get("name")->value<std::string>();
      if (mod_map.count(*mod_name) == 1) {
        ++parse_errors;
	      PLOG_ERROR << "The modifier '" << *mod_name << " has already been defined";
	      continue;
      } 
      if (! modifier->contains("type")) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier '" << *mod_name << " does not specify a type";
	      continue;
      }
      std::optional<std::string> mod_type = modifier->get("type")->value<std::string>();
      if (! Modifier::hasType(*mod_type)) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier '" << *mod_name << "' has unknown type '" << *mod_type << "'";
	      continue;
      }
      // Now we can finally create the mod. The constructors handle the rest of the configuration.
      try {
	      PLOG_VERBOSE << "Adding modifier '" << *mod_name << "' of type " << *mod_type;
	      std::shared_ptr<Modifier> m = Modifier::create(*mod_type, *modifier);
        mod_map.insert({*mod_name, m});
      }
      catch (std::runtime_error e) {
        ++parse_errors;
	      PLOG_ERROR << "Modifier '" << *mod_name << "' not created: " << e.what();
      }
    }
  }
  if (mod_map.size() == 0) {
    ++parse_errors;
    PLOG_ERROR << "No modifiers were defined.";
    throw std::runtime_error("No modifiers defined");
  }
}

std::string Game::getModList() {
  Json::Value root;
  Json::Value& data = root["mods"];
  for (auto const& [key, val] : mod_map) {
    // Mods that are marked unlisted are not reported to the chatbot
    if (! val->isUnlisted()) {
      data.append(val->toJsonObject());
    }
  }
	
  Json::StreamWriterBuilder builder;	
  return Json::writeString(builder, root);
}

// This is the game-specific initialization
void Game::initializeRemap(const toml::table& config) {
  double scale = config["remapping"]["touchpad_scale"].value_or(1.0);
  if (scale == 0) {
    PLOG_ERROR << "Touchpad scale cannot be 0. Setting to 1";
    ++parse_errors;
    scale = 1;
  }
  remap_table.setTouchpadScale(scale);

  // Condition is optional; Flag an error if bad condition name but not if missing entirely
  std::optional<std::string> c = config["remapping"]["touchpad_condition"].value<std::string>();
  if (c) {
    std::shared_ptr<GameCondition> condition = getCondition(*c);
    if (condition) {
      remap_table.setTouchpadCondition(condition);
    } else {
      ++parse_errors;
      PLOG_ERROR << "The condition " << *c << " is not defined";
    }
  } 
  double scale_if = config["remapping"]["touchpad_scale_if"].value_or(1.0);
  if (scale_if == 0) {
    ++parse_errors;
    PLOG_ERROR << "Touchpad scale_if cannot be 0. Setting to 1";
    scale_if = 1;
  }
  remap_table.setTouchpadScaleIf(scale_if);

  int skew = config["remapping"]["touchpad_skew"].value_or(0);
  remap_table.setTouchpadSkew(skew);

  PLOG_DEBUG << "Touchpad scale = " << scale << "; condition = " << 
                remap_table.getTouchpadCondition()->getName() <<
                "; scale_if = " << scale_if << "; skew = " << skew;
}

/*
bool Game::remapEvent(DeviceEvent& event) {
  DeviceEvent new_event;

  // Get the object with information about the incomming signal
  std::shared_ptr<ControllerInput> from_controller = getInput(event);
  assert (from_controller);
  std::shared_ptr<ControllerInput> to_console = from_controller->getRemap();

  // If this signal isn't remapped, we're done
  if (to_console == nullptr) {
    return true;
  }

  // Touchpad active requires special setup and tear-down
  if (from_controller->getSignal() == ControllerSignal::TOUCHPAD_ACTIVE) {
    prepTouchpad(event);
  }

  // When mapping from any type of axis to buttons/hybrids and the choice of button is determined
  // by the sign of the value. If the negative remap is a button, the positive will be a button also,
  // so it's safe to check that.
  if (event.value < 0 && event.id == TYPE_AXIS && to_console->getButtonType() == TYPE_BUTTON) {
    to_console = from_controller->getNegRemap();
    assert (to_console);
  }

  // If we are remapping to NOTHING, we drop this signal.
  if (to_console->getSignal() == ControllerSignal::NOTHING) {
    PLOG_DEBUG << "dropping remapped event";
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
      controller.applyEvent(new_event);
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
      controller.applyEvent(new_event);
    }
  }

  // Update the event
  event.type = to_console->getButtonType();
  event.id = to_console->getID(event.type);
  event.value = from_controller->remapValue(to_console, event.value);
  return true;
}
*/