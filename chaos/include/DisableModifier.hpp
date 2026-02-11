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

#include "Modifier.hpp"
#include "Game.hpp"

namespace Chaos {

  enum class DisableFilter { ALL, ABOVE, BELOW};

  /**
   * \brief A modifier that disables a particullar list of commands from being passed to the
   * controller.
   *
   * By default, we completely block the relevant signals, i.e., set them to 0. If a filter
   * is set, we only block negative or positive values, which allows us to stop movement in
   * one direction while allowing the other.
   */
  class DisableModifier : public Modifier::Registrar<DisableModifier> {
  protected:
    /**
     * What type of filtering to apply to the signal
     */
    DisableFilter filter;

  public:
    
    /**
     * \brief The public constructor
     * \param config A TOML modifier-table object. If the constructor is properly dispatched, this
     *        object will contain the key/value pair 'type=disable'.
     * \param e Engine interface
     */
    DisableModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    // virtual routines we need to override
    bool tweak(DeviceEvent& event);

  };
};
