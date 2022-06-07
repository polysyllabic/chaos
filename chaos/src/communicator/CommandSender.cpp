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

void CommandSender::setEndpoint(const std::string& endpoint) {
  zmqpp::socket_type type = zmqpp::socket_type::request;
  socket = new zmqpp::socket(context, type);
  PLOG_DEBUG << "Connecting request socket to endpoint " << endpoint;
  socket->connect(endpoint);
}

bool CommandSender::sendMessage(std::string message) {
  PLOG_DEBUG << "Sending message: " << message;
  socket->send(message.c_str());
  zmqpp::message msg;
  // Wait for reply. This call is blocking
  socket->receive(msg);
  msg >> reply;
  return true;
}
