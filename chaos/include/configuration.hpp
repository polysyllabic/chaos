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

#include "controllerInput.hpp"
#include "sequence.hpp"
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

    std::filesystem::path game_config; 
    std::filesystem::path log_path;
    unsigned int usleep_interval;

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
    const std::string& getGameFile() { return game_config.string(); }


    
    /**
     * \brief Adds a list of objects to a map.
     *
     * \param[in] config The table to process.
     * \param[in] key The key whose presence we're testing for
     * \param[in,out] map The map that we're building.
     * \param[in] defs The master list of defined objects
     *
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
     * \brief Adds a list of objects to a vector or accept the 'ALL' keyword.
     *
     * \param config The table to process.
     * \param key The key whose presence we're testing for
     * \param vec The vector that we're building.
     *
     * \return true if the value for the key was the special keyword 'ALL'
     * \return false if the value for the key was an array
     *
     * \throws std::runtime_error Throws an error if the object we're adding has not been defined.
     *
     * As an alternative to specifying an array as the value for the key, the user can specify
     * 'ALL' as a simple string value (not in an array). The return value indicates whether this
     * shortcut was used. If 'ALL' is detected, the vector will be empty.
     *
     * The templated class T must provide an accessor function, <T>::get(const std::string& name),
     * which returns a shared pointer to the class object, if it is defined for the given string.
     */
    template<typename T>
    static bool addToVectorOrAll(const toml::table& config, const std::string& key,
        std::vector<std::shared_ptr<T>>& vec) {
      std::optional<std::string_view> for_all = config[key].value<std::string_view>();
      if (for_all && *for_all == "ALL") {
        vec.clear();
        PLOG_VERBOSE << key << ": ALL";
        return true;
      } else {
        addToVector(config, key, vec);
      }
      return false;
    }

    /**
     * \brief Adds a list of objects to a vector.
     *
     * \param config The table to process.
     * \param key The key whose presence we're testing for
     * \param vec The vector that we're building.
     *
     * \throws std::runtime_error Throws an error if the object we're adding has not been defined.
     *
     * The templated class T must provide an accessor function, <T>::get(const std::string& name),
     * which returns a shared pointer to the class object, if it is defined for the given string.
     */
    template<typename T>
    static void addToVector(const toml::table& config, const std::string& key,
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
	          PLOG_VERBOSE << "Added '" + *cmd + "' to the " + key + " vector.";
	        } else {
	          throw std::runtime_error("Unrecognized object '" + *cmd + "' in " + key);
      	  }
	      }
      }      
    }
  
    template<typename T>
    static std::shared_ptr<T> getObject(const toml::table& config, const std::string& key, bool required) {
      std::optional<std::string> obj = config[key].value<std::string>();
      if (obj) {
        return T::get(*obj);
      } else {
        if (required) {
          PLOG_ERROR << "Missing required value for the key '" << key << "'\n";
        }
      }
      return nullptr;
    }
  };
};
