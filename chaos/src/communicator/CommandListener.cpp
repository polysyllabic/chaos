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

void CommandListener::setEndpoint(const std::string& ep) {
  endpoint = ep;
  socket = createSocket();
}

zmq::socket_t* CommandListener::createSocket() {
  PLOG_DEBUG << "Creating reply socket and binding to " << endpoint;
  zmq::socket_t* s = new zmq::socket_t(context, zmq::socket_type::rep);
  s->set(zmq::sockopt::rcvtimeo, 250);
  s->bind(endpoint);
  // No waiting at close time
  //int linger = 0;
  //s->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
  return s;
}

void CommandListener::setObserver(CommandObserver* observer ) {
  this->observer = observer;
}

void CommandListener::doAction() {
  zmq::message_t message;
  zmq::message_t r(reply);
  // Wait for a message to arive. This call is blocking
  auto res = socket->recv(message, zmq::recv_flags::none);	
  if (!res) {
    return;
  }
  std::string text = message.to_string();
  PLOG_VERBOSE << "CommandListener received this message: " << text;
  // Send ACK response
  auto ack = socket->send(r, zmq::send_flags::none);

  // Tell observer what we received
  if (observer != nullptr) {
    observer->newCommand(text);
  }

}
