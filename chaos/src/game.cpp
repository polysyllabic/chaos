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
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include "game.hpp"
#include "configuration.hpp"
#include "gameCommand.hpp"
#include "gameCondition.hpp"
#include "modifier.hpp"
#include "gameMenu.hpp"

using namespace Chaos;

Game::Game(const std::string& configfile) {
  // Initialize logging
  static plog::RollingFileAppender<plog::TxtFormatter> logger("chaos.log");
  loadConfigFile(configfile);
}

int Game::loadConfigFile(const std::string& configfile) {
  int error_count = 0;
  toml::table configuration;
  try {
    configuration = toml::parse_file(configfile);
  }
  catch (const toml::parse_error& err) {
    PLOG_ERROR << "Parsing the configuration file failed:" << err << std::endl;
    return 1;
  }

  // Process the TOML file. This will initialize all the mods and associated data.
  Configuration configuration(configfile);

  // Process those commands that do not have conditions
  GameCommand::buildCommandMapDirect(configuration);

  // Process all conditions
  GameCondition::buildConditionList(configuration);

  // Process those commands that have conditions
  GameCommand::buildCommandMapCondition(configuration);

  // Initialize defined sequences and static parameters for sequences
  Sequence::initialize(configuration);

  // Initialize the menu system
  menu = GameMenu(configuration);

  // Create the modifiers
  Modifier::buildModList(configuration);

}
