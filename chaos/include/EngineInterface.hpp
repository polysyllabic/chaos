/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
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
#include "DeviceEvent.hpp"

// This gathers all the various data into a single facade run through the engine
// Should we split this into multiple interfaces? Unless we need to speed things up, it's
// probably better only to keep a pointer to a single facade (the engine)
namespace Chaos {
  class Modifier;
  class GameCommand;
  class MenuItem;
  class ControllerInput;
  class SignalRemap;
  class GameCondition;
  class Sequence;
  
  class EngineInterface {
  public:
    virtual void fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod) = 0;
    // These functions access the controller
    virtual short getState(uint8_t id, uint8_t type) = 0;
    virtual bool eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) = 0;
    virtual void setOff(std::shared_ptr<GameCommand> command) = 0;
    virtual void setOn(std::shared_ptr<GameCommand> command) = 0 ;
    // These functions access the menu system
    virtual std::shared_ptr<MenuItem> getMenuItem(const std::string& name) = 0;
    virtual void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) = 0;
    virtual void restoreMenuState(std::shared_ptr<MenuItem> item) = 0;
    // These functions access the signal (remap) table
    virtual std::shared_ptr<ControllerInput> getInput(const std::string& name) = 0;
    virtual void setCascadingRemap(std::unordered_map<std::shared_ptr<ControllerInput>, SignalRemap>& remaps) = 0;
    virtual void clearRemaps() = 0;
    // Game Controller Inputs
    virtual void addControllerInputs(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<ControllerInput>>& vec) = 0;
    // Game Commands
    virtual void addGameCommands(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<GameCommand>>& vec) = 0;
    // Game Conditions
    virtual void addGameConditions(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<GameCondition>>& vec) = 0;
    // Sequences
    virtual std::shared_ptr<Sequence> createSequence(toml::table& config,
                                                     const std::string& key, bool required) = 0;
  };
};