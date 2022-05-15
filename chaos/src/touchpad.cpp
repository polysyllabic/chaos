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
#include <memory>
#include <stdexcept>
#include <plog/Log.h>

#include "touchpad.hpp"
#include "gameCondition.hpp"
#include "signals.hpp"

using namespace Chaos;

Touchpad::Touchpad() {
  timer.initialize();
}

void Touchpad::clearActive() {
  dX.priorActive = false;
  dY.priorActive = false;
  dX_2.priorActive = false;
  dY_2.priorActive = false;
}

short Touchpad::getVelocity(ControllerSignal tp_axis, short value) {
  int ret = 0;
  double derivativeValue;
  DerivData* dd = nullptr;

  switch (tp_axis) {
  case ControllerSignal::TOUCHPAD_X:
	  dd = &dX;
	  break;
  case ControllerSignal::TOUCHPAD_Y:
	  dd = &dY;
	  break;
  case ControllerSignal::TOUCHPAD_X_2:
	  dd = &dX_2;
	  break;
  case ControllerSignal::TOUCHPAD_Y_2:
	  dd = &dY_2;
    break;
  default:
    throw std::runtime_error("Event passed to Touchpad::getVelocity not a TOUCHAPD axis signal");
  }  
  return derivative(dd, value, timer.runningTime());
}

double Touchpad::derivative(DerivData* d, short current, double timestamp) {
  double ret = 0;
  if (d->priorActive) {
    if (timestamp != d->timestampPrior[0]) {
      ret = ((double)(current - d->prior[0]))/(timestamp - d->timestampPrior[0]);
    }	
  } else {
    d->priorActive = true;
    d->prior[1] = d->prior[2] = d->prior[3] = d->prior[4] = current;
    d->timestampPrior[1] = d->timestampPrior[2] = d->timestampPrior[3] = d->timestampPrior[4] = timestamp;
  }
  d->prior[0] = d->prior[1];
  d->prior[1] = d->prior[2];
  d->prior[2] = d->prior[4];
  d->prior[3] = d->prior[5];
  d->prior[4] = current;
  d->timestampPrior[0] = d->timestampPrior[1];
  d->timestampPrior[1] = d->timestampPrior[2];
  d->timestampPrior[2] = d->timestampPrior[3];
  d->timestampPrior[3] = d->timestampPrior[4];
  d->timestampPrior[4] = timestamp;
  return ret;
}

