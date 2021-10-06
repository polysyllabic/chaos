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
#ifndef CHAOS_INTERFACE_HPP
#define CHAOS_INTERFACE_HPP
#include <queue>
#include <string>
#include <mogi/thread.h>
#include <zmqpp/zmqpp.hpp>

namespace Chaos {

  class CommandListenerObserver {
  public:
    virtual void newCommand( const std::string& command ) = 0;
  };

  class CommandListener : public Mogi::Thread {
  private:
    zmqpp::socket *socket;
    zmqpp::context context;
	
    CommandListenerObserver* observer = NULL;
	
    void doAction();
	
  public:
    CommandListener();
    ~CommandListener();
	
    std::string reply;
	
    void addObserver( CommandListenerObserver* observer );
    void setReply(const std::string& reply);
  };

  class CommandSender { //}: public Mogi::Thread {
  private:
    zmqpp::socket *socket;
    zmqpp::context context;
	
    std::string reply;
	
    //void doAction();
	
  public:
    CommandSender();
    ~CommandSender();
	
    bool sendMessage(std::string message);
  };

  class ChaosInterface : public Mogi::Thread {
  private:
    CommandListener listener;
    CommandSender sender;
	
    std::queue<std::string> outgoingQueue;
	
    void doAction();
	
  public:
    ChaosInterface();
	
    bool sendMessage(std::string message);
    void addObserver( CommandListenerObserver* observer );
  };

};

#endif
