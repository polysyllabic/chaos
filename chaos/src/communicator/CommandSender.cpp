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
#include "CommandSender.hpp"
#include <plog/Log.h>
using namespace Chaos;

CommandSender::CommandSender() {
}

CommandSender::~CommandSender() {
  if(socket != nullptr) {
    delete socket;
  }
}

void CommandSender::setEndpoint(const std::string& ep) {
  endpoint = ep;
  socket = createSocket();
}

zmq::socket_t* CommandSender::createSocket() {
  PLOG_VERBOSE << "Creating request socket and connecting to " << endpoint;
  zmq::socket_t* s = new zmq::socket_t(context, zmq::socket_type::req);
  s->connect(endpoint);
  // No waiting at close time
  s->set(zmq::sockopt::linger, 0);
  return s;
}

bool CommandSender::sendMessage(const std::string& message) {
  PLOG_VERBOSE << "Sending message: " << message;
  zmq::message_t msg(message);
  zmq::message_t reply;

  int retries = request_retries;
  // Wait for reply. We use the lazy pirate pattern to try to recover from a failure of the
  // interface. See https://zguide.zeromq.org/docs/chapter4/
  while (retries > 0) {
    socket->send(msg, zmq::send_flags::none);
    // Poll socket for a reply, with timeout
    zmq::pollitem_t items[] = { {*socket, 0, ZMQ_POLLIN, 0} };
    zmq::poll (&items[0], 1, request_timeout);
    // If we got a reply, process it
    if (items[0].revents & ZMQ_POLLIN) {
      auto res = socket->recv(reply, zmq::recv_flags::none);
      PLOG_DEBUG << "received ack";
      return true;
    }
    else {
      --retries;
      PLOG_DEBUG << "Timeout waiting for reply. Dropping old socket";
      delete socket;
      socket = createSocket();
    }
  }
  PLOG_DEBUG << "Abandoning message";
  return false;
}
