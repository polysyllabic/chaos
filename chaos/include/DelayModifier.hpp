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
#include <toml++/toml.h>
#include <timer.hpp>

#include "Modifier.hpp"
#include "EngineInterface.hpp"

namespace Chaos {

  typedef struct _TimeAndEvent{
    double time;
    DeviceEvent event;
  } TimeAndEvent;

  /**
   * \brief Subclass of modifiers that introduce a delay between a button press and when it is
   * passed to the controller.
   *
   * Example TOML definition:
   *     [[modifier]]
   *     name = "Shooting Delay"
   *     description = "Introduces a 0.5 second delay to shooting and throwing"
   *     type = "delay"
   *     groups = [ "debuff", "combat" ]
   *     appliesTo = [ "shoot/throw" ]	
   *     delay = 0.5
   *
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "delay" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - appliesTo: A list of commands affected by the mod. (_Required_)
   * - delay: Time in seconds to delay the listed commands. (_Required_)
   *
   */
  class DelayModifier : public Modifier::Registrar<DelayModifier> {
  protected:
    std::queue<TimeAndEvent> eventQueue;
    double delayTime;
    
  public:
    
    DelayModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    void update();
    bool tweak(DeviceEvent& event);
  };
};

