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
#include <optional>
#include <string>
#include <tuple>

namespace Chaos {

  enum class CommandType { BUTTON, AXIS, HYBRID };

  class DeviceCommand {
  private:
    DeviceCommand();
    /**
     * The numerical ID that the controller expects to be associated with
     * this command.
     */
    uint8_t id;
  public:

    static std::map<std::string, std::function<DeviceCommand*()>> button_map;
    static DeviceCommand* get();
    
    virtual std::tuple<uint8_t, uint8_t> getID() = 0;
    virtual CommandType getType() = 0;

  };

  class Button : public DeviceCommand {
    
  public:
    static void build(std::string& name, uint8_t id);

    inline CommandType getType() { return CommandType::Button; }
  };
  
  class Axis : public DeviceCommand {
    
  public:
    static void build(std::string& name, uint8_t id);

    inline CommandType getType() { return CommandType::Axis; }
  };

  /**
   * A control that has both a button and an axis event (R2, L2)
   */
  class Hybrid : public DeviceCommand {
  private:
    uint8_t axis_id;
  public:
    static void build(std::string& name, uint8_t id);

    inline uint8_t getAxis() { return axis_id; }
    
    inline CommandType getType() { return CommandType::Hybrid; }
  };
  

};
