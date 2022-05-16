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

#include "Game.hpp"
#include "TOMLUtils.hpp"
#include "GameCommand.hpp"
#include "GameCondition.hpp"
#include "Modifier.hpp"
#include "GameMenu.hpp"

using namespace Chaos;

Game::Game() {
  sequences = std::make_shared<SequenceTable>();
}

bool Game::loadConfigFile(const std::string& configfile, Controller& controller) {
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

  // Process the game-command definitions
  parse_errors += game_commands.buildCommandList(configuration, signal_table);

  // Process the conditions
  parse_errors += game_conditions.buildConditionList(configuration, game_commands);

  // Initialize the remapping table
  parse_errors += signal_table.initializeInputs(configuration, game_conditions);

  // Initialize defined sequences and static parameters for sequences
  parse_errors += sequences->buildSequenceList(configuration, game_commands, controller);

  use_menu = configuration["use_menu"].value_or(true);

  if (use_menu) {
    // Initialize the menu system
    menu.initialize(configuration, sequences);
  }

  // Create the modifiers
  parse_errors += modifiers.buildModList(configuration, use_menu);
  return true;
}
