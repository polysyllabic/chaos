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
#include <queue>
#include <toml++/toml.h>

#include "modifier.hpp"

namespace Chaos {

  /**
   * \brief Modifier that allows an action for a set period of time and then blocks it until a
   * cooldown period has expired.
   */
  class CooldownModifier : public Modifier::Registrar<CooldownModifier> {

  public:
    static const std::string name;
    CooldownModifier(Controller* controller, ChaosEngine* engine, const toml::table& config);
    
    void update();
  };
};

