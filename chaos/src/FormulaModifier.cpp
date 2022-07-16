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
#include <math.h>
#include "FormulaModifier.hpp"
#include "ControllerInput.hpp"
#include "TOMLUtils.hpp"
#include "config.hpp"

using namespace Chaos;


const std::string FormulaModifier::mod_type = "formula";

  FormulaModifier::FormulaModifier(toml::table& config, EngineInterface* e) {

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "applies_to", "begin_sequence", "finish_sequence",
      "condition", "formula_type", "amplitude", "period_length", "unlisted"});
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
  } else {
    throw std::runtime_error("Unrecognized formula type: " + *formula);
  }

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
    command_value[cmd] = cmd->getState();
  }
  command_offset.clear();
  for (auto& cmd : commands) {
    command_offset[cmd] = 0;
  }
}

void FormulaModifier::update() {
  DeviceEvent event;
  event.type = TYPE_AXIS;

  bool apply_formula = inCondition();

  double t = timer.runningTime() * period_length;
  int i = 0;

  for (auto& cmd : commands) {
    
    if (apply_formula) {
      switch (formula_type) {
        case FormulaTypes::CIRCLE:
          if (i % 2) {
            command_offset[cmd] = (int) (amplitude * std::sin(t));
          } else {
            command_offset[cmd] = (int) (amplitude * std::cos(t));
          }
          break;
        case FormulaTypes::EIGHT_CURVE:
          if (i % 2) {
            command_offset[cmd] = (int) (amplitude * std::sin(t)*std::cos(t));
          } else {
            command_offset[cmd] = (int) (amplitude * std::sin(t));
          }
          break;
        case FormulaTypes::JANKY:
          command_offset[cmd] = (int) (amplitude * (std::cos(t+4.0*i) + std::cos(2.0*t)/2.0) * std::sin(t+4.0*i)*0.2/2.0);
      }
      event.id = cmd->getInput()->getID();
      event.value = fmin(fmax(command_value[cmd] + command_offset[cmd], JOYSTICK_MIN), JOYSTICK_MAX);
      engine->fakePipelinedEvent(event, getptr());
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