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
#include <queue>
#include <memory>
#include <toml++/toml.h>
#include <timer.hpp>

#include "Modifier.hpp"
#include "Game.hpp"

namespace Chaos {

  /**
   * \brief Modifier that allows an action for a set period of time and then blocks it until a
   * cooldown period has expired.
   *
   * Example TOML definition:
   * [[modifier]]
   * name = "Bad Stamina"
   * description = "Running is disabled after 2 seconds and takes 4 seconds to recharge."
   * type = "cooldown"
   * groups = [ "movement" ]
   * appliesTo = [ "dodge/sprint" ]
   * timeOn = 2.0
   * timeOff = 4.0
   *
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "cooldown" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - appliesTo: A commands affected by the mod. (_Required_)
   * - timeOn: Length of time to allow the command. (_Required_)
   * - timeOff: Length of time spent in cooldown, where the command is blocked. (_Required_)
   *
   * Note: Although the appliesTo key takes an array, this mod type expects only a single
   * command. All but the first will be ignored.
   * 
   * \todo Allow cooldown to do things other than block a signal
   */
  class CooldownModifier : public Modifier::Registrar<CooldownModifier> {

  protected:
    DeviceEvent cooldownCommand;
    bool pressedState;
    double cooldownTimer;
    bool inCooldown;
    
    /**
     * Time that the event is allowed before we block it
     */
    double time_on;
    /**
     * Time that the event is held in cooldown before re-enabled.
     */
    double time_off;

  public:
    CooldownModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    void begin();
    void update();
    bool tweak(DeviceEvent& event);
  };
};

