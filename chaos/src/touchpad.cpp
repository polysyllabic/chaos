/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
#include <plog/Log.h>

#include "touchpad.hpp"
#include "gameCommand.hpp"

using namespace Chaos;

void Touchpad::initialize(const toml::table& config) {

  timer.initialize();
  
  //  condition = GPInput::NONE;
  std::optional<std::string> tp_condition = config["touchpad"]["scale_on"].value<std::string>();
  if (tp_condition) {
    std::shared_ptr<GameCommand> condition = GameCommand::get(*tp_condition);
    // condition must be an existing command
    if (condition) {
      scale_if = config["touchpad"]["scale_if"].value_or(1.0);      
    } else {
      throw std::runtime_error("Touchpad condition '" + *tp_condition + "' does not exist");
    }
  }
  scale = config["touchpad"]["scale"].value_or(1.0);
  skew = config["touchpad"]["skew"].value_or(0);

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
int Touchpad::toAxis(const DeviceEvent& event, bool inCondition) {
  int ret = 0;
  double derivativeValue;
  DerivData* dd = NULL;

  if (event.type == TYPE_AXIS) {
      switch (event.id) {
      case AXIS_TOUCHPAD_X:
	dd = &dX;
	break;
      case AXIS_TOUCHPAD_Y:
	dd = &dY;
	break;
      case AXIS_TOUCHPAD_X_2:
	dd = &dX_2;
	break;
      case AXIS_TOUCHPAD_Y_2:
	dd = &dY_2;
      }
    }
    if (!dd) {
      throw std::runtime_error("Touchpad::toAxis: Original event not a TOUCHAPD axis signal");
    }
    
    derivativeValue = derivative(dd, event.value, timer.runningTime()) *
      (condition->getBinding() != GPInput::NONE && inCondition) ? scale_if : scale;

    if (derivativeValue > 0) {
      ret = (int) derivativeValue + skew;
    }
    else if (derivativeValue < 0) {
      ret = (int) derivativeValue - skew;
    }
    
    PLOG_VERBOSE << "Touchpad-to-axis conversion returning  " << ret << "\n";
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
