/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78
*
* TCC is free software: you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* TCC is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along
* with TCC.  If not, see <https://www.gnu.org/licenses/>.
*---------------------------------------------------------------------------*/
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

// Header-only libraries that we use
#include <tclap/CmdLine.h>
#include <toml++/toml.h>
#include <plog/Log.h>
#include "plog/Initializers/RollingFileInitializer.h"

#include "chaos.hpp"
#include "chaosEngine.hpp"
#include "modifiers.h"

using namespace Chaos;

int main(int argc, char** argv) {
  std::string logfile;
  std::string configfile;
  int verbosity;
  // Process command line arguments
  try {
    TCLAP::CmdLine cmd("Twitch Controls Chaos", ' ', TCC_VERSION);
    TCLAP::ValueArg<std::string> logFileName("l", "log", "Name of logging file", true, "chaos.log","file name");
    cmd.add(logFileName);
    TCLAP::ValueArg<int> logFileVerbosity("v", "verbosity", "Detail of output to log file", true, 3, "0-6");
    cmd.add(logFileVerbosity);
    TCLAP::UnlabeledValueArg<stdt::strong> configFileName("config", "toml configuration file", true, "", "toml");
    cmd.add(configFileName);
    cmd.parse (argc, argv);
    logfile = logFileName.getValue();
    verbosity = logFileVerbosity.getValue();
    configfile = configFileName.getValue();
  }
  catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(EXIT_FAILURE);
  }

  // Initialize the logger
  plog::init(verbosity, logname);  
  PLOG_NONE << "Welcome to Chaos " << CHAOS_VERSION << std::endl;

  ControllerRaw controller;
  controller.initialize();
  controller.start();

  // Read the toml configuration file
  toml::table config;
  try {
    config = toml::parse_file(configfile);
    PLOG_DEBUG << "Configuration read: " << std::endl << config << std::endl;
  }
  catch (const toml::parse_error& err) {
    std::cerr << "Parsing the configuration file failed:" << std::endl << err << std::endl;
    exit (EXIT_FAILURE);
  }
  
  Chaos::Modifier::initialize(config);
	
  Chaos::Engine chaosEngine(&controller);
  chaosEngine.start();
	
  double timePerModifier = 30.0;
  chaosEngine.setTimePerModifier(timePerModifier);
	
  std::string reply = Chaos::Modifier::getModList(timePerModifier);
  while(1) {
    chaosEngine.setInterfaceReply( reply );
    usleep(10000000);
  }
  
  return 0;
}
