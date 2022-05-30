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

CommandListener::CommandListener() {
  const std::string endpoint = "tcp://*:5555";

  // generate a pull socket
  zmqpp::socket_type type = zmqpp::socket_type::reply;
  socket = new zmqpp::socket(context, type);

  // bind to the socket
  socket->bind(endpoint);
}

CommandListener::~CommandListener() {
  if(socket != nullptr) {
    delete socket;
  }
}

void CommandListener::addObserver(CommandObserver* observer ) {
  this->observer = observer;
}

void CommandListener::doAction() {
  // receive the message
  zmqpp::message message;
  // decompose the message
  socket->receive(message);	// Blocking
  std::string text;
  message >> text;

  //Do some 'work'
  PLOG_VERBOSE << "CommandListener received this message: " << text;
  socket->send(reply.c_str());
	
  if (observer != nullptr) {
    observer->newCommand(text);
  }
}

void CommandListener::setReply(const std::string& reply) {
  this->reply = reply;
}
