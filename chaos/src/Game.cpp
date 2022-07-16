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
#include <memory>
#include <optional>
#include <string_view>

#include <json/json.h>
#include <plog/Log.h>

#include "Game.hpp"
#include "GameCommand.hpp"
#include "GameCondition.hpp"
#include "Modifier.hpp"
#include "GameMenu.hpp"
#include "EngineInterface.hpp"
#include "TOMLUtils.hpp"
#include "MenuItem.hpp"
#include "ControllerInput.hpp"
#include "SequenceTable.hpp"

using namespace Chaos;

Game::Game(Controller& c) : controller{c}, signal_table{c} {
  sequences = std::make_shared<SequenceTable>();
}

bool Game::loadConfigFile(const std::string& configfile, EngineInterface* engine) {
  parse_errors = 0;
  parse_warnings = 0;
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
    ++parse_warnings;
    active_modifiers = 1;
  } else if (active_modifiers > 5) {
    PLOG_WARNING << "Having too many active modifiers may cause undesirable side-effects.";
  }
  PLOG_INFO << "Active modifiers: " << active_modifiers;

  time_per_modifier = configuration["mod_defaults"]["time_per_modifier"].value_or(180.0);

  if (time_per_modifier < 10.0) {
    PLOG_WARNING << "Minimum active time for modifiers is 10 seconds.";
    time_per_modifier = 10.0;
    ++parse_warnings;
  }
  PLOG_INFO << "Time per modifier: " << time_per_modifier << " seconds";

  // Initialize the controller input table
  parse_errors += signal_table.initializeInputs(configuration);

  // Process the game-command definitions
  buildCommandList(configuration);

  // Process the conditions
  buildConditionList(configuration);

  // Initialize defined sequences and static parameters for sequences
  buildSequenceList(configuration);

  use_menu = configuration["use_menu"].value_or(true);

  if (use_menu) {
    // Initialize the menu system's general settings
    makeMenu(configuration);
  }

  // Create the modifiers
  parse_errors += modifiers.buildModList(configuration, engine, use_menu);
  PLOG_INFO << "Loaded configuration file for " << name << " with " << parse_errors << " errors.";
  return true;
}

void Game::makeMenu(toml::table& config) {
  PLOG_VERBOSE << "Creating menu items menu";
  menu.setDefinedSequences(sequences);

  toml::table* menu_list = config["menu"].as_table();
  if (! config.contains("menu")) {
    PLOG_ERROR << "No 'menu' table found in configuration file";
    ++parse_errors;
    return;
  }

  bool remember_last = (*menu_list)["remember_last"].value_or(false);
  PLOG_VERBOSE << "menu remember_last = " << remember_last;
  menu.setRememberLast(remember_last);

  bool hide_guarded = (*menu_list)["hide_guarded"].value_or(false);
  PLOG_VERBOSE << "menu hide_guarded = " << hide_guarded;
  menu.setHideGuarded(hide_guarded);

  if (! menu_list->contains("layout")) {
    PLOG_ERROR << "No menu layout found!";
    ++parse_errors;
    return;
  }

  // Menu layout
  toml::array* arr = (*menu_list)["layout"].as_array();
  if (! arr) {
    ++parse_errors;
    PLOG_ERROR << "Menu layout must be in an array.";
  }
  for (toml::node& elem : *arr) {
    toml::table* m = elem.as_table();
    if (m) {
      addMenuItem(*m);
    } else {
      ++parse_errors;
      PLOG_ERROR << "Each menu-item definition must be a table.";
    }
  }
}

