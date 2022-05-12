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
#include "configuration.hpp"
#include "gameCommand.hpp"
#include "modifier.hpp"
#include "gameMenu.hpp"

using namespace Chaos;

// Parse the TOML file into memory and do initial setup.
Configuration::Configuration(const std::string& fname) {
  toml::table configuration;
  try {
    configuration = toml::parse_file(fname);
  }
  catch (const toml::parse_error& err) {
    std::cerr << "Parsing the configuration file failed: " << err << std::endl;
    throw err;
  }

  log_path = configuration["log_dir"].value_or(".");
  if (! std::filesystem::exists(log_path)) {
    // If this directory doesn't exist, create it
    std::filesystem::create_directory(log_path);
  } else {
    // Verify that this is actually a directory and not a file
    if (! std::filesystem::is_directory(log_path)) {
      std::cerr << "Log path '" << log_path.string() << "' is not a directory!";
    }
  }
  // Initialize the logger
  std::filesystem::path logfile = configuration["log_file"].value_or("chaos.log");  
  logfile = log_path / logfile;
  
  plog::Severity maxSeverity = (plog::Severity) configuration["log_verbosity"].value_or(3);
  size_t max_size = (size_t) configuration["max_log_size"].value_or(0);
  int max_logs = configuration["max_log_files"].value_or(8);  
  plog::init(maxSeverity, logfile.c_str(), max_size, max_logs);
  
  PLOG_NONE << "Welcome to Chaos " << CHAOS_VERSION;

}




