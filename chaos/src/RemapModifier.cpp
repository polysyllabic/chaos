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
#include <random.hpp>

#include "RemapModifier.hpp"
#include <EngineInterface.hpp>
#include <TOMLUtils.hpp>

using namespace Chaos;

const std::string RemapModifier::mod_type = "remap";

RemapModifier::RemapModifier(toml::table& config, EngineInterface* e) {

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "signals", "disableSignals", "remap", "random_remap", "unlisted"});

  initialize(config, e);
  
  engine->addControllerInputs(config, "signals", signals);

  disable_signals = config["disableSignals"].value_or(false);

  // remap
  if (config.contains("remap")) {
    if (config.contains("random_remap")) {
      throw std::runtime_error("Use either the 'remap' or 'random_remap' keys, not both.");
    }
    random = false;
    const toml::array* remap_list = config.get("remap")->as_array();
    if (! remap_list) {
      throw std::runtime_error("Expect 'remap' to contain an array of remappings.");
    }
    PLOG_VERBOSE << "Processing remap list";
    for (auto& elem : *remap_list) {
      const toml::table* remapping = elem.as_table();
      if (! remapping) {
	      throw std::runtime_error("Remapping instructions must be formatted as a table");
      }
      TOMLUtils::checkValid(*remapping, std::vector<std::string>{"from", "to", "to_neg", "to_min", "invert",
                 "threshold", "sensitivity"}, "remap config");
      
      std::shared_ptr<ControllerInput> from = lookupInput(*remapping, "from", true);

      std::shared_ptr<ControllerInput> to = lookupInput(*remapping, "to", true);

      std::shared_ptr<ControllerInput> to_neg = lookupInput(*remapping, "to_neg", false);
      // Check for unsupported remappings
      if (from->getType() == ControllerSignalType::DUMMY) {
        throw std::runtime_error("Cannot map from NONE or NOTHING");
      } else if (from->getType() != to->getType()) {
        if ((to->getType() == ControllerSignalType::ACCELEROMETER ||
            to->getType() == ControllerSignalType::GYROSCOPE || 
            to->getType() == ControllerSignalType::TOUCHPAD) || (to_neg && (
            to_neg->getType() == ControllerSignalType::ACCELEROMETER || 
            to_neg->getType() == ControllerSignalType::GYROSCOPE || 
            to_neg->getType() == ControllerSignalType::TOUCHPAD
            ))) {
          throw std::runtime_error("Cross-type remapping not supported going to the accelerometer, gyroscope, or touchpad.");
        }
        if (to_neg && to_neg->getType() != ControllerSignalType::DUMMY && to_neg->getType() != to->getType()) {
          PLOG_WARNING << "The 'to' and 'to_neg' signals belong to different classes. Are you sure this is what you want?";
        }
      }
      bool to_min = (*remapping)["to_min"].value_or(false);
      bool invert = (*remapping)["invert"].value_or(false);
      if (invert && (from->getType() == ControllerSignalType::BUTTON || from->getType() == ControllerSignalType::HYBRID)) {
        PLOG_WARNING << "Inverting the signal only makes sense for axes. Ignored.";
        invert = false;
      }

      double thresh_proportion = (*remapping)["threshold"].value_or(0);
      short threshold = 1;
      if (thresh_proportion < 0 || thresh_proportion > 1) {
        PLOG_WARNING << "Threshold proportion = " << thresh_proportion << ": must be between 0 and 1";
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

  // Note: Random remapping mmay break if axes and buttons are included in the same list.
  // Currently we don't check for this.
  if (config.contains("randomRemap")) {
    random = true;
    const toml::array* remap_list = config.get("randomRemap")-> as_array();
    if (! remap_list || !remap_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error("randomRemap must be an array of strings");
    }
    for (auto& elem : *remap_list) {
      std::optional<std::string> signame = elem.value<std::string>();
      // the is_homogenous test above should ensure that signame always has a value
      assert(signame);
      std::shared_ptr<ControllerInput> sig = engine->getInput(*signame);
      if (! sig) {
	      throw std::runtime_error("Controller input for random remap '" + *signame + "' is not defined");
      }
      remaps.insert({sig, {nullptr, nullptr, false, false, 0, 1}});
    }
  }
}

std::shared_ptr<ControllerInput> RemapModifier::lookupInput(const toml::table& config, const std::string& key, bool required) {
  std::optional<std::string> inp = config[key].value<std::string>();
  std::shared_ptr<ControllerInput> rval = nullptr;
  if (inp) {
    rval = engine->getInput(*inp);
    if (!rval) {
      throw std::runtime_error(*inp + " is not a defined signal");
    }
  } else if (required) {
    throw std::runtime_error("Missing Required '" + key + "' key in remap table");
  } 
  return rval;
}

void RemapModifier::begin() {
  if (random) {
    // Generate a new remapping
    Random rng;
    std::vector<std::shared_ptr<ControllerInput>> buttons;
    // Collect a list of the signals that we're going to remap
    for (auto& [key, value] : remaps) {
      buttons.push_back(key);
    }
    // Iterate through the list of signals we're remapping and assign a random signal to it
    for (auto& [key, value] : remaps) {
      int index = floor(rng.uniform(0, buttons.size()-0.01));
      auto it = buttons.begin();
      std::advance(it, index);
      value.to_console =  *it;
      buttons.erase(it);
    } 
  }
  _apply();
}

// Apply our remaps to the main remap table. This routine is called both in begin() and whenever
// another mod is removed from the active list in order to make sure that we can remove our remaps
// without trashing any that may be applied by a mod later in the queue.
void RemapModifier::apply() {
  PLOG_DEBUG << "Updating remaps for " << name;
  engine->setCascadingRemap(remaps);
}

// Undo the remapping
void RemapModifier::finish() {
  // When we remove ourselves, we reset the *entire* remap table to nothing. Any remaining remap
  // mods that are active will then reapply themselves when their apply functions are called.
  engine->clearRemaps();
}