void Game::addMenuItem(toml::table& config) {

  TOMLUtils::checkValid(config, std::vector<std::string>{"name", "type", "offset", "tab", "confirm",
                        "initialState", "parent", "guard", "hidden", "counter", "counterAction"});

  std::optional<std::string> entry_name = config["name"].value<std::string>();
  if (! entry_name) {
    ++parse_errors;
    PLOG_ERROR << "Menu item missing required name field";
    return ;
  }

  std::optional<std::string> menu_type = config["type"].value<std::string>();
  if (! menu_type) {
    ++parse_errors;
    PLOG_ERROR << "Menu item definition lacks required 'type' parameter.";
    return;
  }

  if (*menu_type != "option" && *menu_type != "select" && *menu_type != "menu") {
    ++parse_errors;
    PLOG_ERROR << "Menu type '" << *menu_type << "' not recognized.";
    return;
  }

  bool opt = (*menu_type == "option" || *menu_type == "select");
  bool sel = (*menu_type == "select" || *menu_type == "menu");

  PLOG_VERBOSE << "Adding menu item '" << *entry_name << "' of type " << *menu_type;

  if (! config.contains("offset")) {
    PLOG_WARNING << "Menu item '" << config["name"] << "' missing offset. Set to 0.";
  }
  short off = config["offset"].value_or(0);
  short tab = config["tab"].value_or(0);
  short initial = config["initialState"].value_or(0);
  bool hide = config["hidden"].value_or(false);
  bool confirm = config["confirm"].value_or(false);

  PLOG_VERBOSE << "-- offset = " << off << "; tab = " << tab <<
      "; initial_state = " << initial << "; hidden = " << hide <<
      "; confirm = " << confirm;

  std::shared_ptr<MenuItem> parent;
  try {
    PLOG_VERBOSE << "checking parent";
    parent = menu.getMenuItem(config, "parent");
  } catch (const std::runtime_error& e) {
    ++parse_errors;
    PLOG_ERROR << e.what();
  }

  std::shared_ptr<MenuItem> guard;
  try {
    PLOG_VERBOSE << "checking guard";
    guard = menu.getMenuItem(config, "guard");
  } catch (const std::runtime_error& e) {
    ++parse_errors;
    PLOG_ERROR << e.what();
  }

  std::shared_ptr<MenuItem> counter;
  try {
    PLOG_VERBOSE << "checking sibling";
    counter = menu.getMenuItem(config, "counter");
  } catch (const std::runtime_error& e) {
    ++parse_errors;
    PLOG_ERROR << e.what();
  }

  // CounterAction ignored for now
/* 
  CounterAction action = CounterAction::NONE;
  std::optional<std::string> action_name = config["counterAction"].value<std::string>();  
  if (action_name) {
    if (*action_name == "reveal") {
      action = CounterAction::REVEAL;
    } else if (*action_name == "zeroReset") {
      action = CounterAction::ZERO_RESET;
    } else if (*action_name != "none") {
      throw std::runtime_error("Unknown counterAction type: " + *action_name);
    }
  } */

  PLOG_VERBOSE << "constructing menu item";
  std::shared_ptr<MenuItem> m = std::make_shared<MenuItem>(menu, *entry_name,
      off, tab, initial, hide, opt, sel, confirm, parent, guard, counter);
  
  if (! menu.insertMenuItem(*entry_name, m)) {
    ++parse_errors;
    PLOG_ERROR << "Duplicate menu item: " << *entry_name; 
  }
}

void Game::buildCommandList(toml::table& config) {

  // Wipe everything if we're reloading a file
  if (game_commands.size() > 0) {
    PLOG_VERBOSE << "Clearing existing GameCommand data.";
    game_commands.clear();
  }

  // We should have an array of tables. Each table defines one command binding.
  toml::array* arr = config["command"].as_array();
  if (arr) {
    for (toml::node& elem : *arr) {
      if (toml::table* command = elem.as_table()) {
        std::optional<std::string> cmd_name = (*command)["name"].value<std::string>();
	      if (! cmd_name) {
	        PLOG_ERROR << "Command definition missing required 'name' field: " << command;          
          ++parse_errors;
          continue;
        }
        std::optional<std::string> item = (*command)["binding"].value<std::string>();
        if (!item) {
          PLOG_ERROR << "Missing command binding.";
          ++parse_errors;
          continue;
        }
        std::shared_ptr<ControllerInput> bind = signal_table.getInput(*item);
        if (!bind) {
          PLOG_ERROR << "Specified command binding does not exist.";
          ++parse_errors;
          continue;
        }
	      PLOG_VERBOSE << "Inserting '" << *cmd_name << "' into game command list.";
	      try {
      	  auto [it, result] = game_commands.try_emplace(*cmd_name, std::make_shared<GameCommand>(*cmd_name, bind));
          if (!result) {
            ++parse_errors;
            PLOG_ERROR << "Duplicate command definition ignored: " << *cmd_name;
          }
	      }
	      catch (const std::runtime_error& e) {
          ++parse_errors;
	        PLOG_ERROR << "In definition for game command '" << *cmd_name << "': " << e.what();
	      }
	    }
    }
  } else {
    ++parse_errors;
    PLOG_ERROR << "Command definitions should be an array of tables";
  }

  if (game_commands.size() == 0) {
    ++parse_errors;
    PLOG_ERROR << "No game commands were defined";
  }
}

