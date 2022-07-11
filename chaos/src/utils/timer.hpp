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
 * 
 * This file contains code derived from the mogillc/nico library by Matt Bunting, Copyright 2016 by
 * Mogi, LLC and distributed under the LGPL library version 2 license.
 * The original version can be found here: https://github.com/mogillc/nico
 * 
 */
#pragma once
//#include <chrono>
#include <sys/time.h>

//namespace chr = std::chrono;

namespace Chaos {

  //using usec = std::chrono::microseconds;
  using usec = double;

  /**
   * \brief Keeps track of time
   * 
   * This keeps track of time such as running time and change in loop time. This is the Time class
   * from the Mogi librar re-implemented to use chrono.
   */
  class Timer {
  private:
    struct timeval tv;
    double timecycle;
    // chr::time_point<chr::steady_clock, std::chrono::microseconds> timecycle;
    double oldtimecycle;
    // chr::time_point<chr::steady_clock, std::chrono::microseconds> oldtimecycle;
    usec dtime;
    usec runningtime;
  public:
    Timer();

    /**
     * \brief Resets the references for computing running time.
     */
    void reset();

    /**
     * \brief Updates the measured components based on the last call.
     * 
     * This function typically should be called once every loop cycle. Updates internal time
     * values.
     */
    void update();  // updates to get new time values

    /**
     * \brief Starts the internal time measurement.
     * 
     * When the next time update() is called, values are updated based in reference to this call.
     * Initializes the internal values needed for time measurement.
     */
    void initialize();  // self explanatory

    /**
     * \brief Get the running time since initialization.
     * \return The time since initialize() was called.
     */
    usec runningTime();

    /**
     * \brief Returns the last computed difference in time between the most recent update() and the
     * update() prior to that.
     *
     * \return The delta time between the previous two update() calls.
     */
    usec dTime();
  };

};