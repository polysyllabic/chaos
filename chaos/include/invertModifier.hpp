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

#include "modifier.hpp"

namespace Chaos {
  class Game;
  /** 
   * \brief A modifier that inverts an axis-based command.
   *
   * The following fields are used to define an invert modifier in the TOML file:
   *
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "cooldown" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - appliesTo: A commands affected by the mod. (_Required_)
   * - type = "disable"
   * - disableOnStart: A boolean value that, if true, sends a disable signal to any commands in the
   * appliesTo list during the begin() routine. (_Optional_)
   * - disableOnFinish: A boolean value that, if true, sends a disable signal to any commands in the
   * appliesTo list during the finish() routine. (_Optional_)
   */
  class InvertModifier : public Modifier::Registrar<InvertModifier> {

  public:
    static const std::string mod_type;
    
    InvertModifier(toml::table& config, Game& game);

    void begin();
    void finish();
    bool tweak(DeviceEvent& event);

  };
};