void Game::buildConditionList(toml::table& config) {
  
  // If we're loading a game after initialization, empty the previous data
  if (game_conditions.size() > 0) {
    PLOG_VERBOSE << "Clearing existing GameCondition data.";
    game_conditions.clear();
  }

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
      try {
        PLOG_VERBOSE << "Adding condition '" << *cond_name << "' to map";
        auto c = makeCondition(*condition);
        if (c) {
  	      auto [it, result] = game_conditions.try_emplace(*cond_name, c);
          if (! result) {
            ++parse_errors;
            PLOG_ERROR << "Duplicate condition name: " << *cond_name;
          }
        }
	    }
	    catch (const std::runtime_error& e) {
        ++parse_errors;
	      PLOG_ERROR << "In definition for condition '" << *cond_name << "': " << e.what(); 
	    }
    }
  }
}

std::shared_ptr<GameCondition> Game::makeCondition(toml::table& config) {
  assert(config.contains("name"));

  std::optional<std::string> condition_name = config["name"].value<std::string>();

  PLOG_VERBOSE << "Initializing game condition " << *condition_name;
  
  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "while", "set_on", "clear_on", "threshold", "threshold_type"});

  std::shared_ptr<GameCondition> ret = std::make_shared<GameCondition>(*condition_name);

  if (config.contains("while")) {
    if (config.contains("set_on") || config.contains("clear_on")) {
      ++parse_errors;
      PLOG_ERROR << "A condition must contain either a while or a set_on/clear_on pair";
      return nullptr;
    }
    const toml::array* cmd_list = config.get("while")->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      ++parse_errors;
      PLOG_ERROR << "'while' must be an array of strings";
      return nullptr;
    }

    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
      std::shared_ptr<GameCommand> item = getCommand(*cmd);
      if (item) {
        ret->addCondition(item);
        PLOG_VERBOSE << "Added '" + *cmd + "' to the while vector.";
      } else {
        ++parse_errors;
        PLOG_ERROR << "Unrecognized command '" << *cmd << "' in while list";
        return nullptr;
      }
    }
    // case of empty while list
    if (!ret->isTransient()) {
      ++parse_errors;
      PLOG_ERROR << "No commands in while list";
      return nullptr;
    }
  } else {
    // If we get here, we're constructing a persistent condition
  if (!config.contains("set_on") || !config.contains("clear_on")) {
    ++parse_errors;
    PLOG_ERROR << "A persistent condition must contain both set_on and clear_on parameters";
    return nullptr;
  }

  std::shared_ptr<GameCommand> cmd = getCommand(config, "set_on", true);
  if (! cmd) {
    return nullptr;
  }
  ret->setSetOn(cmd);

  cmd = getCommand(config, "clear_on", true);
  if (! cmd) {
    return nullptr;
  }
  ret->setClearOn(cmd);
  }

  // Default type is magnitude
  ThresholdType threshold_type = ThresholdType::MAGNITUDE;
  std::optional<std::string_view> thtype = config["threshold_type"].value<std::string_view>();
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

  double proportion = config["threshold"].value_or(1.0);
  if (proportion < 0 || proportion > 1) {
    ++parse_warnings;
    PLOG_WARNING << "Condition threshold must be between 0 and 1. Using 1";
    proportion = 1.0;
  }
  ret->setThreshold(proportion);

  PLOG_VERBOSE << "Transient condition: " << *condition_name <<  "; " << ((thtype) ? *thtype : "magnitude") <<
      " threshold proportion = " << proportion << " -> " << ret->getThreshold();

  return ret;
}

