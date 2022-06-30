#include <string>
#include <iostream>
#include <GameMenu.hpp>

int main(int argc, char const *argv[])
{
  if (argc < 2) {
    std::cerr << "Please specify a configuration file";
  }
  std::string configfile = argv[1];

  return 0;
}
