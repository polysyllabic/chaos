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
#include <string>
#include <optional>
#include <string_view>

#include <toml++/toml.h>

namespace Chaos {

  /**
   * \brief Parse TOML file and dispatch results to other classes.
   *
   * This class provides a central place to define all the parameters that we support through
   * the TOML file.
   */
  class TOMLReader {

  private:
    /**
     * The bsae table containing the parsed configuration file.
     */
    toml::table configuration;
    std::optional<std::string_view> version;
    /**
     * The name of the game this TOML file supports.
     */
    std::optional<std::string_view> game;
  public:
    /**
     * \brief The constructor for the reader.
     *
     * The constructor takes a string pointing to a file path+name and performs the basic parsing.
     * The TOML tables. The results are then available for query by other classes.
     */
    TOMLReader(const std::string& fname);

    /**
     * Returns a reference to the root TOML table.
     */
    inline const toml::table& getConfig() { return configuration; }

    std::string_view getGameName();

    /**
     * \brief Get the version of the TOML format used in the configuration file.
     * \return The version of the 
     */
    std::string_view getVersion();
  };

};
