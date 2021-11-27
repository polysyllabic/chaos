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
#include <vector>
#include <stdexcept>
#include <unordered_map>

#include <plog/Log.h>
#include <toml++/toml.h>

#include "gamepadInput.hpp"

namespace Chaos {

  /**
   * \brief Parse TOML file and dispatch results to other classes.
   *
   * This class provides a central place to define all the parameters that we support through
   * the TOML file. It also provides templated static helper routines to simplify common
   * parsing tasks that the modifiers need to do.
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
     * \param[in] fname The fully qualified file name of the file to be parsed.
     *
     * The constructor performs the basic parsing and creates a toml::table object containing the
     * completely parsed file.
     */
    TOMLReader(const std::string& fname);

    /**
     * \brief Get the object containing the parsed TOML information.
     * \return A reference to the root TOML table.
     */
    inline const toml::table& getConfig() { return configuration; }

    std::string_view getGameName();

    /**
     * \brief Get the version of the TOML format used in the configuration file.
     * \return The version of the 
     */
    std::string_view getVersion();

    static GPInput getSignal(const toml::table& config, const std::string& key, bool required);
    
    /**
     * \brief Adds a list of objects to a map.
     * \param[in] config The table to process.
     * \param[in] key The key whose presence we're testing for
     * \param[in,out] map The map that we're building.
     * \param[in] defs The master list of defined objects
     * \throws std::runtime_error Throws an error if the object we're adding has not been defined.
     */
    template<typename T>
    static void addToMap(const toml::table& config,
			 const std::string& key,
			 std::unordered_map<std::string, T>& map,
			 const std::unordered_map<std::string, T>& defs) {
      
      // This entity is assumed to be optional, so do nothing if the key isn't present
      if (config.contains(key)) {
	const toml::array* cmd_list = config.get(key)->as_array();
	if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
	  throw std::runtime_error(key + " must be an array of strings");
	}
	for (auto& elem : *cmd_list) {
	  std::optional<std::string> cmd = elem.value<std::string>();
	  // should be impossible to get here with null cmd because of homogeneous test above
	  assert(cmd);

	  // check that the string matches the name of a previously defined object
	  if (defs.count(*cmd) == 1) {
	    map[*cmd] = defs[*cmd];
	    PLOG_VERBOSE << "Added '" + *cmd + "' to the " + key + " map" << std::endl;
	  } else {
	    throw std::runtime_error("unrecognized object '" + *cmd + "' in " + key);
	  }
	}
      }      
    }
    
    /**
     * \brief Adds a list of objects to a vector.
     * \param[in] config The table to process.
     * \param[in] key The key whose presence we're testing for
     * \param[in,out] vec The vector that we're building.
     * \throws std::runtime_error Throws an error if the object we're adding has not been defined.
     *
     * The templated class T must provide an accessor function, <T>::get(const std::string& name),
     * which returns a shared pointer to the class object, if it is defined.
     */
    template<typename T>
    static void addToVector(const toml::table& config,
			    const std::string& key,
			    std::vector<std::shared_ptr<T>>& vec) {
      
      // This entity is assumed to be optional, so do nothing if the key isn't present
      if (config.contains(key)) {
	const toml::array* cmd_list = config.get(key)->as_array();
	if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
	  throw std::runtime_error(key + " must be an array of strings");
	}
	
	for (auto& elem : *cmd_list) {
	  std::optional<std::string> cmd = elem.value<std::string>();
	  // should be impossible to get here with null cmd because of homogeneous test above
	  assert(cmd);

	  // check that the string matches the name of a previously defined object
	  std::shared_ptr<T> defs = T::get(*cmd);
	  if (defs) {
	    vec.push_back(defs);
	    PLOG_VERBOSE << "Added '" + *cmd + "' to the " + key + " vector." << std::endl;
	  } else {
	    throw std::runtime_error("unrecognized object '" + *cmd + "' in " + key);
	  }
	}
      }      
    }
  };
  
};
