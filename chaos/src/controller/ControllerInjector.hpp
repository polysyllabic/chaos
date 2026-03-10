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
#include "DeviceEvent.hpp"

namespace Chaos {

  /**
   * \brief Interface for classes that need to intercept signals coming from the controller, modify
   * them, and inject new signals.
   * 
   */
  class ControllerInjector  {
  public:
    /**
     * \brief Sniff an input event and optionally modify the forwarded output event.
     *
     * \param input Incoming event from controller processing.
     * \param output Event to forward toward the console pipeline.
     * \return true if output should be forwarded, false to drop it.
     */
    virtual bool sniffify(const DeviceEvent& input, DeviceEvent& output) = 0;

    /**
     * Whether raw USB reports should bypass state reconstruction in ControllerRaw.
     *
     * Returning true means the incoming report should be forwarded unchanged.
     */
    virtual bool prefersRawPassthrough() const { return false; }
  };

};
