/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file at the top-level directory of this distribution for details of the
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

#include <toml++/toml.h>
#include <mogi/math/systems.h>

#include "controllerInput.hpp"

namespace Chaos {

  class GameCondition;

  /**
   * \brief A class to encapulate the touchpad functionality on the controller.
   */
  class Touchpad {
  private:
    /**
     * \brief The default scaling applied to convert touchpad input to axis events.
     * 
     * The touchpad parameters control the formula to convert from touchpad to axis events based on
     * the change in axis value over time (derivative). The formula is scale * derivative + skew.
     * This result is then clipped to the limits of the joystick value. This is the default scaling
     * factor applied if there is no touchpad condition or that condition is false.
     */
    double scale;

     /**
      * \brief Condition under which to use an alternate scaling factor
      *
      * If the is defined, the engine will check the if this condition is currently met, and if it is,
      * the derivative will be multiplied by the value of touchpad_scale_if instead of the default.
      */
    std::shared_ptr<GameCondition> condition;

    /**
     * \brief The alternate scaling applied to convert touchpad input to axis events.
     * 
     * The touchpad parameters control the formula to convert from touchpad to axis events based on
     * the change in axis value over time (derivative). The formula is scale * derivative + skew.
     * This result is then clipped to the limits of the joystick value. This is the alternate scaling
     * factor applied if the ouchpad condition is true.
     */
    double scale_if;

    /**
     * \brief Offset to apply to the axis value when the derivative is non-zero.
     * 
     * The sign of the skew is the same as the sign of the derivative. In other words, if the derivative
     * is positive, the skew is added to the result, and the derivative is negative, the skew is subtracted.
     */
    short skew;
    
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

    bool inCondition();

    short toAxis(ControllerSignal signal, short value);
  };
  
};
