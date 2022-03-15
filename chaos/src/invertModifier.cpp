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
#include <stdexcept>
#include <plog/Log.h>
#include <toml++/toml.h>

#include "invertModifier.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

const std::string InvertModifier::name = "invert";

InvertModifier::InvertModifier(toml::table& config) {
  TOMLReader::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "appliesTo", "disableOnStart", "disableOnFinish"});
  initialize(config);

  if (commands.empty()) {
    throw std::runtime_error("No commands defined in appliesTo");
  }

}

void InvertModifier::begin() {  
  if (disable_on_begin) {
    for (auto& cmd : commands) {
      Controller::instance().setOff(cmd);
    }
  }
}

void InvertModifier::finish() {  
  if (disable_on_finish) {
    for (auto& cmd : commands) {
      Controller::instance().setOff(cmd);
    }
  }
}

bool InvertModifier::tweak(DeviceEvent& event) {
  // Traverse the list of affected commands
  for (auto& cmd : commands) {
    if (Controller::instance().matches(event, cmd)) {
      event.value = -((int)event.value+1);
      break;
    }
  }
  return true;
}

