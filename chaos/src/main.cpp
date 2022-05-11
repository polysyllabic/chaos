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
#include <cstdlib>
#include <iostream>
#include <string>

#include <toml++/toml.h>

#include "config.hpp"
#include "chaosEngine.hpp"
#include "controller.hpp"
#include "configuration.hpp"
#include "game.hpp"

using namespace Chaos;

int main(int argc, char** argv) {

  // Process the configuration file for non-gameplay-related information
  // This will start up the logger and configure other basic settings
  Configuration chaos_config("chaosconfig.toml");

  // Now process the game-configuration file. If a file is specified on the command line, we use
  // that. Otherwise we use the default. If no default is set, or the specified file does not exist,
  // we abort.
  if (argc == 1) {
    std::cerr << "Usage: chaos input_file\n  input_file: The TOML file for the game you want to make chaotic.\n";
    exit (EXIT_FAILURE);
  }
  std::string configfile = (argc > 1) ? argv[1] : chaos_config.getGameFile();

  // Parse the game information
  Game game = Game(configfile);

  // Configure the controller
  Controller controller;
  controller.start();

  // Start the engine
  std::shared_ptr<ChaosEngine> engine = std::make_shared<ChaosEngine>(controller);
  engine->start();

  // Set the data that the interface will want to query
  engine->setGame(game.getName());
  engine->setActiveMods(game.getNumActiveMods());
  engine->setTimePerModifier(game.getTimePerModifier());

  while(engine->keepGoing()) {
    // loop until we get an exit signal
    usleep(1000000);
  }
  
  return 0;
}
