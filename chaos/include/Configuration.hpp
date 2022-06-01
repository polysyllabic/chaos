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
#pragma once
#include <filesystem>
#include <string>
#include <optional>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <unordered_map>

#include <plog/Log.h>
#include <toml++/toml.h>

#include "ControllerInput.hpp"
#include "Sequence.hpp"
#include "enumerations.hpp"

namespace Chaos {

  /**
   * \brief Parse TOML file and dispatch results to other classes.
   *
   * This class provides a central place to define all the parameters that we support through
   * the TOML file. It also provides templated static helper routines to simplify common
   * parsing tasks that the modifiers need to do.
   */
  class Configuration {

  private:
    std::string toml_version;
    std::filesystem::path game_config; 
    std::filesystem::path log_path;
    unsigned int usleep_interval;
    std::string interface_addr;
    unsigned int interface_port;
    unsigned int listener_port;

  public:
    /**
     * \brief The constructor for the reader.
     *
     * \param[in] fname The fully qualified file name of the file to be parsed.
     *
     * The constructor performs the basic parsing and creates a toml::table object containing the
     * completely parsed file, which is then available for other initialization routines.
     */
    Configuration(const std::string& fname);

    /**
     * \brief Get the default game configuration file
     * 
     * \return const std::string& 
     * 
     * This is the file we try to load if none is specified on the command line
     */
    std::string getGameFile() { return game_config.string(); }

    /**
     * \brief Get the TCP address of the machine running the python interface
     * 
     * \return std::string 
     * 
     * The address and port are separately configurable through the chaosconfig.toml TOML file.
     * The default address is localhost, and it should be left unchanged if the interface and
     * engine are both running on the Pi.
     */
    std::string getInterfaceAddress() { return "tcp://" + interface_addr + ":" +
                                          std::to_string(interface_port); }

    /**
     * \brief Get the TCP endpoint to listen for incomming messages from the python interface
     * 
     * \return std::string 
     * 
     * The port that the socket listens on can be configured in the configuration TOML file.
     * We listen to all local IP addresses on this port.
     */
    std::string getListenerAddress() { return "tcp://*:" + std::to_string(listener_port); }

    bool matchVersion(const std::string& ver) { return toml_version == ver; }
  };
};