void Game::buildSequenceList(toml::table& config) {

  // Set global parameters for sequences
  Sequence::setPressTime(config["controller"]["button_press_time"].value_or(0.0625));
  Sequence::setReleaseTime(config["controller"]["button_release_time"].value_or(0.0625));
  
  // For case of loading a 2nd game file
  sequences->clearSequenceList();

  // Initialize pre-defined sequences defined in the TOML file
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
        PLOG_ERROR << "Sequence definition missing required 'name' field";
        continue;
      }

      toml::array* event_list = config["sequence"].as_array();
      if (! event_list) {
        ++parse_errors;
        PLOG_ERROR << "Sequence definition missing required 'sequence' field";
        continue;
      }
      try {
        std::shared_ptr<Sequence> s = makeSequence(*seq, "sequence", true);
        if (!sequences->addDefinedSequence(*seq_name, s)) {
          ++parse_errors;
          PLOG_ERROR << "Duplicate sequence ignored: " << *seq_name;
        }
      }
      catch (const std::runtime_error& e) {
        ++parse_errors;
        PLOG_ERROR << "In definition for Sequence '" << *seq_name << "': " << e.what(); 
      }
    }
  } else {
    ++parse_warnings;
    PLOG_WARNING << "No pre-defined sequences found.";
  }

}

std::shared_ptr<Sequence> Game::makeSequence(toml::table& config, 
                                             const std::string& key,
                                             bool required) {

  std::shared_ptr<Sequence> seq = std::make_shared<Sequence>(controller);

  toml::array* event_list = config[key].as_array();
  if (! event_list) {
    if (required) {
      ++parse_errors;
      PLOG_ERROR << "Missing required '" << key << "' key";
    }
    return nullptr;
  }

  for (toml::node& elem : *event_list) {
    toml::table definition = *elem.as_table();
    TOMLUtils::checkValid(definition, std::vector<std::string>{"event", "command", "delay",
                          "repeat", "value"}, "makeSequence");
      
    std::optional<std::string> event = definition["event"].value<std::string>();
    if (! event) {
      ++parse_errors;
	    PLOG_ERROR << "Sequence missing required 'event' parameter";
      continue;
    }

    double delay = definition["delay"].value_or(0.0);
    if (delay < 0) {
      ++parse_errors;
	    PLOG_ERROR << "Delay must be a non-negative number of seconds.";
      continue;
    }
    unsigned int delay_usecs = (unsigned int) (delay * SEC_TO_MICROSEC);

    int repeat = definition["repeat"].value_or(1);
    if (repeat < 1) {
	    PLOG_WARNING << "The value of 'repeat' should be an integer greater than 1. Will ignore.";
      ++parse_warnings;
	    repeat = 1;
    }

    if (*event == "delay") {
      if (delay_usecs == 0) {
        PLOG_WARNING << "You've tried to add a delay of 0 microseconds. This will be ignored.";
        ++parse_warnings;
	    } else {
	      seq->addDelay(delay_usecs);
	    }
      continue;
    }

    std::optional<std::string> cmd = definition["command"].value<std::string>();

    // Append a predefined sequence
    if (*event == "sequence") {
      if (cmd) {
        std::shared_ptr<Sequence> new_seq = sequences->getSequence(*cmd);
        if (!new_seq) {
          ++parse_errors;
	        PLOG_ERROR << "Undefined sequence: " << *cmd;
          continue;
        }
        seq->addSequence(new_seq);
      }
    } else {
      // The remaining events all require a defined command.
      std::shared_ptr<GameCommand> command = (cmd) ? getCommand(*cmd) : nullptr;
      if (!command) {
        ++parse_errors;
        PLOG_ERROR << "Undefined command: " << *cmd;
        continue;
   	  }

      std::shared_ptr<ControllerInput> signal = command->getInput();

	    // If this signal is a hybrid control, this gets the button max (axes still return axis max value)
	    int max_val = (signal) ? signal->getMax(TYPE_BUTTON) : 0;
      int value = definition["value"].value_or(max_val);

      if (*event == "hold") {
	      if (repeat > 1) {
          ++parse_warnings;
	        PLOG_WARNING << "Repeat is not supported with 'hold' and will be ignored.";
    	  }
        PLOG_DEBUG << "Hold " << signal->getName() << " at value " << value << " for " << delay_usecs << " useconds";
	      seq->addHold(signal, value, delay_usecs);
      } else if (*event == "press") {
	      for (int i = 0; i < repeat; i++) {
	        seq->addPress(signal, value);
	        if (delay_usecs > 0) {
            PLOG_DEBUG << "Press " << signal->getName() << " at value " << value  << " with a delay of " << delay_usecs << " useconds";
	          seq->addDelay(delay_usecs);
	        } else {
            PLOG_DEBUG << "Press " << signal->getName();
          }
	      }
      } else if (*event == "release") {
	      if (repeat > 1) {
          ++parse_warnings;
	        PLOG_WARNING << "Repeat is not supported with 'release' and will be ignored.";
	      }
        PLOG_DEBUG << "Release " << signal->getName() << " (delay =" << delay_usecs << " usec)";
	      seq->addRelease(signal, delay_usecs);
      } else {
        ++parse_errors;
    	  PLOG_ERROR << "Unrecognized event type: " << *event;
      }
    }
  }
  return seq;
}

