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
#include <list>
#include <plog/Log.h>
#include <mogi/math/systems.h>
#include "remapModifier.hpp"
#include "tomlUtils.hpp"

using namespace Chaos;

const std::string RemapModifier::mod_type = "remap";

RemapModifier::RemapModifier(toml::table& config) {
  Mogi::Math::Random rng;

  checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "signals", "disableSignals", "remap", "random_remap", "unlisted"});

  initialize(config);
  
  Configuration::addToVector<ControllerInput>(config, "signals", signals);

  disable_signals = config["disableSignals"].value_or(false);

#ifndef NDEBUG
  PLOG_VERBOSE << " - signals: ";
  for (auto sig : signals) {
    PLOG_VERBOSE << sig->getName();
  }
  PLOG_VERBOSE << " - disableSignals: " << disable_signals;
#endif
  
  // remap
  if (config.contains("remap")) {
    if (config.contains("random_remap")) {
      throw std::runtime_error("Use either the 'remap' or 'random_remap' keys, not both.");
    }
    const toml::array* remap_list = config.get("remap")->as_array();
    if (! remap_list) {
      throw std::runtime_error("Expect 'remap' to contain an array of remappings.");
    }
    for (auto& elem : *remap_list) {
      const toml::table* remapping = elem.as_table();
      checkValid(*remapping, std::vector<std::string>{
	      "from", "to", "to_neg", "to_min", "invert", "threshold", "sensitivity"}, "remap config");
      if (! remapping) {
	      throw std::runtime_error("Gampad signal remapping must be a table");
      }      
      ControllerSignal from = getSignal(*remapping, "from", true);
      ControllerSignal to = getSignal(*remapping, "to", true);
      ControllerSignal to_neg = getSignal(*remapping, "to_neg", false);

      ControllerSignalType from_type = ControllerInput::get(from)->getType();
      ControllerSignalType to_type = ControllerInput::get(to)->getType();
      ControllerSignalType to_neg_type = ControllerInput::get(to_neg)->getType();

      // Check for unsupported remappings
      if (from_type == ControllerSignalType::DUMMY) {
        PLOG_ERROR << "Cannot map from NONE or NOTHING";
      } else if (from_type != to_type) {
        if (to_type == ControllerSignalType::ACCELEROMETER || to_neg_type == ControllerSignalType::ACCELEROMETER ||
            to_type == ControllerSignalType::GYROSCOPE || to_neg_type == ControllerSignalType::GYROSCOPE ||
            to_type == ControllerSignalType::TOUCHPAD || to_neg_type == ControllerSignalType::TOUCHPAD) {
          PLOG_ERROR << "Cross-type remapping not supported going to the accelerometer, gyroscope, or touchpad.";
        }
        if (to_neg_type != ControllerSignalType::DUMMY && to_neg_type != to_type) {
          PLOG_WARNING << "The 'to' and 'to_neg' signals belong to different classes. Are you sure this is what you want?";
        }
      }
      bool to_min = (*remapping)["to_min"].value_or(false);
      bool invert = (*remapping)["invert"].value_or(false);
      if (invert && (from_type == ControllerSignalType::BUTTON || from_type == ControllerSignalType::HYBRID)) {
        PLOG_WARNING << "Inverting the signal only makes sense for axes. Ignored.";
        invert = false;
      }

      double thresh_proportion = (*remapping)["threshold"].value_or(0);
      short threshold = 1;
      if (thresh_proportion < 0 || thresh_proportion > 1) {
        PLOG_ERROR << "Threshold proportion = " << thresh_proportion << ": must be between 0 and 1";
      } else {
        threshold = (int) (thresh_proportion * JOYSTICK_MAX);
      }

      double sensitivity = (*remapping)["sensitivity"].value_or(1);
      if (sensitivity == 0) {
        PLOG_ERROR << "The sensitivity cannot be 0. Using 1 instead.";
        sensitivity = 1;
      }
      remaps.insert({from, {to, to_neg, to_min, invert, threshold, sensitivity}});
    }
  }

  // Note: Random remapping will break if axes and buttons are included in the same list.
  // Currently we don't check for this.
  if (config.contains("randomRemap")) {
    const toml::array* remap_list = config.get("randomRemap")-> as_array();
    if (! remap_list || !remap_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error("randomRemap must be an array of strings");
    }
    std::vector<std::shared_ptr<ControllerInput>> buttons;
    for (auto& elem : *remap_list) {
      std::optional<std::string> signame = elem.value<std::string>();
      assert(signame);
      std::shared_ptr<ControllerInput> sig = ControllerInput::get(*signame);
      if (! sig) {
	      throw std::runtime_error("signal for random remap '" + *signame + "' not defined");
      }
      buttons.push_back(sig);
      remaps.insert({sig->getSignal(), {ControllerSignal::NONE, ControllerSignal::NONE, false, false, 0, 1}});
    }

    for (auto& r : remaps) {
      int index = floor(rng.uniform(0, buttons.size()-0.01));
      auto it = buttons.begin();
      std::advance(it, index);
      r.second.to_console =  (*it)->getSignal();
      buttons.erase(it);
    }
  }
}

void RemapModifier::begin() {
  apply();
  sendBeginSequence();
}


// Apply our remaps to the main remap table. This routine is called both in begin() and whenever
// another mod is removed from the active list in order to make sure that we can remove our remaps
// without trashing any that may be applied by a mod later in the queue.
void RemapModifier::apply() {
  // SignalRemap
  for (const auto& [from, remap] : remaps) {
    ControllerInput::get(from)->setCascadingRemap(remap);
  }
}

// Undo the remapping
void RemapModifier::finish() {
  for (const auto& [from, remap] : remaps) {
    ControllerInput::get(from)->setRemap(ControllerSignal::NONE);
  }
  sendFinishSequence();
}

