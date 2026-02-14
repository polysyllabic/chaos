/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS file at
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
#include "Configuration.hpp"
#include "ControllerInput.hpp"
#include "ControllerRaw.hpp"
#include "EngineInterface.hpp"
#include "ChaosEngine.hpp"

using namespace Chaos;

int main(int argc, char** argv) {

  try {
    // Process the configuration file for non-gameplay-related information
    // This will start up the logger and configure other basic settings
    Configuration chaos_config("chaosconfig.toml");

    // Now process the game-configuration file. If a file is specified on the command line, we use
    // that. Otherwise we use the default.
    std::string configfile = (argc > 1) ? argv[1] : chaos_config.getGameFile();

    // Configure controller and engine. Build the engine before starting the controller thread so
    // input injection is wired before any controller events are processed.
    ControllerRaw controller;
    ChaosEngine engine = ChaosEngine(controller, chaos_config.getListenerAddress(),
                                     chaos_config.getInterfaceAddress());

    // Keep engine running/listening even if the game file is invalid, but remain paused until a
    // valid game config is loaded.
    engine.setGame(configfile);

    controller.start();
    engine.start();

    while(engine.keepGoing()) {
      // loop until we get an exit signal
      usleep(1000000);
    }
  } catch (const std::exception& e) {
    std::cerr << "Fatal startup error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}
