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

#include "signals.hpp"
#include "Touchpad.hpp"

using namespace Chaos;

Touchpad::Touchpad() {
  timer.initialize();
}

void Touchpad::firstTouch() {
  dX.priorActive = false;
  dY.priorActive = false;
}

short Touchpad::getAxisValue(ControllerSignal tp_axis, short value) {
  short axis_val;
  double scaling;
  DerivData* dd = nullptr;
  switch (tp_axis) {
  case ControllerSignal::TOUCHPAD_X:
	  dd = &dX;
    scaling = scale_x;
	  break;
  case ControllerSignal::TOUCHPAD_Y:
	  dd = &dY;
    scaling = scale_y;
	  break;
  case ControllerSignal::TOUCHPAD_X_2:
  case ControllerSignal::TOUCHPAD_Y_2:
    // We're ignoring the second finger for now
    return 0;
  default:
    throw std::runtime_error("Event passed to Touchpad::getVelocity not a TOUCHAPD axis signal");
  }  
  
  if (useVelocity()) {
    axis_val = (short) (derivative(dd, value, timer.runningTime()) * velocity_scale);
  } else {
    axis_val = (short) (distance(dd, value, timer.runningTime()) * scaling);
  }

  if (axis_val > 0) {
    axis_val += skew;
  }
  else if (axis_val < 0) {
    axis_val -= skew;
  }
  return axis_val;
}

double Touchpad::derivative(DerivData* d, short current, double timestamp) {
  double ret = 0;
  if (d->priorActive) {
    if (timestamp != d->timestampPrior[0]) {
      double denom = timestamp - d->timestampPrior[0];
      PLOG_DEBUG << "deriv denom = " << denom;
      ret = ((double)(current - d->prior[0]))/(denom);
    }	
  } else {
    d->priorActive = true;
    d->prior[1] = d->prior[2] = d->prior[3] = d->prior[4] = current;
    d->timestampPrior[1] = d->timestampPrior[2] = d->timestampPrior[3] = d->timestampPrior[4] = timestamp;
  }
  for (int i=0; i < 4; i++) {
    d->prior[i] = d->prior[i+1];
    d->timestampPrior[i] = d->timestampPrior[i+1];
  }
  d->prior[4] = current;
  d->timestampPrior[4] = timestamp;
  return ret;
}

double Touchpad::distance(DerivData* d, short current, double timestamp) {
  double ret = 0;
  // The timestamp difference won't be relevant unless we check for inactive time
  if (d->priorActive) {
    d->prior[1] = current;
    d->timestampPrior[1] = timestamp;
    ret = d->prior[1] - d->prior[0];
  } else {
    d->priorActive = true;
    d->prior[0] = current;
    d->timestampPrior[0] = timestamp;
  }
  return ret;
}