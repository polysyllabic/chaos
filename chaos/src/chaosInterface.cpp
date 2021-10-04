/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#include "chaosInterface.hpp"

#include <iostream>
#include <unistd.h>
#include <string>
#include <json/json.h>

using namespace Chaos;

ChaosInterface::ChaosInterface() {
  listener.start();
  start();
}

void ChaosInterface::doAction() {
  while (!outgoingQueue.empty()) {
    lock();
    std::string message = outgoingQueue.front();
    outgoingQueue.pop();
    unlock();
		
    sender.sendMessage(message);
  }
	
  pause();
}
	
bool ChaosInterface::sendMessage(std::string message) {
  lock();
  outgoingQueue.push(message);
  unlock();
	
  resume();
  return true;
}

void ChaosInterface::addObserver( CommandListenerObserver* observer ) {
  listener.addObserver(observer);
}
