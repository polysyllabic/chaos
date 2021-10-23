/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#include "modifier.hpp"

namespace Chaos {

  // subclass of modifiers that select options through the game menu
  class MenuModifier : public Modifier::Registrar<MenuModifier> {
  private:
    bool busy;
    
  public:
    static const std::string name;
    
    MenuModifier(const toml::table& config);
    void begin();
    bool tweak(DeviceEvent* event);
  };
};
