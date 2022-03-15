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
//#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include <toml++/toml.h>
#include <fstream>

#include "config.hpp"
#include "chaosEngine.hpp"
#include "modifier.hpp"
#include "controller.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

int main(int argc, char** argv) {
  // the name of the configuration file should be the only argument on the command line.
  if (argc == 1) {
    std::cerr << "usage: chaos input_file\n  input_file: The TOML file for the game you want to make chaotic.\n";
    exit (EXIT_FAILURE);
  }
  std::string configfile(argv[1]);
  
  // Configure the controller
  Controller::instance().initialize();
  Controller::instance().start();

  // Process the TOML file. This will initialize all the mods and associated data.
  TOMLReader config(configfile);

  // Start the engine
  ChaosEngine::instance().start();

  // Announce the mods to the chaos interface
  std::string reply = Modifier::getModList();
  while(1) {
    ChaosEngine::instance().setInterfaceReply( reply );
    usleep(10000000);
  }
  
  return 0;
}
