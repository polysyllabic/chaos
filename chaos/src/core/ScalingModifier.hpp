/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributors.
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
   * \brief A modifier that modifies incoming signals according to a linear formula.
   *
   * The incoming signal, x, will be transformed to $amplitude * x + offset$ and clipped to the
   * min/max values of the signal.
   * 
   * The TOML syntax defining menu items is described in chaosConfigFiles.md
   */
  class ScalingModifier : public Modifier::Registrar<ScalingModifier> {
  private:
    double amplitude;
    double while_amplitude;
    double offset;
    
  public:
    
    /**
     * \brief Construct a scaling modifier from TOML configuration.
     *
     * \param config Modifier configuration table.
     * \param e Engine interface pointer.
     */
    ScalingModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;

    /**
     * \brief Return this modifier's registered factory type name.
     */
    const std::string& getModType() { return mod_type; }

    /**
     * \brief Apply linear scaling to matching event values.
     *
     * \param event Event to transform.
     * \return true if event should continue through pipeline.
     */
    bool tweak(DeviceEvent& event);

  };
};
