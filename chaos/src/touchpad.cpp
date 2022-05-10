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
#include "configuration.hpp"

using namespace Chaos;

void Touchpad::initialize(const toml::table& config) {
  scale = config["remapping"]["touchpad_scale"].value_or(1.0);
  if (scale == 0) {
    PLOG_ERROR << "Touchpad scale cannot be 0. Setting to 1";
    scale = 1;
  }
  // Condition is optional; Warn if bad condition name but not if missing entirely
  std::optional<std::string> c = config["remapping"]["touchpad_condition"].value<std::string>();
  if (c) {
    condition = GameCondition::get(*c);
    if (! condition) {
      PLOG_WARNING << "The condition " << *c << " is not defined";
    }
  } 
  scale_if = config["remapping"]["touchpad_scale_if"].value_or(1.0);
  if (scale_if == 0) {
    PLOG_ERROR << "Touchpad scale_if cannot be 0. Setting to 1";
    scale_if = 1;
  }
  skew = config["remapping"]["touchpad_skew"].value_or(0);
  timer.initialize();
  PLOG_DEBUG << "Touchpad scale = " << scale << "; condition = " << condition->getName() <<
    "; scale_if = " << scale_if << "; skew = " << skew;
}

void Touchpad::clearActive() {
  dX.priorActive = false;
  dY.priorActive = false;
  dX_2.priorActive = false;
  dY_2.priorActive = false;
}

// Given a touchpad event, returns an equivalent axis signal
// Note: This is NOT clipped to joystick min/max. That must be done by the calling function.
// This represents a first stab at generalization beyond TLOU2. It will almost certainly need
// further refinement to handle the requirements of other games.
short Touchpad::toAxis(ControllerSignal signal, short value) {
  int ret = 0;
  double derivativeValue;
  DerivData* dd = nullptr;

  switch (signal) {
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
    throw std::runtime_error("Touchpad::toAxis: Original event not a TOUCHAPD axis signal");
  }
  
  // Use the touchpad value to update the running derivative count
  derivativeValue = derivative(dd, value, timer.runningTime()) *
      (condition->inCondition()) ? scale_if : scale;

  if (derivativeValue > 0) {
    ret = (int) derivativeValue + skew;
  }
  else if (derivativeValue < 0) {
    ret = (int) derivativeValue - skew;
  }
    
  PLOG_DEBUG << "Derivative: " << derivativeValue << ", skew = " << skew << ", returning " << ret;
  return ret;
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

bool Touchpad::inCondition() {
  return condition->inCondition();  
}