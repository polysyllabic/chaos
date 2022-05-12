/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
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

#include "subMenu.hpp"
#include "tomlUtils.hpp"

using namespace Chaos;

SubMenu::SubMenu(const toml::table& config) : MenuItem(config) {
  checkValid(config, std::vector<std::string>{
      "name", "type", "parent", "offset", "initialState", "hidden", "confirm", "tab", "counterAction"});
}

void SubMenu::setState(Sequence& seq, unsigned int new_val) {
  PLOG_ERROR << "Cannot set the state of a submenu";
}

void SubMenu::restoreState(Sequence& seq) {
  PLOG_ERROR << "Cannot restore the state of a submenu";
}
