/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
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
#include <cassert>
#include <plog/Log.h>

#include "cooldownModifier.hpp"
#include "chaosEngine.hpp"
#include "configuration.hpp"

using namespace Chaos;

const std::string CooldownModifier::mod_type = "cooldown";

CooldownModifier::CooldownModifier(toml::table& config) {
  Configuration::checkValid(config, std::vector<std::string>{"name", "description", "type", "groups",
							  "beginSequence", "finishSequence", "appliesTo", "timeOn", "timeOff", "unlisted"});
  initialize(config);
  if (commands.empty()) {
    throw std::runtime_error("No command associated with cooldown modifier.");
  }
  if (commands.size() > 1) {
    PLOG_WARNING << "More than one cooldown command assigned in appliesTo. All but the first will be ignored.";
  }
  // Save the DeviceEvent that we're operating on.
  cooldownCommand.time = 0;
  cooldownCommand.value = 0;
  cooldownCommand.type = commands[0]->getInput()->getButtonType();
  cooldownCommand.id = commands[0]->getInput()->getID(cooldownCommand.type);
  time_on = config["timeOn"].value_or(0.0);
  if (time_on == 0) {
    PLOG_WARNING << "The timeOn for this cooldown mod is 0 seconds!";
  }
  PLOG_VERBOSE << " - timeOn: " << time_on;
  
  time_off = config["timeOff"].value_or(0.0);
  if (time_off == 0) {
    PLOG_WARNING << "The timeOff for this cooldown mod is 0 seconds!";
  }
  PLOG_VERBOSE << " - timeOff: " << time_off;
}

void CooldownModifier::begin() {
  pressedState = Controller::instance().getState(cooldownCommand.id, cooldownCommand.type);
  cooldownTimer = 0.0;
  inCooldown = false;
  sendBeginSequence();
}

void CooldownModifier::update() {

  // Get the difference since the last time update.
  double deltaT = timer.dTime();

  if (inCooldown) {
    // count down until cooldown ends
    cooldownTimer -= deltaT;
    if (cooldownTimer < 0.0) {
      cooldownTimer = 0.0;
    }
  } else {
    if (Controller::instance().getState(cooldownCommand.id, cooldownCommand.type)) {
      // increment cooldown timer until time_on exceeded
      cooldownTimer += deltaT;
      if (cooldownTimer > time_on) {
	      cooldownTimer = time_off;
	      inCooldown = true;
	      // Disable event
	      // TODO: allow cooldown to do things other than block a signal
	      ChaosEngine::instance().fakePipelinedEvent(cooldownCommand, getptr());
      }
    }
  }
}

bool CooldownModifier::tweak(DeviceEvent& event) {
  if (event.id == cooldownCommand.id && event.type == cooldownCommand.type) {
    return inCooldown == false;
  }
  return true;
}

void CooldownModifier::finish() {
  sendFinishSequence();
}
