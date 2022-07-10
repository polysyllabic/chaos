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
#include <unordered_map>
#include <memory>
#include <string>

#include <signals.hpp>
#include <DeviceEvent.hpp>

namespace Chaos {
  class Controller;
  class ControllerInput;

  using RemapTable = std::unordered_map<std::shared_ptr<ControllerInput>, SignalRemap>;

  /**
   * \brief Class holding the table of all controller signal information, including remaps
   * 
   */
  class ControllerInputTable {

  public:
    ControllerInputTable(Controller& c);

    /**
     * \brief Accessor for the class object identified by name.
     * \param name Name of the input
     * \return The ControllerInput object identified by the given string
     *
     * Used for processing the TOML file.
     */
    std::shared_ptr<ControllerInput> getInput(const std::string& name);
    
    /**
     * \brief Get the ControllerInput object
     * \param signal The enumeration of the controller signal
     */
    std::shared_ptr<ControllerInput> getInput(ControllerSignal signal);
    
    /**
     * \brief Get the ControllerInput object for the incoming signal from the controller.
     * \param event The event coming from the controller.
     */
    std::shared_ptr<ControllerInput> getInput(const DeviceEvent& event);

    /**
     * \brief Get the ControllerInput object
     * \param signal The enumeration of the controller signal
     */
    std::shared_ptr<ControllerInput> getInput(const toml::table& config, const std::string& key);

    std::unordered_map<ControllerSignal, std::shared_ptr<ControllerInput>>& getInputMap() { return inputs; }

    /**
     * \brief Tests if an event matches this signal
     * \param event The incomming event from the controller
     * \param to The input signal we're testing for
     * \return Whether event id and type match the definition for this controller input.
     *
     * Remapping should be complete before the ordinary (non-remapping) mods see the event, so
     * it is safe to test for the actual events that the console expects.
     */
    bool matchesID(const DeviceEvent& event, ControllerSignal to);

    int initializeInputs(const toml::table& config);

    void addToVector(const toml::table& config, const std::string& key,
                     std::vector<std::shared_ptr<ControllerInput>>& vec);

  private:
    /**
     * Look up signal by enumeration
     */
    std::unordered_map<ControllerSignal, std::shared_ptr<ControllerInput>> inputs;

    /**
     * Lookup signal by TOML file name.
     */
    std::unordered_map<std::string, std::shared_ptr<ControllerInput>> by_name;

    /**
     * Lookup signal by DeviceEvent index.
     */
    std::unordered_map<int, std::shared_ptr<ControllerInput>> by_index;


  };

};