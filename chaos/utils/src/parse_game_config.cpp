/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributors.
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

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <plog/Log.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <toml++/toml.h>

#include "Controller.hpp"
#include "ControllerInput.hpp"
#include "DeviceEvent.hpp"
#include "EngineInterface.hpp"
#include "Game.hpp"
#include "GameCommand.hpp"
#include "GameCondition.hpp"
#include "MenuItem.hpp"
#include "Modifier.hpp"
#include "Sequence.hpp"

namespace {

class ParseEngine final : public Chaos::EngineInterface {
public:
  ParseEngine() : game{controller} {}

  bool loadGame(const std::string& config_path) {
    return game.loadConfigFile(config_path, this);
  }

  int gameErrors() {
    return game.getErrors();
  }

  bool isPaused() override {
    return true;
  }

  void fakePipelinedEvent(Chaos::DeviceEvent& event, std::shared_ptr<Chaos::Modifier>) override {
    controller.applyEvent(event);
  }

  short getState(uint8_t id, uint8_t type) override {
    return controller.getState(id, type);
  }

  bool eventMatches(const Chaos::DeviceEvent& event, std::shared_ptr<Chaos::GameCommand> command) override {
    if (!command) {
      return false;
    }
    std::shared_ptr<Chaos::ControllerInput> signal = command->getInput();
    return signal ? controller.matches(event, signal) : false;
  }

  void setOff(std::shared_ptr<Chaos::GameCommand> command) override {
    if (!command) {
      return;
    }
    std::shared_ptr<Chaos::ControllerInput> signal = command->getInput();
    if (signal) {
      controller.setOff(signal);
    }
  }

  void setOn(std::shared_ptr<Chaos::GameCommand> command) override {
    if (!command) {
      return;
    }
    std::shared_ptr<Chaos::ControllerInput> signal = command->getInput();
    if (signal) {
      controller.setOn(signal);
    }
  }

  void setValue(std::shared_ptr<Chaos::GameCommand> command, short value) override {
    if (!command) {
      return;
    }
    std::shared_ptr<Chaos::ControllerInput> signal = command->getInput();
    if (signal) {
      controller.setValue(signal, value);
    }
  }

  void applyEvent(const Chaos::DeviceEvent& event) override {
    controller.applyEvent(event);
  }

  std::shared_ptr<Chaos::Modifier> getModifier(const std::string& name) override {
    return game.getModifier(name);
  }

  std::unordered_map<std::string, std::shared_ptr<Chaos::Modifier>>& getModifierMap() override {
    return game.getModifierMap();
  }

  std::list<std::shared_ptr<Chaos::Modifier>>& getActiveMods() override {
    return active_mods;
  }

  std::shared_ptr<Chaos::MenuItem> getMenuItem(const std::string& name) override {
    return game.getMenu().getMenuItem(name);
  }

  void setMenuState(std::shared_ptr<Chaos::MenuItem> item, unsigned int new_val) override {
    game.getMenu().setState(item, new_val, false, controller);
  }

  void restoreMenuState(std::shared_ptr<Chaos::MenuItem> item) override {
    game.getMenu().restoreState(item, controller);
  }

  std::shared_ptr<Chaos::ControllerInput> getInput(const std::string& name) override {
    return game.getSignalTable().getInput(name);
  }

  std::shared_ptr<Chaos::ControllerInput> getInput(const Chaos::DeviceEvent& event) override {
    return game.getSignalTable().getInput(event);
  }

  void addControllerInputs(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<Chaos::ControllerInput>>& vec) override {
    game.getSignalTable().addToVector(config, key, vec);
  }

  std::string getEventName(const Chaos::DeviceEvent& event) override {
    return game.getEventName(event);
  }

  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<Chaos::GameCommand>>& vec) override {
    game.addGameCommands(config, key, vec);
  }

  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<Chaos::ControllerInput>>& vec) override {
    game.addGameCommands(config, key, vec);
  }

  void addGameConditions(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<Chaos::GameCondition>>& vec) override {
    game.addGameConditions(config, key, vec);
  }

  std::shared_ptr<Chaos::Sequence> createSequence(toml::table& config,
                                                  const std::string& key, bool required) override {
    return game.makeSequence(config, key, required);
  }

private:
  Chaos::Controller controller;
  Chaos::Game game;
  std::list<std::shared_ptr<Chaos::Modifier>> active_mods;
};

void printUsage(const char* program) {
  std::cerr
      << "Usage: " << program
      << " <game-config.toml> [--chaos-config <chaosconfig.toml>] [--verbosity <0-6>]\n";
}

plog::Severity clampSeverity(int raw) {
  const int clamped = std::clamp(raw, static_cast<int>(plog::none), static_cast<int>(plog::verbose));
  return static_cast<plog::Severity>(clamped);
}

plog::Severity getConfiguredSeverity(const std::filesystem::path& chaos_config_path, plog::Severity fallback) {
  try {
    toml::table config = toml::parse_file(chaos_config_path.string());
    const int configured = config["log_verbosity"].value_or(static_cast<int>(fallback));
    return clampSeverity(configured);
  } catch (const std::exception& err) {
    std::cerr << "Warning: unable to read " << chaos_config_path << ": " << err.what()
              << ". Using fallback log verbosity " << static_cast<int>(fallback) << ".\n";
    return fallback;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  std::filesystem::path game_config_path;
  std::filesystem::path chaos_config_path{"chaosconfig.toml"};
  plog::Severity severity = plog::info;
  bool explicit_verbosity = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--chaos-config") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --chaos-config\n";
        return EXIT_FAILURE;
      }
      chaos_config_path = argv[++i];
      continue;
    }
    if (arg == "--verbosity") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for --verbosity\n";
        return EXIT_FAILURE;
      }
      try {
        severity = clampSeverity(std::stoi(argv[++i]));
      } catch (const std::exception&) {
        std::cerr << "Invalid value for --verbosity: " << argv[i] << "\n";
        return EXIT_FAILURE;
      }
      explicit_verbosity = true;
      continue;
    }
    if (!arg.empty() && arg[0] == '-') {
      std::cerr << "Unknown option: " << arg << "\n";
      printUsage(argv[0]);
      return EXIT_FAILURE;
    }
    if (!game_config_path.empty()) {
      std::cerr << "Unexpected extra argument: " << arg << "\n";
      printUsage(argv[0]);
      return EXIT_FAILURE;
    }
    game_config_path = arg;
  }

  if (game_config_path.empty()) {
    std::cerr << "Missing game configuration path\n";
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }
  if (!explicit_verbosity) {
    severity = getConfiguredSeverity(chaos_config_path, severity);
  }

  plog::init<plog::TxtFormatter>(severity, plog::streamStdOut);

  ParseEngine engine;
  bool loaded = false;
  try {
    loaded = engine.loadGame(game_config_path.string());
  } catch (const std::exception& err) {
    PLOG_ERROR << "Fatal parse exception: " << err.what();
    loaded = false;
  } catch (...) {
    PLOG_ERROR << "Fatal parse exception: unknown error";
    loaded = false;
  }
  const bool playable = loaded && engine.gameErrors() == 0;
  if (!playable) {
    PLOG_WARNING << "Game configuration '" << game_config_path.string()
                 << "' has errors or failed to load. Staying paused until a valid game is loaded.";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
