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
#include "menuModifier.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

const std::string MenuModifier::name = "menu";

MenuModifier::MenuModifier(toml::table& config) {
  TOMLReader::checkValid(config, std::vector<std::string>{
    "name", "description", "type", "groups", "begin", "finish", "beginSequence", "finishSequence"});

  initialize(config);

  TOMLReader::buildMenuCommand(config, "begin", begin_menu);
  
  TOMLReader::buildMenuCommand(config, "finish", finish_menu);

}

void MenuModifier::begin() {
  // Perform any non-menu-related actions
  sendBeginSequence();

  // execute all menu commands in the list
  for (auto& m : begin_menu) {
    m.doAction();
  }
}

void MenuModifier::finish() {
  sendFinishSequence();

  for (auto& m : finish_menu) {
    m.doAction();
  }
}

bool MenuModifier::tweak(DeviceEvent* event) {
  // block all signals if we're busy sending a sequence
  return !in_sequence;
}

