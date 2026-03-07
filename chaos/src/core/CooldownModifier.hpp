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
#include <queue>
#include <memory>
#include <unordered_map>
#include <toml++/toml.h>
#include <timer.hpp>

#include "Modifier.hpp"
#include "Game.hpp"

namespace Chaos {

  /**
   * \brief Enumeration of possible cooldown states
   * 
   * UNTRIGGERED: The trigger condition for the monitored action hasn't occurred yet.
   * ALLOW: The action has been triggered and we're allowing it for a set period of time.
   * BLOCK: The allow-time has expired and we are in the cooldown period.
   */
  enum class CooldownState { UNTRIGGERED, ALLOW, BLOCK };

  /**
   * \brief Defines how the allow period starts and accumulates time.
   *
   * TRIGGER: Once allow begins, the timer advances regardless of while-condition changes.
   * CUMULATIVE: Allow-time only advances while while-condition is true.
   * CANCELABLE: Like cumulative, but if while-condition drops during allow, reset to untriggered.
   */
  enum class CooldownStartType { TRIGGER, CUMULATIVE, CANCELABLE };

  /**
   * \brief Modifier that allows an action for a set period of time and then blocks it until a
   * cooldown period has expired.
   *
   * The trigger option is provided in order to ensure that the first event encountered in the
   * trigger list will trip the timer (assuming that the conditions are also true). Although
   * you can probably achieve similar functionality in most cases using just a while condition,
   * the while conditions are polled continually on a timed loop, whereas each incoming event
   * is checked against the trigger list. This ensures that there's no chance of missing
   * a quickly changing state if the engine for some reason gets bogged down.
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
     * Defines how the allow period behaves while in the ALLOW state.
     */
    CooldownStartType start_type;

    /**
     * Time that the event is allowed before we block it
     */
    double time_on;

    /**
     * Time that the event is held in cooldown before re-enabled.
     */
    double time_off;

    /**
     * Inputs to monitor to that will trigger the timer to start in conjunction with the while
     * conditions.
     */
    std::vector<std::shared_ptr<ControllerInput>> trigger;

    /**
     * Last seen values for blocked commands while in BLOCK state.
     *
     * This allows us to restore held controls when cooldown expires without requiring
     * the user to release and re-press the control.
     */
    std::unordered_map<std::string, short> blocked_command_values;

    void snapshotBlockedCommands();
    void restoreBlockedCommands();

  public:
    CooldownModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    void begin();
    void update();
    bool tweak(DeviceEvent& event);
  };
};
