/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78. Additional code by polysyl.
*
* TCC is free software: you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* TCC is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along
* with TCC.  If not, see <https://www.gnu.org/licenses/>.
*---------------------------------------------------------------------------*/
#include "modifier.h"

#include <iostream>
#include <unistd.h>
#include <sstream>
#include <json/json.h>

using namespace Chaos;

Modifier::Modifier() {
  this->timer.initialize();
  me = this;
  pauseTimeAccumulator = 0;
}

void Modifier::setDualshock(Controller* dualshock) {
  this->dualshock = dualshock;
}

void Modifier::setChaosEngine(Engine* chaosEngine) {
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

void Modifier::invert(DeviceEvent* event, AxisID axis) {
  if ((event->id == axis) && event->type == TYPE_AXIS) {
    event->value = -((int) event->value + 1);
  }
}

// Disable a button unconditionally
void Modifier::disable_button(DeviceEvent* event, ButtonID button) {
  if (event->id == button && event->type == TYPE_BUTTON) {
    event->value = 0;
  }
}

// Disable button if another button is also currently presssed (the condition)
void Modifier::disable_button(DeviceEvent* event, ButtonID button, ButtonID alsoPressed) {
  if (event->id == button && event->type == TYPE_BUTTON && condition(alsoPressed, TYPE_BUTTON))
    event->value = 0;
  }
}

// Disable an axis unconditionally
void Modifier::disable_axis(DeviceEvent* event, AxisID axis) {
  if (event->id == axis && event->type == TYPE_AXIS) {
    event->value = JOYSTICK_MIN;
  }
}

// Test current state of a button/axis and return as boolean
bool Modifier::condition(unsigned char button, ButtonType bType) {
  return dualshock->getState(button, bType);
}
