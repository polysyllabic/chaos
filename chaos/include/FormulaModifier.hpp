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
#include <unordered_map>
#include <toml++/toml.h>

#include "Modifier.hpp"
#include "Game.hpp"

namespace Chaos {

  enum class FormulaTypes {
    CIRCLE, EIGHT_CURVE, JANKY
  };

  /**
   * \brief Modifier that alters the signal through a formula.
   *
   * Formula modifiers apply an offset to incoming signals according to a specified, time-dependent
   * formula. Currently, we don't have the ability to parse a general formula, so you must select
   * from a pre-defined formula types.
   * 
   * The following keys are defined for this class of modifier:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "formula" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - appliesTo: A commands affected by the mod. (_Required_)
   * - amplitude: Proportion of the maximum  signal by (_Optional. Default = 1_)
   * - period_length: Time in seconds before the formula completes its cycle (_Optional. Default = 1_)
   * - beginSequence: A sequence of button presses to apply during the begin() routine. (_Optional_)
   * - finishSequence: A sequence of button presses to apply during the finish() routine. (_Optional_)
   * - unlisted: A boolian that, if true, will cause the mod not to be reported to the chaos interface (_Optional_)
   * 
   * Notes:
   * Commands altered by formula modifiers should be axes only.
   */
  class FormulaModifier : public Modifier::Registrar<FormulaModifier> {

  private:
    FormulaTypes formula_type;
    double amplitude;
    double period_length;

    std::unordered_map<std::shared_ptr<GameCommand>, int> command_value;
    std::unordered_map<std::shared_ptr<GameCommand>, int> command_offset;

  public:
    FormulaModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }

    void begin();
    void update();
    void finish();
    bool tweak(DeviceEvent& event);
  };
};

