/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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

#include <toml++/toml.h>
#include <mogi/math/systems.h>

#include "gamepadInput.hpp"
#include "gameCommand.hpp"

namespace Chaos {

  /**
   * \brief A class to encapulate the touchpad functionality on the controller.
   */
  class Touchpad {
    /**
     * The scaling of the touchpad can be affected by a game condition, such as whehter or not the
     * user is aiming. If there is no condition, this will be set to GPInput::NONE.
     */
    std::shared_ptr<GameCommand> condition;
    double scale;
    double scale_if;
    int skew;
    
    /**
     * Remembers whether touchpad is currently active
     */
    bool active;
    /**
     * Signal that the ordinary axes require disabling.
     */
    bool disableAxes;
    
    typedef struct _DerivData {
      short prior[5];
      double timestampPrior[5];
      bool priorActive;
    } DerivData;

    /**
     * State information to track the x-component of the distance a finger has travelled on the
     * touchpad between samples of the touchpad.
     */
    DerivData dX;
    /**
     * State information to track the y-component of the distance a finger has travelled on the
     * touchpad.
     */
    DerivData dY;
    /**
     * State information to track the x-component of the distance a second finger has travelled on
     * the touchpad. TLOU 2 doesn't use this, and I don't actually know if it will be useful for
     * anything else, but I'm including it just in case.
     */
    DerivData dX_2;
    /**
     * State information to track the y-component of the distance a second finger has travelled on
     * the touchpad. TLOU 2 doesn't use this, and I don't actually know if it will be useful for
     * anything else, but I'm including it just in case.
     */
    DerivData dY_2;

    double derivative(DerivData* d, short current, double timestamp);

    Mogi::Math::Time timer;
    
  public:
    /**
     * Initialize touchpad parameters from the TOML file.
     */
    void initialize(const toml::table& config);
    
    static Touchpad& instance() {
      static Touchpad touchpad{};
      return touchpad;
    }

    /**
     * \brief Reset touchpad's priorActive status 
     */
    void clearActive();

    /**
     * Query whether the touchpad is currently in use
     */
    bool isActive() { return active; }

    /**
     * Set whether the touchpad is currently in use.
     */
    void setActive(bool state) { active = state; }

    std::shared_ptr<GameCommand> getCondition() { return condition; }
    
    int toAxis(const DeviceEvent& event, bool inCondition);
  };
  
};
