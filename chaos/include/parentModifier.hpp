/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
#include <vector>
#include <string>
#include <toml++/toml.h>

#include "modifier.hpp"
#include "chaosEngine.hpp"

namespace Chaos {

  /**
   * A modifier that enables child modifiers, either from a specific list or randomly selected.
   */
  class ParentModifier : public Modifier::Registrar<ParentModifier> {
  protected:
    std::vector<Modifier*> children;
    
  public:
    static const std::string name;
    
    ParentModifier(const toml::table& config);

    void begin();

  };
};
