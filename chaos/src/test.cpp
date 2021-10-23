#include <string_view>
#include <iostream>
#include <unordered_map>
#include <toml++/toml.h>
#include <fstream>
#include <plog/Log.h>

#include "gameCommand.hpp"

using namespace Chaos;
using namespace std::literals::string_view_literals;

int main(int argc, char** argv) {

  // Read the toml configuration file
  TOMLReader config = new TOMLReader("../examples/tlou2.toml");

  std::cout << "Game: " << config.getGameName() << "\n";

  std::unordered_map<std::string, std::string> mod_list;
  
  auto mods = config["modifier"];
  if (! mods) {
    std::cerr << "TOML file contains no modifier definitions!\n";
    return -1;
  }
  // should have an array of tables, each one defining an individual modifier
  toml::array* arr = mods.as_array();
  if (! arr) {
    std::cerr << "Modifiers should be defined as an array of tables, i.e., start with [[modifier]]\n";
    return -1;
  }
  for (toml::node& elem : *arr) {
    // grab the table for the individual modifier
    toml::table* modifier = elem.as_table();
    if (! modifier) {
      std::cerr << "Modifier definition must be a table\n";
      continue;
    }
    std::cout << "Table: " << *modifier << "\n";
    if (!modifier->contains("name")) {
      std::cerr << "Modifier missing required 'name' field: " << *modifier << "\n";
      continue;
    }
    std::optional<std::string> mod_name = modifier->get("name")->value<std::string>();
    if (mod_list.contains(*mod_name)) {
      std::cerr << "The modifier '" << *mod_name << " has already been defined\n";
      continue;
    }
    if (!modifier->contains("type")) {
      std::cerr << "Modifier missing required 'type' field: " << *modifier << "\n";
      continue;
    }
    std::optional<std::string> mod_type = modifier->get("type")->value<std::string>();
    // screen for type here
    std::cout << "Creating modifier '" << *mod_name << "' of type '" << *mod_type << "'\n";
    
    std::optional<std::string> description = modifier->get("description")->value<std::string>();
    if (description) {
      std::cout << " -- " << *description << "\n";
    }
    std::vector<std::shared_ptr<GameCommand>> commands;
    if (modifier->contains("appliesTo")) {
      if (toml::array* cmd_list = modifier->get("appliesTo")->as_array()) {
	if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
	  std::cout << "  appliesTo must be an array of strings!\n";
	}
	for (auto& elem : *cmd_list) {
	  std::optional<std::string> cmd = elem.value<std::string>();
	  if (cmd) {
	    if (GameCommand::bindingMap.contains(*cmd)) {
	      commands.push_back(GameCommand::bindingMap[*cmd]);
	      std::cout << "   applies to: " << *cmd << std::endl;
	    } else {
	      std::cout << "*** unrecognized command: " << *cmd << std::endl;
	    }
	  }
	}
      } else {
	std::cerr << "*** ->  appliesTo must be an array!\n";
      }
    }
    if (modifier->contains("begin")) {
      auto b = modifier->get("begin");
      std::cout << "BEGIN = " << b.value();
    }
  }
}
