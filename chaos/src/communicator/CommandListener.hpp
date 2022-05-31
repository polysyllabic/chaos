/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
#include <string>
#include <mogi/thread.h>
#include <zmqpp/zmqpp.hpp>

#include "CommandObserver.hpp"

namespace Chaos {

  /**
   * \brief A ZMQ server to receive messages from the chaosface client
   * 
   */
  class CommandListener : public Mogi::Thread {
  private:
    zmqpp::socket *socket;
    zmqpp::context context;
    std::string reply;
    CommandObserver* observer = nullptr;
	
    void doAction();
	
  public:
    CommandListener();
    ~CommandListener();
	
    void setEndpoint(const std::string& endpoint);
	
    /**
     * \brief Set an observer for incomming messages
     * 
     * \param observer Pointer to the observer
     * 
     * We only allow one observer at the moment.3
     */
    void setObserver(CommandObserver* observer);
    void setReply(const std::string& reply);
  };

};
