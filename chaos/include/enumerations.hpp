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

// This file contains the enumerated classes we use
namespace Chaos {
  /**
   * \brief Defines the types of threshold events that will trigger the condition.
   *
   * The specific test for the condition is determined by this type, which must be one of the 
   * following values:
   *  - ABOVE: A signal whose magnitude is greater than the threshold (DEFAULT)
   *  - BELOW: A signal whose magnitude is less than the threshold
   *  - GREATER: A signal whose signed value is greater than or equal to the threshold
   *  = LESS: A signal whose signed value is less than the threshold
   *  - DISTANCE: Calculates the Pythagorean distance of the first two signals in the condition
   * list (this assumes that they are axes). The condition is true if this distance is greater than
   * or equal to the threshold.
   *  - DISTANCE_BELOW: Like DISTANCE but true if the distance is less than the threshold
   */
  enum class ThresholdType { ABOVE, BELOW, GREATER, LESS, DISTANCE, DISTANCE_BELOW };

  /**
   * \brief The type of comparison to perform with the vector of game conditions.
   *
   * Conditions are stored in vectors to permit testing of more than one input signal
   * simultaneously. A call to inCondition runts the test specified by ConditionCheck. The
   * following tests are supported:
   * - ALL: Every condition in a vector of game conditions must be true for the condition test to
   * return true. (_Default_)
   * - ANY: The condition test returns true if any gamestate in the vector is true.
   * - NONE: The check returns true if _no_ condition in the vector is true.
   */
  enum class ConditionCheck { ALL, ANY, NONE };

};
