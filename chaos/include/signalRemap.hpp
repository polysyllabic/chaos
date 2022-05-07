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
#include "signals.hpp"

namespace Chaos {

  /**
   * Contains the information to remap signals between the controller and the console.
   */  
  struct SignalRemap {
    /**
     * \brief The input type that the controller to which the input will be altered before it is
     * sent to the console.
     *
     * If no remapping is defined, this will be set to from_controller. ControllerSignal::NOTHING is a
     * remapping that drops the signal. For remapping of an axis to multiple buttons, this contains
     * the remap for positive axis values.
     */
    ControllerSignal to_console;
    /**
     * The remapped control used for negative values when mapping one input onto multiple buttons for
     * output.
     */
    ControllerSignal to_negative;
    /**
     * \brief Proportion of axis signal required to remap
     * 
     * If the signal falls below the threshold proportion, the remapped signal will be 0. This is
     * intended, for example, to prevent tiny movements (or controller drift) from triggering a
     * button press
     */
    short threshold;


    double scale;

    /**
     * If true, button-to-axis remaps go to the joystick minimum. Otherwise they go to the maximum.
     */
    bool to_min;

    /**
     * If true, invert the value on remapping.
     */
    bool invert;
    
    // Constructor
    SignalRemap(ControllerSignal to, ControllerSignal neg_to, bool min, bool inv, short thresh, double sensitivity) :
      to_console(to),
      to_negative(neg_to),
      to_min(min),
      invert(inv),
      threshold(thresh),
      scale(sensitivity) {}
  };

};