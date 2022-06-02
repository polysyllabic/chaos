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
#include <string>
#include <plog/Log.h>

#include "CommandListener.hpp"

using namespace Chaos;

CommandListener::CommandListener() {}

CommandListener::~CommandListener() {
  if(socket != nullptr) {
    delete socket;
  }
}

void CommandListener::setEndpoint(const std::string& endpoint) {

  // create a reply socket
  zmqpp::socket_type type = zmqpp::socket_type::reply;
  socket = new zmqpp::socket(context, type);
  // bind to the socket
  PLOG_DEBUG << "Binding reply socket to endpoint " << endpoint;
  socket->bind(endpoint);
}

void CommandListener::setObserver(CommandObserver* observer ) {
  this->observer = observer;
}

void CommandListener::doAction() {
  zmqpp::message message;
  // Wait for a message to arive. This call is blocking
  socket->receive(message);	
  std::string text;
  message >> text;
  PLOG_VERBOSE << "CommandListener received this message: " << text;

  // Tell observer what we received
  if (observer != nullptr) {
    observer->newCommand(text);
  }

  // Send an acknowledgment. We do this after notifying the observers to give them
  // a chance to set the reply if necessary
  socket->send(reply.c_str());
}

void CommandListener::setReply(const std::string& reply) {
  this->reply = reply;
}
