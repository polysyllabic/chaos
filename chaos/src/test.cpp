#include <string_view>
#include <iostream>

#include <toml++/toml.h>
#include <fstream>

#include "gameCommand.hpp"

using namespace Chaos;
using namespace std::literals::string_view_literals;

int main(int argc, char** argv) {
  ControllerCommand::initialize();
  // Read the toml configuration file
  toml::table config;
  try {
    std::cout << "Reading TOML file\n";
    config = toml::parse_file("../examples/tlou2.toml");
  }
  catch (const toml::parse_error& err) {
    std::cerr << "Parsing the configuration file failed:" << std::endl << err << std::endl;
    exit (EXIT_FAILURE);
  }
  std::optional<std::string_view> toml_version = config["chaos_toml"].value<std::string_view>();
  if (!toml_version) {
    std::cerr << "Could not find version id (chaos_toml). Are you sure this is the right file?\n";
    exit(EXIT_FAILURE);
  }
  std::string_view chaos_game = config["game"].value_or("game unknown");

  std::cout << "TOML format v" << *toml_version << "\n";
  std::cout << "Game: " << chaos_game << "\n";

  // Assign the default mapping of commands to button/joystick presses
  // The TOML file defines bindings as an array of tables, each table containing one
  // command-to-controll mapping.
  GameCommand::initialize(config);
  
}