void Game::addGameCommands(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<ControllerInput>>& vec) {
  if (config.contains(key)) {
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      ++parse_errors;
      PLOG_ERROR << key << " must be an array of strings";
      return;
   	}

    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
   	  std::shared_ptr<GameCommand> item = getCommand(*cmd);
      if (item) {
        vec.push_back(item->getInput());
        PLOG_VERBOSE << "Added '" << *cmd << "' to the " << key << " vector.";
      } else {
        ++parse_errors;
        PLOG_ERROR << "Unrecognized command: " << *cmd << " in " << key;
     	}
    }
  }      
}

void Game::addGameCommands(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<GameCommand>>& vec) {
      
  if (config.contains(key)) {
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      ++parse_errors;
      PLOG_ERROR << key << " must be an array of strings";
      return;
   	}
	
    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
   	  std::shared_ptr<GameCommand> item = getCommand(*cmd);
      if (item) {
        vec.push_back(item);
        PLOG_VERBOSE << "Added '" << *cmd << "' to the " << key << " vector.";
      } else {
        ++parse_errors;
        PLOG_ERROR << "Unrecognized command: " << *cmd << " in " << key;
     	}
    }
  }      
}

void Game::addGameConditions(const toml::table& config, const std::string& key,
                             std::vector<std::shared_ptr<GameCondition>>& vec) {
      
  if (config.contains(key)) {
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      ++parse_errors;
      PLOG_ERROR << key << " must be an array of strings";
      return;
   	}
	
    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
   	  std::shared_ptr<GameCondition> item = getCondition(*cmd);
      if (item) {
        // Make a new copy of the reference condition rather than reusing the same pointer, so that each
        // mod has its own separate copy of the conditions' states.
        std::shared_ptr<GameCondition> copy(new GameCondition(*item));
        vec.push_back(copy);
        PLOG_VERBOSE << "Added '" << *cmd << "' to the " << key << " vector.";
      } else {
        ++parse_errors;
        PLOG_ERROR << "Unrecognized condition: " << *cmd << " in " << key;
     	}
    }
  }      
}

std::shared_ptr<GameCommand> Game::getCommand(const std::string& name) {
  auto iter = game_commands.find(name);
  if (iter != game_commands.end()) {
    return iter->second;
  }
  return nullptr;
}

std::shared_ptr<GameCommand> Game::getCommand(const toml::table& config, const std::string& key,
                                              bool required) {
  std::optional<std::string> cmd_name = config[key].value<std::string>();
  if (!cmd_name) {
    if (required) {
      ++parse_errors;
      PLOG_ERROR << "Missing  required '" << *cmd_name << "' parameter";
    }
    return nullptr;
  }
  std::shared_ptr<GameCommand> cmd = getCommand(*cmd_name);
  if (! cmd) {
    ++parse_errors;
    PLOG_ERROR << "The command '" << *cmd_name << "' does not exist (set_on)";
    return nullptr;
  }
  return cmd;
}

std::shared_ptr<GameCondition> Game::getCondition(const std::string& name) {
  auto iter = game_conditions.find(name);
  if (iter != game_conditions.end()) {
    return iter->second;
  }
  return nullptr;
}
