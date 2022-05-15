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

namespace Chaos {

  /**
   * \brief A class to handle velocity calculations from the touchpad
   * 
   * To translate touchpad input into axes in a smooth way, we calculate the velocity with which the
   * user moves his or her finger over the touchpad. This class encapsulates those calculations.
   */
  class Touchpad {
  private:
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

    /**
     * \brief Update the derivative 
     * 
     * \param d 
     * \param current 
     * \param timestamp 
     * \return double 
     */
    double derivative(DerivData* d, short current, double timestamp);

    Mogi::Math::Time timer;
    
  public:
    Touchpad();

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

    /**
     * \brief Get the velocity of the touchpad signal change
     * 
     * \param tp_axis The touchpad axis to test
     * \param value The reported event value for this signal
     * \return short The velocity along this axis
     * 
     * The velocity is tracked by taking a running average of the differences in signal value over
     * the last 5 calls to the function.
     */
    short getVelocity(ControllerSignal tp_axis, short value);
    
  };
  
};
