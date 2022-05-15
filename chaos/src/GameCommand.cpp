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
#include <string>
#include <toml++/toml.h>
#include <plog/Log.h>

#include "GameCommand.hpp"
#include "GameCondition.hpp"
#include "ControllerInput.hpp"

using namespace Chaos;

GameCommand::GameCommand(const std::string& cmd, std::shared_ptr<ControllerInput> bind) :
                         name{cmd}, binding{bind} {
}

std::shared_ptr<ControllerInput> GameCommand::getRemappedSignal() {
  // If not remapping, return the actual signal
  if (binding->getRemap() == nullptr) {
    return binding;
  }
  // If remapping to dev/null, return null
  if (binding->getRemap()->getSignal() == ControllerSignal::NOTHING) {
    return nullptr;
  }
  return binding->getRemap();
}

short GameCommand::getState() {
  return binding->getRemappedState();
}
