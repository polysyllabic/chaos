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
