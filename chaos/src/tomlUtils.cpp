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
#include <algorithm>
#include <plog/Log.h>
#include "tomlUtils.hpp"
#include "controllerInput.hpp"

using namespace Chaos;

bool checkValid(const toml::table& config, const std::vector<std::string>& goodKeys,
          const std::string& name) {
  bool ret = true;
  for (auto&& [k, v] : config) {
    if (std::find(goodKeys.begin(), goodKeys.end(), k) == goodKeys.end()) {
      PLOG_WARNING << "The key '" << k << "' is unused in " << name;
      ret = false;
    }
  }
  return ret;
}


ControllerSignal getSignal(const toml::table& config, const std::string& key, bool required) {
  if (! config.contains(key)) {
    if (required) {
      throw std::runtime_error("missing required '" + key + "' field");
    }
    return ControllerSignal::NONE;
  }
  std::optional<std::string> signal = config.get(key)->value<std::string>();
  std::shared_ptr<ControllerInput> inp = ControllerInput::get(*signal);
  if (! inp) {
    throw std::runtime_error("Controller signal '" + *signal + "' not defined");
  }
  return inp->getSignal();
}


