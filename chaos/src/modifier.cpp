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
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <cstdlib>
#include <json/json.h>

#include "modifier.hpp"

using namespace Chaos;

Modifier::Modifier() {
  this->timer.initialize();
  me = this;
  pauseTimeAccumulator = 0;
}

void Modifier::setController(Controller* controller) {
  this->controller = controller;
}

void Modifier::setChaosEngine(ChaosEngine* chaosEngine) {
  this->chaosEngine = chaosEngine;
}

void Modifier::_update(bool isPaused) {
  timer.update();
  if (isPaused) {
    pauseTimeAccumulator += timer.dTime();
  }
  this->update();
}
 
void Modifier::begin() {	// virtual override
}

void Modifier::update() {	// virtual override
}

void Modifier::finish() {	// virtual override
}

const char* Modifier::description() {	// virtual override
  return "";
}

std::map<std::string, std::function<Modifier*()>> Modifier::factory;

Modifier* Modifier::build( const std::string& name ) {
  Modifier* mod = NULL;
	
  if (factory.count(name) > 0) {
    mod = factory[name]();
  }
	
  return mod;
}

Json::Value modifierToJsonObject( std::string name, Modifier* mod ) {
  Json::Value result;
  result["name"] = name;
  result["desc"] = mod->description();
  return result;
}

std::string Modifier::getModList(double timePerModifier) {
  Json::Value root;
  Json::Value & data = root["mods"];
  int i = 0;
  for (std::map<std::string,std::function<Modifier*()>>::iterator it = factory.begin();
       it != factory.end();
       it++) {
    Modifier* tempMod = it->second();
    data[i++] = modifierToJsonObject( it->first, tempMod);
    delete tempMod;
  }
	
  Json::StreamWriterBuilder builder;
	
  return Json::writeString(builder, root);
}

double Modifier::lifetime() {
  return timer.runningTime() - pauseTimeAccumulator;
}

// Alter the signal as necessary.
// By default, it is valid and we do nothing
bool Modifier::tweak( DeviceEvent* event ) {
  return true;
}

void Modifier::setParentModifier(Modifier* parent) {
  this->me = parent;
}

/**
 * Test current state of a button and return its pressed state as boolean.
 */
bool Modifier::buttonPressed(Button button) {
  return controller->getState(controller->getButton(button), TYPE_BUTTON);
}

/**
 * Force the button off. This disables the button press unconditionally.
 */
void Modifier::buttonOff(DeviceEvent* event, Button button) {
  if (event->id == controller->getButton(button) && event->type == TYPE_BUTTON) {
    event->value = 0;
  }
}

/**
 * Force the button off if another button is also pressed at the same time.
 */
void Modifier::buttonOff(DeviceEvent* event, Button button, Button alsoPressed) {
  if (event->id == controller->getButton(button) && event->type == TYPE_BUTTON && buttonPressed(alsoPressed)) {
    event->value = 0;
  }
}

/**
 * Force the button on. This disables the button press unconditionally.
 */
void Modifier::buttonOn(DeviceEvent* event, Button button) {
  if (event->id == controller->getButton(button) && event->type == TYPE_BUTTON) {
    event->value = 1;
  }
}

/**
 * Force an axis to zero.
 */
void Modifier::axisOff(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = 0;
  }
}

/**
 * Force an axis to the minimum value.
 */
void Modifier::axisMin(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = controller->getJoystickMin();
  }
}

/**
 * Force an axis to the maximum value.
 */
void Modifier::axisMax(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = controller->getJoystickMax();
  }
}

/**
 * Invert the axis value (swap positive and negative values).
 */
void Modifier::axisInvert(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = -((int) event->value + 1);
  }
}

/**
 * Only allow positive axis values.
 */
void Modifier::axisPositive(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = event->value >= 0 ? event->value  : 0;
  }
}

/**
 * Only allow negative axis values.
 */
void Modifier::axisNegative(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = event->value <= 0 ? event->value  : 0;
  }
}

/**
 * Make all negative values positive.
 */
void Modifier::axisAbsolute(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = abs(event->value);
  }
}

/**
 * Make all positive values negative.
 */
void Modifier::axisNegAbsolute(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS) {
    event->value = -abs(event->value);
  }
}

/**
 * Returns false if the axis criterion matches. When false is returned
 * from a tweak routine, it has the effect of preventing other mods in
 * the list from further modifying this event. If true is returned, further
 * processing can occur.
 */
bool Modifier::axisDropEvent(DeviceEvent* event, Axis axis) {
  if (event->id == controller->getAxis(axis) && event->type == TYPE_AXIS)
    return false;
  return true;
}
