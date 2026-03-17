/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributors.
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
#include <memory>
#include <csignal>
#include <string>
#include <unistd.h>

#include <toml++/toml.h>

#include "config.hpp"
#include "Configuration.hpp"
#include "ControllerInput.hpp"
#include "ControllerRaw.hpp"
#include "EngineInterface.hpp"
#include "ChaosEngine.hpp"

using namespace Chaos;

namespace {
volatile std::sig_atomic_t gShutdownSignal = 0;

void handleShutdownSignal(int signal_number) {
  gShutdownSignal = signal_number;
}

void installShutdownSignalHandlers() {
  struct sigaction action {};
  action.sa_handler = handleShutdownSignal;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);
}
}

int main(int argc, char** argv) {

  std::unique_ptr<ControllerRaw> controller;
  std::unique_ptr<ChaosEngine> engine;

  try {
    // Process the configuration file for non-gameplay-related information
    // This will start up the logger and configure other basic settings
    Configuration chaos_config("chaosconfig.toml");

    // If a game config path is specified on the command line, use that immediately.
    // Otherwise, wait for chaosface to pick one from the discovered list.
    std::string configfile = (argc > 1) ? argv[1] : "";

    // Configure controller and engine. Build the engine before starting the controller thread so
    // input injection is wired before any controller events are processed.
    controller = std::make_unique<ControllerRaw>();
    engine = std::make_unique<ChaosEngine>(*controller, chaos_config.getListenerAddress(),
                                           chaos_config.getInterfaceAddress(), true,
                                           chaos_config.getDefaultModListPath());
    installShutdownSignalHandlers();

    engine->setAvailableGames(chaos_config.getAvailableGames());
    if (!configfile.empty()) {
      // Keep engine running/listening even if the game file is invalid, but remain paused until a
      // valid game config is loaded.
      engine->setGame(configfile);
    } else {
      engine->announceAvailableGames();
    }

    controller->start();
    engine->start();
  } catch (const std::exception& e) {
    std::cerr << "Fatal startup error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  try {
    while(engine->keepGoing()) {
      if (gShutdownSignal != 0) {
        engine->requestShutdown();
      }
      // loop until we get an exit signal
      usleep(100000);
    }
  } catch (const std::exception& e) {
    std::cerr << "Fatal runtime error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}
