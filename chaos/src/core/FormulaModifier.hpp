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
#include <string>
#include <unordered_map>
#include <toml++/toml.h>

#include "Modifier.hpp"
#include "Game.hpp"

namespace Chaos {

  enum class FormulaTypes {
    CIRCLE, EIGHT_CURVE, JANKY, RANDOM_OFFSET
  };

  /**
   * \brief Modifier that alters the signal through a formula.
   *
   * Formula modifiers apply an offset to incoming signals according to a specified, time-dependent
   * formula, chosen from a pre-defined set. Currently, we support circles, figure-8s, a "janky"
   * pattern that produces an irregular up and down pattern, and a fixed random-offset mode.
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
   * - eight_curve: axis 1 = sin(4(t+1.6))
   *                axis 2 = sin(8(t+1.6))
   * - janky:       axis 1 = (cos(t) + cos(2t)/2) * sin(t/5)/2
   *                axis 2 = (cos(t+4) + cos(2t)/2) * sin((t+4)/5)/2
   * - random_offset: axis 1..N use fixed offsets selected once at begin()
   * 
   * Commands altered by formula modifiers should be axes only.
   */
  class FormulaModifier : public Modifier::Registrar<FormulaModifier> {

  private:
    FormulaTypes formula_type;
    double amplitude;
    double period_length;
    bool has_direction;
    double range_min;
    double range_max;
    double direction_min;
    double direction_max;
    bool condition_active_last{false};

    std::unordered_map<std::shared_ptr<GameCommand>, int> command_value;
    std::unordered_map<std::shared_ptr<GameCommand>, int> command_offset;
    std::unordered_map<std::shared_ptr<GameCommand>, int> command_fixed_offset;

  public:
    /**
     * \brief Construct a formula modifier from TOML configuration.
     *
     * \param config Modifier configuration table.
     * \param e Engine interface pointer.
     */
    FormulaModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;

    /**
     * \brief Return this modifier's registered factory type name.
     */
    const std::string& getModType() { return mod_type; }

    /**
     * \brief Initialize formula-specific runtime state.
     */
    void begin();

    /**
     * \brief Update formula output and cached offsets.
     */
    void update();

    /**
     * \brief Restore any state changed by this modifier on shutdown.
     */
    void finish();

    /**
     * \brief Apply the configured formula to matching events.
     *
     * \param event Incoming event to transform.
     * \return true to forward the transformed event.
     */
    bool tweak(DeviceEvent& event);
  };
};
