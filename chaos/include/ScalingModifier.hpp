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
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "scaling" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - appliesTo: A commands affected by the mod. (_Required_)
   * - amplitude: Amount to multiply the incoming signal by (_Optional. Default = 1_)
   * - offset: Ammount to add to the incomming signal (_Optional. Default = 0_)
   * - beginSequence: A sequence of button presses to apply during the begin() routine. (_Optional_)
   * - finishSequence: A sequence of button presses to apply during the finish() routine. (_Optional_)
   * - unlisted: A boolian that, if true, will cause the mod not to be reported to the chaos interface (_Optional_)
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

