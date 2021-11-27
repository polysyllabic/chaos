/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
#include <toml++/toml.h>
#include "gamepadInput.hpp"

namespace Chaos {

  /**
   * \brief Keeps track of a particular game state.
   *
   * A game state is defined in a TOML file and represents a persistant condition. It represents
   * a particular boolean game state caused by a button press even after that button is relased.
   * This state continues until some other event occurs to turn it off. To refresh the game state,
   * updateState() should be invoked from a modifier's tweak() routine. The current state of the
   * condition can be checked with isState().
   *
   * TO DO: Extend to non-boolean states, and those invoked by multiple conditions.
   */
  class GameState {
  private:
    GPInput signal_on;
    GPInput signal_off;

    bool current_state;

  /**
   * Global map for defined game states
   */
  static std::unordered_map<std::string, std::shared_ptr<GameState>> states;

  public:
    GameState(const toml::table& config);

    /**
     * Accessor for the class object of the game state associated with this name.
     */
    static std::shared_ptr<GameState> get(const std::string& name);
    
    /**
     * Initialize the global map to hold the game states. This is called once during the
     * start-up phase.
     */
    static void initialize(toml::table& config);

    
    /**
     * Turns the state on or off if the event matches
     */
    GPInput getOn() { return signal_on; }
    GPInput getOff() { return signal_off; }
    void setState(bool state) { current_state = state; }
    /**
     * Indicates whether the game is in the given state.
     */
    inline bool getState() { return current_state; }
  };

};
