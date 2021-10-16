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
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include <tclap/CmdLine.h>

#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include <toml++/toml.h>
#include <fstream>

#include "config.hpp"
#include "chaosEngine.hpp"
#include "modifier.hpp"
#include "controllerRaw.hpp"

using namespace Chaos;

int main(int argc, char** argv) {
  std::string logfile;
  std::string configfile;
  plog::Severity maxSeverity;
  // Process command line arguments
  try {
    TCLAP::CmdLine cmd("Twitch Controls Chaos", ' ', CHAOS_VERSION);
    TCLAP::ValueArg<std::string> logFileName("l", "log", "Name of logging file", true, "chaos.log","file name");
    cmd.add(logFileName);
    TCLAP::ValueArg<int> logFileVerbosity("v", "verbosity", "Detail of output to log file", true, 3, "0-6");
    cmd.add(logFileVerbosity);
    TCLAP::UnlabeledValueArg<std::string> configFileName("config", "toml configuration file", true, "", "toml");
    cmd.add(configFileName);
    cmd.parse (argc, argv);
    logfile = logFileName.getValue();
    maxSeverity = (plog::Severity) logFileVerbosity.getValue();
    configfile = configFileName.getValue();
  }
  catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(EXIT_FAILURE);
  }

  // Initialize the logger
  plog::init(maxSeverity, logfile.c_str());  
  PLOG_NONE << "Welcome to Chaos " << CHAOS_VERSION << std::endl;

  // Read the toml configuration file
  toml::table config;
  try {
    config = toml::parse_file(configfile);
    PLOG_DEBUG << "Configuration read: " << std::endl << config << std::endl;
  }
  catch (const toml::parse_error& err) {
    PLOG_FATAL << "Parsing the configuration file failed:" << std::endl << err << std::endl;
    exit (EXIT_FAILURE);
  }

  std::optional<std::string_view> toml_version = config["chaos_toml"].value<std::string_view>();
  if (!toml_version) {
    PLOG_FATAL << "Could not find version id (chaos_toml). Are you sure this is the right file?\n";
    exit(EXIT_FAILURE);
  }

  std::string chaos_game = config["game"].value_or("game unknown");
  PLOG_NONE << "Initializing chaos for " << chaos_game << std::endl;
  
  // Configure the controller
  ControllerRaw controller;
  controller.initialize();
  controller.start();


  // Assign the default mapping of commands to button/joystick presses.
  // The TOML file defines bindings as an array of tables, each table containing one
  // command-to-controll mapping.
  auto bindings = config["command"];
  if (toml::array* arr = cmds.as_array()) {
    for (toml::node& elem : arr) {
      if (toml::table* command = elem.as_table()) {
	if (command->contains("name") && command->contains("binding")) {
	  std::optional<std::string_view> cmd_name = command->get("name")->value<std::string_view>();
	  std::optional<std::string_view> cmd_bind = command->get("binding")->value<std::string_view>();
	  
	}
	else {
	  PLOG_ERROR << "Bad command binding: " << *command << "\n";
	}
	
      }
    }
  } else {
    PLOG_ERROR << "No commands defined in TOML file\n";
    exit (EXIT_FAILURE);
  }
  
  // Walk through the modifiers defined in the TOML file and create the appropriate list of
  // modifiers.
  auto mods = config["modifier"];
  
	
  ChaosEngine chaosEngine(&controller);
  chaosEngine.start();
	
  double timePerModifier = 30.0;
  chaosEngine.setTimePerModifier(timePerModifier);
	
  std::string reply = Chaos::Modifier::getModList();
  while(1) {
    chaosEngine.setInterfaceReply( reply );
    usleep(10000000);
  }
  
  return 0;
}
