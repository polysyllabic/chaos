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
#include <vector>
#include <toml++/toml.h>

#include "Modifier.hpp"
#include "ControllerInput.hpp"
#include "ControllerInputTable.hpp"

namespace Chaos {
  class Game;
  /** 
   * \brief A modifier that remaps the game commands to different inputs from the controller.
   *
   * The job of a remapping modifier is to set up and tear down its contributions to the global
   * remaping table. The actual remapping is done before the list of modifiers is traversed. This
   * order ensures that all signals are in their ready-for-the-console state before modifiers
   * that are defined by game commands are applied.
   *
   * The following fields are used to define a remap modifier in the TOML file:
   *
   * - name (required)
   * - description (required)
   * - type = "remap"
   * - signals: An array of signals (expected to be axes) that the remap needs to process.
   * - disableSignals: If true, all the signals lised in the signals list will all receive a 0 signal
   * when the mod is initialized.
   * - remap An array of the signals that will be remapped. Each remap definition should be
   * contained in an inline table with the following possible keys:
   *   - from (required): The signal we intercept from the controller
   *   - to (required): The signal we pass to the console. For axis-to-button mapping, this is the
   * button that receives positive signals.
   *   - to_neg (optional): For axis-to-button mapping, this is the button that receives negative
   * signals.
   *   - threshold (optional): The absolute value of the signal that must be received in order to
   * trigger the remapping.
   *   - to_min (optional): When mapping buttons-to-axis, if true, the axis is set to the joystick
   * minimum. If false, it is set to the joystick maximum.
   *   - scale (optional): A divisor that scales the incomming accelerometer signal down.
   *   .
   * - random_remap: A list of controls that will be randomly remapped among one another.
   * 
   * A remap modifier may specify *either* a fixed list of remaps with the "remap" key *or* a list
   * of controls that will be randomly scrambled with the "random_remap" key, but not both.
   * 
   * When using random_remap, note that you can only scramble signals within the same basic class:
   * buttons or axes. Trying to scramble across input types will almost certainly break things,
   * since an axis must map to two buttons and vice versa.
   * 
   * Note that remapping is done by controller signal, not game command. Setting 'to' or 'to_neg' to
   * NOTHING has the effect of blocking the signal from reaching the console.
   * 
   * 
   */
  class RemapModifier : public Modifier::Registrar<RemapModifier> {

  private:
    ControllerInputTable& remap_table;

    /**
     * The list of remappings set by this mod.
     */
    std::unordered_map<std::shared_ptr<ControllerInput>, SignalRemap> remaps;

    /**
     * If true, sends events to zero out the button on initialization.
     */
    bool disable_signals;

    /**
     * \brief The list of signals affected by this mod.
     *
     * If set, these signals will be disabled when the mod starts.
     */
    std::vector<std::shared_ptr<ControllerInput>> signals;
    
    std::shared_ptr<ControllerInput> lookupInput(const toml::table& config, const std::string& key, bool required);


  public:
    static const std::string mod_type;
    
    RemapModifier(toml::table& config, Game& game);

    void begin();
    void apply();
    void finish();

  };
};

