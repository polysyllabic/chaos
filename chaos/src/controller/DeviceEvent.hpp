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
#include <cstdint>

namespace Chaos {

  typedef struct DeviceEvent {
    unsigned int time;
    short int value;
    uint8_t type;
    uint8_t id;

    /**
     * \brief Compute the packed lookup index for this event's type/id pair.
     */
    int index() const {
      return ((int) type << 8) + (int) id;
    }
    
    /**
     * \brief Check whether this event represents a synthetic delay marker.
     */
    bool isDelay() const {
      return (value == 0 && id == 255 && type == 255);
    }

    /**
     * \brief Compare two events by type/id identity.
     */
    bool operator==(const DeviceEvent &other) const {
      return type == other.type && id == other.id;
    }

    /**
     * \brief Provide strict weak ordering by type, then id.
     */
    bool operator<(const DeviceEvent &other) const {
      return type < other.type || (type == other.type && id < other.id);
    }
  } DeviceEvent;

};
