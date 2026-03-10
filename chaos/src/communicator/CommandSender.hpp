/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in top-level directory of this distribution for a list of the
 * contributors.
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
#pragma once
#include <queue>
#include <string>
#include <chrono>
#include <zmq.hpp>

namespace Chaos {

  class CommandSender {
  private:
    zmq::context_t context;	
    zmq::socket_t *socket = nullptr;
    std::string endpoint;

    std::chrono::milliseconds request_timeout{10000};
    int request_retries{3};

    zmq::socket_t* createSocket();

  public:
    /**
     * \brief Construct a sender with default retry settings.
     */
    CommandSender();

    /**
     * \brief Release sender socket resources.
     */
    ~CommandSender();
	
    /**
     * \brief Set the endpoint used for outbound requests.
     *
     * \param ep ZMQ endpoint string.
     */
    void setEndpoint(const std::string& ep);

    /**
     * \brief Send a message and wait for acknowledgement.
     *
     * \param message Serialized message payload.
     * \return true when acknowledgement is received.
     */
    bool sendMessage(const std::string& message);
  };

};
