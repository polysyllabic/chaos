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
//#include <sstream>
#include <cstdlib>
#include <json/json.h>

#include "modifier.hpp"

using namespace Chaos;

Modifier::Modifier(Passkey, Controller* controller, ChaosEngine* engine, toml::table& config) :
  controller{controller}, engine{engine}
{
  timer.initialize();
  me = this;
  pauseTimeAccumulator = 0;
  // Initialize those elements from the TOML table that are common to all mod types
  
}

void Modifier::_update(bool isPaused) {
  timer.update();
  if (isPaused) {
    pauseTimeAccumulator += timer.dTime();
  }
  this->update();
}

// Default implementations of virtual functions do nothing
void Modifier::begin() {}

void Modifier::update() {}

void Modifier::finish() {}

bool Modifier::tweak( DeviceEvent* event ) {
  return true;
}

Json::Value Modifier::toJsonObject(std::string& name) {
  Json::Value result;
  result["name"] = name;
  result["desc"] = getDescription();
  return result;
}

std::string Modifier::getModList() {
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



