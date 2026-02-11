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
   * \brief Enumeration of possible cooldown states
   * 
   * UNTRIGERED: The trigger condition for the monitored action hasn't occurred yet.
   * ALLOW: The action has been triggered and we're allowing it for a set period of time.
   * BLOCK: The allow-time has expired and we are in the cooldown period.
   */
  enum class CooldownState { UNTRIGGERED, ALLOW, BLOCK };

  /**
   * \brief Modifier that allows an action for a set period of time and then blocks it until a
   * cooldown period has expired.
   *
   * \todo Allow cooldown to do things other than block a signal
   */
  class CooldownModifier : public Modifier::Registrar<CooldownModifier> {

  private:
    /**
     * Timer to track the cooldown cycle. While we're in the ALLOW phase, this timer
     * increments until it hits the time_on limit. While we're in the BLOCK phase, this
     * timer decrements until it hits 0, and then the cooldown is reset to INACTIVE.
     */
    double cooldown_timer;

    /**
     * Current state of the cooldown
     */
    CooldownState state;

    /**
     * If true, the allow-period only accumulates when the while-condition is true.
     * If false, the cycle occurs whether or not a condition applies.
     */
    bool cumulative;

    /**
     * Time that the event is allowed before we block it
     */
    double time_on;

    /**
     * Time that the event is held in cooldown before re-enabled.
     */
    double time_off;

    /**
     * Vector of signals that will start the trigger
     */
    std::vector<std::shared_ptr<ControllerInput>> trigger;

  public:
    CooldownModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    void begin();
    void update();
    bool tweak(DeviceEvent& event);
  };
};

