/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributors.
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
#include <algorithm>
#include <cmath>
#include <vector>
#include "FormulaModifier.hpp"
#include "ControllerInput.hpp"
#include "TOMLUtils.hpp"
#include "config.hpp"
#include <random.hpp>

using namespace Chaos;

namespace {
struct Interval {
  double min;
  double max;
};

Interval parseInterval(const toml::table& config, const std::string& key) {
  const toml::array* values = config.get(key)->as_array();
  if (!values) {
    throw std::runtime_error("'" + key + "' must be an array with one or two numbers");
  }
  if (values->size() != 1 && values->size() != 2) {
    throw std::runtime_error("'" + key + "' must contain one or two numbers");
  }

  std::vector<double> parsed(values->size());
  size_t parsed_idx = 0;
  for (const auto& node : *values) {
    std::optional<double> val = node.value<double>();
    if (!val) {
      throw std::runtime_error("'" + key + "' must contain only numbers");
    }
    parsed[parsed_idx++] = *val;
  }

  if (parsed.size() == 1) {
    return {parsed[0], parsed[0]};
  }

  return {std::min(parsed[0], parsed[1]), std::max(parsed[0], parsed[1])};
}

double chooseFromInterval(Random& rng, const Interval& interval) {
  if (interval.min == interval.max) {
    return interval.min;
  }
  return rng.uniform(interval.min, interval.max);
}
}

const std::string FormulaModifier::mod_type = "formula";

  FormulaModifier::FormulaModifier(toml::table& config, EngineInterface* e) {

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "applies_to", "begin_sequence", "finish_sequence",
      "while", "while_operation", "formula_type", "amplitude", "period_length",
      "range", "direction", "unlisted"});
  initialize(config, e);

  if (commands.empty()) {
    throw std::runtime_error("No commands defined in applies_to");
  }

  std::optional<std::string> formula = config["formula_type"].value<std::string>();
  if (!formula) {
    throw std::runtime_error("Missing required formula_type key");
  }
  if (*formula == "circle") {
    formula_type = FormulaTypes::CIRCLE;
  } else if (*formula == "eight_curve") {
    formula_type = FormulaTypes::EIGHT_CURVE;
  } else if (*formula == "janky") {
    formula_type = FormulaTypes::JANKY;
  } else if (*formula == "random_offset") {
    formula_type = FormulaTypes::RANDOM_OFFSET;
  } else {
    throw std::runtime_error("Unrecognized formula type: " + *formula);
  }

  if (formula_type != FormulaTypes::RANDOM_OFFSET && (config.contains("range") || config.contains("direction"))) {
    throw std::runtime_error("'range' and 'direction' can only be used with formula_type = random_offset");
  }

  if (config.contains("range")) {
    const Interval interval = parseInterval(config, "range");
    range_min = interval.min;
    range_max = interval.max;
  } else {
    range_min = JOYSTICK_MIN;
    range_max = JOYSTICK_MAX;
  }

  has_direction = config.contains("direction");
  if (has_direction) {
    const Interval interval = parseInterval(config, "direction");
    direction_min = interval.min;
    direction_max = interval.max;
  } else {
    direction_min = 0.0;
    direction_max = 0.0;
  }

  // User sets amplitude as the proportion of JOYSTICK_MAX. We store it as the actual
  // size of the input signal.
  amplitude = config["amplitude"].value_or(1.0);
  if (amplitude < 0.0 || amplitude > 1.0) {
    PLOG_WARNING << "Amplitude must be a proportion between 0 and 1. Setting to 0.5";
    amplitude = 0.5;
  }
  amplitude *= JOYSTICK_MAX;

  period_length = config["period_length"].value_or(1.0);
  if (period_length <= 0.0) {
    PLOG_WARNING << "Period must be a positive number. Setting to 1 second.";
    period_length = 1.0;
  }
  period_length = 2.0 * M_PI / period_length;
}

void FormulaModifier::begin() {
  command_value.clear();
  for (auto& cmd : commands) {
    command_value[cmd] = cmd->getState(true);
  }
  command_offset.clear();
  for (auto& cmd : commands) {
    command_offset[cmd] = 0;
  }

  command_fixed_offset.clear();
  if (formula_type == FormulaTypes::RANDOM_OFFSET) {
    Random rng;
    const double selected_offset = chooseFromInterval(rng, {range_min, range_max});
    const double selected_direction_degrees = has_direction
        ? chooseFromInterval(rng, {direction_min, direction_max})
        : 0.0;
    const double selected_direction_radians = selected_direction_degrees * M_PI / 180.0;

    for (size_t i = 0; i < commands.size(); ++i) {
      const auto& cmd = commands[i];
      int offset = static_cast<int>(std::lround(selected_offset));

      if (has_direction) {
        const bool first_in_pair = ((i % 2) == 0);
        const double component = first_in_pair
            ? selected_offset * std::cos(selected_direction_radians)
            : selected_offset * std::sin(selected_direction_radians);
        offset = static_cast<int>(std::lround(component));
      }
      command_fixed_offset[cmd] = offset;
    }
  }
}

void FormulaModifier::update() {
  DeviceEvent event;
  event.type = TYPE_AXIS;

  double t = timer.runningTime() * period_length;
  int i = 0;

  for (auto& cmd : commands) {
    
    if (inCondition()) {
      if (formula_type == FormulaTypes::RANDOM_OFFSET) {
        command_offset[cmd] = command_fixed_offset[cmd];
      } else {
        const bool first_in_pair = ((i % 2) == 0);
        switch (formula_type) {
          case FormulaTypes::CIRCLE:
            // Apply paired components in-order so [A, B, A, B, ...] maps consistently.
            command_offset[cmd] = first_in_pair
                ? static_cast<int>(amplitude * std::sin(t))
                : static_cast<int>(amplitude * std::cos(t));
            break;
          case FormulaTypes::EIGHT_CURVE:
            command_offset[cmd] = static_cast<int>(amplitude * std::sin(4.0*(i+1)*(t+1.6)));
            break;
          case FormulaTypes::JANKY:
            command_offset[cmd] = static_cast<int>(amplitude * (std::cos(t+4.0*i) + std::cos(2.0*t)/2.0) *
                                         std::sin((t+4.0*i)/5.0)/2.0);
            break;
          case FormulaTypes::RANDOM_OFFSET:
            break;
        }
      }
      event.id = cmd->getInput()->getID();
      event.value = fmin(fmax(command_value[cmd] + command_offset[cmd], JOYSTICK_MIN), JOYSTICK_MAX);
      engine->fakePipelinedEvent(event, getptr());
      PLOG_DEBUG << cmd->getName() << " orig value = " << command_value[cmd] << " + offset " << command_offset[cmd];
      i++;
    } else {
      command_offset[cmd] = 0;
    }
  }
}

void FormulaModifier::finish() {
  DeviceEvent event;
  // Restore axes to their non-skewed positions
  for (auto& cmd : commands) {
    event.id = cmd->getInput()->getID();
    event.type = cmd->getInput()->getButtonType();
    event.value = command_value[cmd];
    engine->fakePipelinedEvent(event, getptr());
  }

}

bool FormulaModifier::tweak(DeviceEvent& event) {
  for (auto& cmd : commands) {
    if (engine->eventMatches(event, cmd)) {
      command_value[cmd] = event.value;
      event.value = fmin(fmax(event.value + command_offset[cmd], JOYSTICK_MIN), JOYSTICK_MAX);
    }
  }
  return true;
}
