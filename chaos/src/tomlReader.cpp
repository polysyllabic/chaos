/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
#include <fstream>
#include <stdexcept>

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include "tomlReader.hpp"
#include "config.hpp"
#include "controllerCommand.hpp"
#include "gameCommand.hpp"
#include "modifier.hpp"

using namespace Chaos;

// Load the TOML file into memory and do initial setup.
// Note that the logger should NOT be initialized when this is constructed. We do that
// based on parameters defined in the TOML file.
TOMLReader::TOMLReader(const std::string& fname) {
  try {
    configuration = toml::parse_file(fname);
  }
  catch (const toml::parse_error& err) {
    // log and re-throw the error
    std::cerr << "Parsing the configuration file failed:\n" << err << std::endl;
    throw err;
  }
  
  // Initialize the logger
  std::string logfile = configuration["log_file"].value_or("chaos.log");
  plog::Severity maxSeverity = (plog::Severity) configuration["log_verbosity"].value_or(3);
  size_t max_size = (size_t) configuration["max_log_size"].value_or(0);
  int max_logs = configuration["max_log_files"].value_or(8);  
  plog::init(maxSeverity, logfile.c_str(), max_size, max_logs);
  
  PLOG_NONE << "Welcome to Chaos " << CHAOS_VERSION << std::endl;

  version = configuration["chaos_toml"].value<std::string_view>();
  if (!version) {
    PLOG_FATAL << "Could not find the version id (chaos_toml). Are you sure this is the right TOML file?\n";
    throw std::runtime_error("Missing chaos_toml identifier");
  }
  
  game = configuration["game"].value<std::string_view>();
  if (!game) {
    PLOG_NONE << "Playing " << *game << "\n";
  }
  else {
    PLOG_WARNING << "No name specified for this game.\n";
  }

  // set up static definitions of controller input names
  ControllerCommand::initialize();
  // Assign the default mappings of game commands to specific button/joystick presses
  GameCommand::initialize(configuration);
  // Create the modifiers
  Modifier::buildModList(configuration);
}


std::string_view TOMLReader::getGameName() {
  return (game) ? *game : "UNKNOWN";
}

std::string_view TOMLReader::getVersion() {
  return (version) ? *version : "UNKNOWN";
}

