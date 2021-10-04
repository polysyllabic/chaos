/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#include "controllerState.hpp"

#include <cstddef>

using namespace Chaos;

ControllerState::~ControllerState() {
}

void* ControllerState::getHackedState() {
  return hackedState;
}

ControllerState* ControllerState::factory(int vendor, int product) {
  if (vendor == 0x054c && product == 0x09cc) {
    return new ControllerStateDualshock;
  } else if (vendor == 0x054c && product == 0x0ce6) {
    return new ControllerStateDualsense;
  } else if (vendor==0x2f24  && product==0x00f8) {	// Magic-S Pro
    return new ControllerStateDualshock;
  }	
  return NULL;
}
