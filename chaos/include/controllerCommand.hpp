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
#include <vector>
#include <memory>
#include <unordered_map>

#include "deviceTypes.hpp"

namespace Chaos {

  /**
   * Defines the nature of a particular gamepad input. This class is used during initialization,
   * when setting up the default mappings of the commands used by the modifiers, as defined in the
   * TOML file.
   */
  class ControllerCommand {
  private:
    // the primary id (the only one for all but hybrid buttons)
    uint8_t button_id;
    uint8_t second_id;  // hybrid buttons are both a button and an axis
    GPInputType input_type;
    int index;
    int index2;
  public:
    ControllerCommand(GPInputType gptype, uint8_t first, uint8_t second);
    ControllerCommand(GPInputType gptype, uint8_t id) :
      ControllerCommand(gptype, id, 0) {}
    static void initialize();
    
    inline void setID(uint8_t new_id) {
      button_id = new_id;
    }
    inline void setID(uint8_t first, uint8_t second) {
      button_id = first;
      second_id = second;
    }
    inline uint8_t getID() {
      return button_id;
    }
    /**
     * Get the axis id of a hybrid control
     */
    inline uint8_t getSecondID() {
      return second_id;
    }
    /**
     * Get the extended input type
     */
    inline GPInputType getType() {
      return input_type;
    }
    /**
     * Get the basic button type (button vs. axis). In the case of hybrid
     * inputs, we return the button.
     */
    inline ButtonType getButtonType() {
      return (input_type == GPInputType::BUTTON || input_type == GPInputType::HYBRID) ? TYPE_BUTTON : TYPE_AXIS;
    }

    inline int getIndex() { return index; }
    inline int getIndex2() { return index2; }
    
    // map string names of buttons to their enumeration (for TOML file)
    static std::unordered_map<std::string, GPInput> buttonNames;
  };

};
