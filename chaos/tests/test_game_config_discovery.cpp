#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "Configuration.hpp"

using namespace Chaos;

namespace {

bool check(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

std::filesystem::path makeTempDir() {
  char path_template[] = "/tmp/chaos_game_discovery_XXXXXX";
  char* created = mkdtemp(path_template);
  if (created == nullptr) {
    throw std::runtime_error("Failed to create temporary directory");
  }
  return std::filesystem::path(created);
}

void writeTextFile(const std::filesystem::path& path, const std::string& contents) {
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("Failed to open file for writing: " + path.string());
  }
  output << contents;
  if (!output) {
    throw std::runtime_error("Failed to write file: " + path.string());
  }
}

std::string makeGameConfig(const std::string& game_name) {
  return "config_file_ver = \"1.0\"\n"
         "chaos_toml = \"main\"\n"
         "game = \"" + game_name + "\"\n";
}

}  // namespace

int main() {
  bool ok = true;
  std::filesystem::path temp_root;

  try {
    temp_root = makeTempDir();
    const std::filesystem::path game_dir = temp_root / "games";
    const std::filesystem::path log_dir = temp_root / "logs";
    std::filesystem::create_directories(game_dir);
    std::filesystem::create_directories(log_dir);

    writeTextFile(game_dir / "alpha.toml", makeGameConfig("Duplicate Game"));
    writeTextFile(game_dir / "beta.toml", makeGameConfig("Duplicate Game"));
    writeTextFile(game_dir / "unique.toml", makeGameConfig("Unique Game"));

    const std::string main_config =
        "config_file_ver = \"1.0\"\n"
        "chaos_toml = \"main\"\n"
        "log_directory = \"" + log_dir.string() + "\"\n"
        "log_file = \"chaos.log\"\n"
        "game_directory = \"" + game_dir.string() + "\"\n"
        "default_game = \"alpha.toml\"\n";
    const std::filesystem::path config_path = temp_root / "chaosconfig.toml";
    writeTextFile(config_path, main_config);

    Configuration configuration(config_path.string());
    const auto& available_games = configuration.getAvailableGames();
    ok &= check(available_games.size() == 3, "should discover all playable game configs");

    std::unordered_map<std::string, std::string> discovered;
    for (const auto& entry : available_games) {
      discovered[entry.first] = entry.second;
    }

    ok &= check(discovered.count("Duplicate Game") == 0,
                "duplicate game names should be disambiguated");
    ok &= check(discovered.count("Duplicate Game [alpha.toml]") == 1,
                "first duplicate should include its file name");
    ok &= check(discovered.count("Duplicate Game [beta.toml]") == 1,
                "second duplicate should include its file name");
    ok &= check(discovered.count("Unique Game") == 1,
                "unique game names should remain unchanged");
  } catch (const std::exception& err) {
    std::cerr << "FAIL: unexpected exception: " << err.what() << '\n';
    ok = false;
  }

  if (!temp_root.empty()) {
    std::error_code cleanup_err;
    std::filesystem::remove_all(temp_root, cleanup_err);
  }

  if (!ok) {
    return 1;
  }

  std::cout << "PASS: duplicate game names are disambiguated with file names\n";
  return 0;
}
