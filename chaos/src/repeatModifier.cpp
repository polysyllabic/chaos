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
#include "repeatModifier.hpp"

using namespace Chaos;

const std::string RepeatModifier::name = "repeat";

RepeatModifier::RepeatModifier(Controller* controller, ChaosEngine* engine, const toml::table& config) {
  initialize(controller, engine, config);
  
}

void RepeatModifier::begin() {
  /*
  DeviceEvent event = {0, 1, TYPE_BUTTON, button};
  controller->applyEvent(&event);
  if (axis != Axis::NONE) {
    event = {0, axisVal, TYPE_AXIS, axis};
    controller->applyEvent(&event);
  }
  pressTime = 0;
  toggleCount = 0; */
}

void RepeatModifier::update() {
  /*
  pressTime += timer.dTime();
  // Is this axis command actually necessary? It should be set in begin and blocked from
  // further changes.
  if (axis != Axis::NONE) {
    event = {0, axisVal, TYPE_AXIS, axis};
    controller->applyEvent(&event);
  }
  if (repeats == 0 || toggleCount < repeats) {
    if (pressTime > onTime && controller->getState(button, TYPE_BUTTON)) {
      DeviceEvent event = {0, 0, TYPE_BUTTON, button};
      controller->applyEvent(&event);
      pressTime = 0;
      if (repeats > 0) {
	toggleCount++;
      }
    } else if (pressTime > offTime && controller->getState(button, TYPE_BUTTON)) {
      DeviceEvent event = {0, 1, TYPE_BUTTON, button};
      controller->applyEvent(&event);
      pressTime = 0;
    }
  } else if (pressTime > cycleDelay) {
    toggleCount = 0;
    pressTime = 0;
    } */
}

bool RepeatModifier::tweak(DeviceEvent* event) {
  /*
  if (forceOn) {
    
  } */
  return true;
}

void RepeatModifier::finish() {
  /*
  DeviceEvent event = {0, 0, TYPE_BUTTON, button};
  controller->applyEvent(&event);
  if (axis != Axis::NONE) {
    event = {0, 0, TYPE_AXIS, axis};
    controller->applyEvent(&event);
    } */
}
