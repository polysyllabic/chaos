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
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <unistd.h>

#include <plog/Log.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Initializers/RollingFileInitializer.h>
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

struct Options {
  std::filesystem::path game_config_path;
  std::string mod_name;
  std::optional<std::filesystem::path> log_file;
  plog::Severity verbosity = plog::debug;
  std::optional<double> duration_override_sec;
  useconds_t loop_sleep_us = 500;
};

class ParseEngine final : public Chaos::EngineInterface {
public:
  ParseEngine() : game{controller} {}

  bool loadGame(const std::string& config_path) {
    return game.loadConfigFile(config_path, this);
  }

  int gameErrors() {
    return game.getErrors();
  }

  double defaultModifierDurationSeconds() {
    return game.getTimePerModifier();
  }

  bool isPaused() override {
    return false;
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
    game.getMenu().setState(item, new_val, true, controller);
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
      << "Usage: " << program << " -g <game-config.toml> -m <modifier name> [options]\n"
      << "Options:\n"
      << "  -g, --game-config <path>   Game config TOML file (required)\n"
      << "  -m, --mod <name>           Modifier name to validate (required)\n"
      << "  -v, --verbosity <0-6>      plog verbosity (default: 5/debug)\n"
      << "  -o, --output <path|->      Log output destination (default: stdout)\n"
      << "  -t, --time <seconds>       Override modifier lifespan for this run\n"
      << "      --sleep-us <us>        Loop sleep interval in microseconds (default: 500)\n"
      << "  -h, --help                 Show this help\n";
}

plog::Severity clampSeverity(int raw) {
  const int clamped = std::clamp(raw, static_cast<int>(plog::none), static_cast<int>(plog::verbose));
  return static_cast<plog::Severity>(clamped);
}

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

bool parseArgs(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      printUsage(argv[0]);
      std::exit(EXIT_SUCCESS);
    }
    if (arg == "-g" || arg == "--game-config") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      options.game_config_path = argv[++i];
      continue;
    }
    if (arg == "-m" || arg == "--mod") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      options.mod_name = argv[++i];
      continue;
    }
    if (arg == "-v" || arg == "--verbosity") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      try {
        options.verbosity = clampSeverity(std::stoi(argv[++i]));
      } catch (const std::exception&) {
        std::cerr << "Invalid value for " << arg << ": " << argv[i] << "\n";
        return false;
      }
      continue;
    }
    if (arg == "-o" || arg == "--output") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      std::filesystem::path output_path = argv[++i];
      if (output_path != "-") {
        options.log_file = output_path;
      } else {
        options.log_file.reset();
      }
      continue;
    }
    if (arg == "-t" || arg == "--time") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      try {
        options.duration_override_sec = std::stod(argv[++i]);
      } catch (const std::exception&) {
        std::cerr << "Invalid value for " << arg << ": " << argv[i] << "\n";
        return false;
      }
      if (!options.duration_override_sec || *options.duration_override_sec <= 0.0) {
        std::cerr << "Modifier duration must be greater than zero.\n";
        return false;
      }
      continue;
    }
    if (arg == "--sleep-us") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      try {
        const long parsed = std::stol(argv[++i]);
        if (parsed <= 0 || parsed > static_cast<long>(std::numeric_limits<useconds_t>::max())) {
          std::cerr << "Invalid value for --sleep-us: " << argv[i] << "\n";
          return false;
        }
        options.loop_sleep_us = static_cast<useconds_t>(parsed);
      } catch (const std::exception&) {
        std::cerr << "Invalid value for --sleep-us: " << argv[i] << "\n";
        return false;
      }
      continue;
    }
    std::cerr << "Unknown option: " << arg << "\n";
    return false;
  }

  if (options.game_config_path.empty()) {
    std::cerr << "Missing required option: -g/--game-config\n";
    return false;
  }
  if (options.mod_name.empty()) {
    std::cerr << "Missing required option: -m/--mod\n";
    return false;
  }
  return true;
}

void initializeLogging(const Options& options) {
  if (options.log_file) {
    const std::filesystem::path parent = options.log_file->parent_path();
    if (!parent.empty()) {
      std::filesystem::create_directories(parent);
    }
    const std::string log_file_path = options.log_file->string();
    plog::init(options.verbosity, log_file_path.c_str());
    return;
  }
  plog::init<plog::TxtFormatter>(options.verbosity, plog::streamStdOut);
}

std::shared_ptr<Chaos::Modifier> resolveModifier(ParseEngine& engine, const std::string& requested_name) {
  std::shared_ptr<Chaos::Modifier> mod = engine.getModifier(requested_name);
  if (mod) {
    return mod;
  }

  const std::string requested_lower = toLower(requested_name);
  std::shared_ptr<Chaos::Modifier> found;
  int matches = 0;
  for (const auto& entry : engine.getModifierMap()) {
    if (toLower(entry.first) == requested_lower) {
      found = entry.second;
      matches++;
    }
  }
  if (matches == 1) {
    PLOG_WARNING << "Using case-insensitive modifier match: '" << found->getName() << "'";
    return found;
  }
  return nullptr;
}

