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
#include <vector>

#include <toml++/toml.h>
#include <plog/Log.h>

#include "signals.hpp"
//#include "sequence.hpp"

namespace Chaos {

  class TOMLUtils {
  public:
  
  /**
   * \brief Test if the table contains any keys other than those listed in the vector.

   * \param config TOML table to test
   * \param goodKeys Vector of strings that are legal keys for this table

   * \return true if all keys are good
   * \return false if any keys do not match an entry in the vector
   *
   * The identity of unknown keys will also be logged. For reporting errors, this version assumes
   * that the table contains a key 'name' that identifies the table.
   */
  static bool checkValid(const toml::table& config, const std::vector<std::string>& goodKeys);

  /**
   * \brief Test if the table contains any keys other than those listed in the vector.
   *
   * \param config TOML table to test
   * \param goodKeys Vector of strings that are legal keys for this table
   * \param name Name of the table (for logging warnings)
   *
   * \return true if all keys are good
   * \return false if any keys do not match an entry in the vector
   *
   * The identity of unknown keys will also be logged. Call this version if #config does not
   * contain a "name" field.
   */
  static bool checkValid(const toml::table& config, const std::vector<std::string>& goodKeys,
                  const std::string& name);

  
  /**
   * \brief Get the value of a number
   * 
   * \tparam T Type of number
   * \param config TOML Table to search
   * \param key Key in the TOML table that has the value we want
   * \param min_val Minimum allowed value for this parameter
   * \param max_val Maximum allowed value for this parameter
   * \param default_val Value to use if the key is not found in the table
   * \return T 
   */
  template<typename T>
  static T getValue(const toml::table& config, const std::string& key, T min_val, T max_val, T default_val) {
    T rval = config[key].value_or(default_val);
    if (rval > max_val) {
      PLOG_ERROR << "Maximum value for '" << key << "' is " << max_val << std::endl;
      rval = max_val;
    } else if (rval < min_val) {
      PLOG_ERROR << "Minimum value for '" << key << "' is " << min_val << std::endl;
      rval = min_val;
    }
    return rval;
  }

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
