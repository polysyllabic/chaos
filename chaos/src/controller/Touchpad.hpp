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
#include <timer.hpp>
#include "signals.hpp"

namespace Chaos {

  /**
   * \brief A class to handle calculations from the touchpad
   * 
   * The touchpad can be used to generate axis events based either on the distance travelled from the
   * first touch point or the change in the touchpad axis values over time. This class encapsulates
   * those calculations.
   * 
   * There are two possible formula that can be used to calculate the axis equivalent from the touchpad:
   * velocity and relative distance.
   * 
   * The velocity formula measures the average speed that the finger has travelled over the touchpad
   * over the touchpad for the previous 5 polling intervals in both the x and y axes. 
   *
   * The distance formula returns the difference in position between where the finger first touched
   * down on the touchpad and wher it is now.
   * 
   * In both cases, that resulting value is multiplied by a scale factor (configurable separately for
   * both the x and y axes) and a skew is added. The skew is meant to amplify the signal linearly in
   * whatever direction the basic signal is. That is, if the returned value is negative, the skew is
   * subracted; if it is positive, the skew is added.
   */
  class Touchpad {
  private:
    /**
     * Remembers whether touchpad is currently active
     */
    bool active;

    typedef struct _DerivData {
      short prior[5];
      double timestampPrior[5];
      bool priorActive;
    } DerivData;

    /**
     * State information to track the x-component of the distance a finger has travelled on the
     * touchpad between samples of the touchpad. Currently we only track finger 1.
     */
    DerivData dX;
    /**
     * State information to track the y-component of the distance a finger has travelled on the
     * touchpad. Currently we only track finger 1.
     */
    DerivData dY;

    /**
     * \brief Update the change in distance since the last sample 
     * 
     * \param d State information for the relevant axis
     * \param current new touchpad axis value
     * \param timestamp Time called
     * \return Average difference between samples over the last five samples
     */
    double derivative(DerivData* d, short current, double timestamp);

    /**
     * \brief Update the relative distance since the first finger touch
     * 
     * \param d State information for the relevant axis
     * \param current new touchpad axis value
     * \param timestamp Time called
     * \return Distance between most recent position and the location of the first point touched
     */
    double distance(DerivData* d, short current, double timestamp);

    Timer timer;

    /**
     * Should we use finger velocity to calculate an axis value?
     */
    static bool use_velocity;

    /**
     * Scale applied to the x axis when converting from touchpad to axis values
     */
    static double scale_x;

    /**
     * Scale applied to the y axis when converting from touchpad to axis values
     */
    static double scale_y;

    /**
     * \brief Offset to apply to the axis value when the axis calculation is non-zero.
     * 
     * This value reflects the minimum value that will be applied when there is any non-zero value from the
     * touchpad calculation. The sign of the skew is the same as the sign of the calculated touchpad value.
     * This serves to ensure that the resulting axis signal is large enough to escape from the dead zone.
     */
    static short skew;

  public:
    Touchpad();

    /**
     * \brief Call when touchpad is newly active
     * 
     * Resets previous state information.
     */
    void firstTouch();

    bool useVelocity() { return use_velocity; }

    static void setVelocity(bool state) { use_velocity = state; }

    /**
     * Query whether the touchpad is currently in use
     */
    bool isActive() { return active; }

    /**
     * Set whether the touchpad is currently in use.
     */
    void setActive(bool state) { active = state; }

    /**
     * \brief Translates the touchpad value to an axis
     * 
     * \param tp_axis The touchpad axis to process
     * \param value The reported event value for this signal
     * \return The scaled axis equivalent
     * 
     * The value is converted according to the selected method (velocity or displacement).
     */
    short getAxisValue(ControllerSignal tp_axis, short value);

    double getScaleX() { return scale_x; }
    double getScaleY() { return scale_y; }    
    static void setScale(double new_x, double new_y) {
      scale_x = new_x;
      scale_y = new_y;
    }

    short getSkew() { return skew; }
    static void setSkew(short new_skew) { skew = new_skew; }
  };
  
};
