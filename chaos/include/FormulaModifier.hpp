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
   * formula, chosen from a pre-defined set. Currently, we support circles, figure-8s, and a "janky"
   * pattern that produces an irregular up and down pattern.
   * 
   * The modifier sets an amplitude, set by the user as a proportion of JOYSTICK_MAXIMUM (i.e.,
   * approximately half the full range of an axis), and then multiplies that value by the formula
   * 
   * Normally, axes should be passed in matched pairs, and the formula calculations assume that is
   * true. If a single axis is passed, or if the axes are passed in irregular orders, the modifier
   * will still run, but the results may not be what you expect give the name of the formula.
   * 
   * Formulas (all results multiplied by amplitude):
   * 
   * - circle:      axis 1 = sin(t)
   *                axis 2 = cos(t)
   * - eight_curve: axis 1 = sin(t)*cos(t)
   *                axis 2 = sin(t)
   * - janky:       axis 1 = (cos(t) + cos(2t)/2) * sin(t/5)/2
   *                axis 2 = (cos(t+4) + cos(2t)/2) * sin((t+4)/5)/2
   * 
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

