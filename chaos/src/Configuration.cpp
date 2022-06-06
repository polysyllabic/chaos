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
#include <filesystem>
#include <stdexcept>

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include "config.hpp"
#include "enumerations.hpp"
#include "Configuration.hpp"
#include "GameCommand.hpp"
#include "Modifier.hpp"
#include "GameMenu.hpp"

using namespace Chaos;

// Parse the TOML file into memory and do initial setup.
Configuration::Configuration(const std::string& fname) {
  toml::table configuration;
  try {
    configuration = toml::parse_file(fname);
  }
  catch (const toml::parse_error& err) {
    // to do: recreate a default config file if it's missing
    std::cerr << "Parsing the configuration file failed: " << err << std::endl;
    throw err;
  }

  toml_version = configuration["chaos_toml"].value_or("");
  if (toml_version.empty()) {
    throw std::runtime_error("Missing chaos version identifier in TOML configuration file");
  }

  log_path = configuration["log_directory"].value_or(".");
  if (! std::filesystem::exists(log_path)) {
    // If this directory doesn't exist, create it
    std::filesystem::create_directory(log_path);
  } else {
    // Verify that this is actually a directory and not a file
    if (! std::filesystem::is_directory(log_path)) {
      std::cerr << "Log path '" << log_path.string() << "' is not a directory!";
    }
  }

  // Set up the main log
  std::filesystem::path logfile = configuration["log_file"].value_or("chaos.log");  
  logfile = log_path / logfile;
  
  bool overwrite = configuration["overwrite_log"].value_or(false);

  plog::Severity maxSeverity = (plog::Severity) configuration["log_verbosity"].value_or(3);

  size_t max_size = (size_t) configuration["max_log_size"].value_or(0);
  int max_logs = configuration["max_log_files"].value_or(8);  
  if (overwrite && std::filesystem::exists(logfile)) {
    std::filesystem::remove(logfile);
  }
  plog::init(maxSeverity, logfile.c_str(), max_size, max_logs);
  PLOG_NONE << "Welcome to Chaos " << CHAOS_VERSION;

  std::filesystem::path sniflog = configuration["sniffify_log"].value_or("");
  sniflog = log_path / sniflog;

  plog::Severity sniffifySeverity = (plog::Severity) configuration["sniffify_verbosity"].value_or(3);
  // IF we've specified a separate sniffify log, set it up
  //if (! sniflog.string().empty()) {
  //  initializeSniffifyLog(sniffifySeverity, sniflog);
  //}

  interface_addr = configuration["interface_addr"].value_or("localhost");
  interface_port = configuration["interface_port"].value_or(5556);
  PLOG_DEBUG << "Sending messages to chaosface at endpoint " << getInterfaceAddress();
  listener_port = configuration["listener_port"].value_or(5555);
  PLOG_DEBUG << "Listening to messages from chaosface at endpoint " << getListenerAddress();

  game_directory = configuration["game_directory"].value_or(".");
  // Error if directory does not exist, or the path contains an ordinary file
  if (! std::filesystem::exists(game_directory)) {
    PLOG_ERROR << "Game directory '" << game_directory.string() << "' does not exist!";
    game_directory = ".";
  } else if (! std::filesystem::is_directory(game_directory)) {
    PLOG_ERROR << "Game directory '" << game_directory.string() << "' is not a directory!";
    game_directory = ".";
  }

  // If the game configuration file doesn't specify a path, use the default games directory
  game_config = configuration["default_game"].value_or("");
  if (! game_config.has_parent_path()) {
    game_config = game_directory / game_config;
  }

}




