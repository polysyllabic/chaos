/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
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
#include <atomic>
#include <queue>
#include <string>
#include <thread.hpp>

#include "CommandListener.hpp"
#include "CommandSender.hpp"

namespace Chaos {

  /**
   * \brief Communication interface between the engine and the chatbot
   * 
   * The interface combines both a server (to receive messages) and a client (to send them).
   * We need both because messages can be initiated from either end.
   */
  class ChaosInterface : public Thread {
  private:
    CommandListener listener;
    CommandSender talker;

    std::queue<std::string> outgoingQueue;
    std::atomic<bool> talker_healthy{true};
    std::atomic<int> talker_health_override{-1};

    void doAction();

  public:
    /**
     * \brief Construct the communication interface.
     */
    ChaosInterface();

    /**
     * \brief Tear down sockets and worker-thread state.
     */
    ~ChaosInterface();

    /**
     * \brief Configure listener and talker endpoints.
     *
     * \param listener_endpoint Endpoint used for inbound interface commands.
     * \param talker_endpoint Endpoint used for outbound interface messages.
     */
    void setupInterface(const std::string& listener_endpoint, const std::string& talker_endpoint);

    /**
     * \brief Queue a message to send to the external interface.
     *
     * \param message Serialized message payload.
     * \return true if the message was accepted for delivery.
     */
    bool sendMessage(std::string message);

    /**
     * \brief Register the observer that receives incoming commands.
     *
     * \param observer Observer implementation to notify.
     */
    void setObserver(CommandObserver* observer);

    /**
     * \brief Report whether the talker socket is currently healthy.
     */
    bool isTalkerHealthy() const;

    /**
     * \brief Test helper to force a specific talker health state.
     *
     * \param healthy Forced health value.
     */
    void setTalkerHealthyForTest(bool healthy);
  };

};