void listAvailableModifiers(ParseEngine& engine) {
  std::vector<std::string> names;
  names.reserve(engine.getModifierMap().size());
  for (const auto& entry : engine.getModifierMap()) {
    if (entry.second) {
      names.push_back(entry.second->getName());
    }
  }
  std::sort(names.begin(), names.end());
  PLOG_INFO << "Available modifiers (" << names.size() << "):";
  for (const auto& name : names) {
    PLOG_INFO << " - " << name;
  }
}

int runModifierLifecycle(std::shared_ptr<Chaos::Modifier> mod, double duration_sec, useconds_t loop_sleep_us) {
  if (!mod) {
    return EXIT_FAILURE;
  }

  std::list<std::shared_ptr<Chaos::Modifier>> active_mods;
  std::list<std::shared_ptr<Chaos::Modifier>> pending_start;
  std::list<std::shared_ptr<Chaos::Modifier>> pending_stop;

  mod->setLifespan(duration_sec);
  pending_start.push_back(mod);

  PLOG_INFO << "Running modifier '" << mod->getName() << "' for up to "
            << duration_sec << " seconds";

  bool paused_prior = false;
  bool saw_start = false;
  bool saw_finish = false;

  while (true) {
    usleep(loop_sleep_us);

    std::vector<std::shared_ptr<Chaos::Modifier>> mods_to_finish;
    std::vector<std::shared_ptr<Chaos::Modifier>> mods_to_begin;
    std::vector<std::shared_ptr<Chaos::Modifier>> mods_to_update;

    while (!pending_stop.empty()) {
      std::shared_ptr<Chaos::Modifier> queued_stop = pending_stop.front();
      pending_stop.pop_front();
      auto it = std::find(active_mods.begin(), active_mods.end(), queued_stop);
      if (it != active_mods.end()) {
        active_mods.erase(it);
        mods_to_finish.push_back(queued_stop);
      }
    }

    while (!pending_start.empty()) {
      std::shared_ptr<Chaos::Modifier> queued_start = pending_start.front();
      pending_start.pop_front();
      if (std::find(active_mods.begin(), active_mods.end(), queued_start) != active_mods.end()) {
        continue;
      }
      active_mods.push_back(queued_start);
      mods_to_begin.push_back(queued_start);
    }

    mods_to_update.assign(active_mods.begin(), active_mods.end());

    for (auto& m : mods_to_finish) {
      m->_finish();
      if (m == mod) {
        saw_finish = true;
      }
    }

    for (auto& m : mods_to_begin) {
      m->_begin();
      if (m == mod) {
        saw_start = true;
      }
    }

    for (auto& m : mods_to_update) {
      m->_update(paused_prior);
    }
    paused_prior = false;

    std::shared_ptr<Chaos::Modifier> expired_mod;
    for (auto& m : active_mods) {
      if (m->lifetime() > m->lifespan()) {
        expired_mod = m;
        break;
      }
    }

    if (expired_mod) {
      bool already_queued = (std::find(pending_stop.begin(), pending_stop.end(), expired_mod)
                             != pending_stop.end());
      if (!already_queued) {
        pending_stop.push_back(expired_mod);
      }
    }

    if (saw_start && saw_finish && active_mods.empty() && pending_start.empty() && pending_stop.empty()) {
      break;
    }
  }

  PLOG_INFO << "Modifier '" << mod->getName() << "' completed. Lifetime=" << mod->lifetime()
            << "s, lifespan=" << mod->lifespan() << "s";
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv) {
  Options options;
  if (!parseArgs(argc, argv, options)) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  try {
    initializeLogging(options);
  } catch (const std::exception& err) {
    std::cerr << "Failed to initialize logging: " << err.what() << "\n";
    return EXIT_FAILURE;
  }

  ParseEngine engine;
  bool loaded = false;
  try {
    loaded = engine.loadGame(options.game_config_path.string());
  } catch (const std::exception& err) {
    PLOG_ERROR << "Fatal parse exception: " << err.what();
    return EXIT_FAILURE;
  } catch (...) {
    PLOG_ERROR << "Fatal parse exception: unknown error";
    return EXIT_FAILURE;
  }

  const bool playable = loaded && engine.gameErrors() == 0;
  if (!playable) {
    PLOG_WARNING << "Game configuration '" << options.game_config_path.string()
                 << "' has errors or failed to load.";
    return EXIT_FAILURE;
  }

  std::shared_ptr<Chaos::Modifier> mod = resolveModifier(engine, options.mod_name);
  if (!mod) {
    PLOG_ERROR << "Modifier '" << options.mod_name << "' was not found in game config '"
               << options.game_config_path.string() << "'.";
    listAvailableModifiers(engine);
    return EXIT_FAILURE;
  }

  const double duration_sec = options.duration_override_sec.value_or(engine.defaultModifierDurationSeconds());
  try {
    return runModifierLifecycle(mod, duration_sec, options.loop_sleep_us);
  } catch (const std::exception& err) {
    PLOG_ERROR << "Fatal validation exception: " << err.what();
    return EXIT_FAILURE;
  } catch (...) {
    PLOG_ERROR << "Fatal validation exception: unknown error";
    return EXIT_FAILURE;
  }
}
