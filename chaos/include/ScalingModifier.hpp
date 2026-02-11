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
#pragma once
#include <string>
#include <toml++/toml.h>

#include "DeviceEvent.hpp"
#include "Modifier.hpp"
#include "EngineInterface.hpp"

namespace Chaos {
  class Game;
  /** 
   * \brief A modifier that modifies incomming signals according to a linear formula.
   *
   * The incoming signal, x, will be transformed to $amplitude * x + offset$ and clipped to the
   * min/max values of the signal.
   * 
   * The TOML syntax defining menu items is described in chaosConfigFiles.md
   */
  class ScalingModifier : public Modifier::Registrar<ScalingModifier> {
  private:
    float amplitude;
    float offset;
    short sign_tweak;
    
  public:
    
    ScalingModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    bool tweak(DeviceEvent& event);

  };
};

