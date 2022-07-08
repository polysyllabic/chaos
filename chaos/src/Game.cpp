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

using namespace Chaos;

Game::Game(Controller& c) : controller{c}, signal_table{c} {
  sequences = std::make_shared<SequenceTable>();
}

bool Game::loadConfigFile(const std::string& configfile, EngineInterface* engine) {
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

  double t = configuration["mod_defaults"]["time_per_modifier"].value_or(180.0);
  time_per_modifier = dseconds(t);

  if (time_per_modifier < std::chrono::seconds(10)) {
    PLOG_WARNING << "Minimum active time for modifiers is 10 seconds.";
    time_per_modifier = std::chrono::seconds(10);
  }
  PLOG_INFO << "Time per modifier: " << time_per_modifier.count() << " seconds";

  // Process the game-command definitions
  parse_errors += game_commands.buildCommandList(configuration, signal_table);

  // Process the conditions
  parse_errors += game_conditions.buildConditionList(configuration, game_commands);

  // Initialize the remapping table
  parse_errors += signal_table.initializeInputs(configuration, game_conditions);

  assert(sequences);
  // Initialize defined sequences and static parameters for sequences
  parse_errors += sequences->buildSequenceList(configuration, game_commands, controller);

  use_menu = configuration["use_menu"].value_or(true);

  if (use_menu) {
    // Initialize the menu system's general settings
    parse_errors += menu.initialize(configuration, sequences);
    makeMenu(configuration);
  }

  // Create the modifiers
  parse_errors += modifiers.buildModList(configuration, engine, use_menu);
  PLOG_INFO << "Loaded configuration file for " << name << " with " << parse_errors << " errors.";
  return true;
}

void Game::makeMenu(toml::table& config) {
  PLOG_VERBOSE << "Creating menu items menu";

  toml::table* menu_list = config["menu"].as_table();
  if (! config.contains("menu")) {
    // this error was already reported in the menu.initialize() routine, so don't double-count
    return;
  }

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
