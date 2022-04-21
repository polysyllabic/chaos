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
#include <list>
#include <plog/Log.h>
#include <mogi/math/systems.h>
#include "remapModifier.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

const std::string RemapModifier::name = "remap";

RemapModifier::RemapModifier(toml::table& config) {
  Mogi::Math::Random rng;

  TOMLReader::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "signals", "disableSignals", "remap", "randomRemap"});

  initialize(config);
  
  TOMLReader::addToVector<GamepadInput>(config, "signals", signals);

  disable_signals = config["disableSignals"].value_or(false);

#ifndef NDEBUG
  PLOG_DEBUG << " - signals: ";
  for (auto sig : signals) {
    PLOG_DEBUG << GamepadInput::getName(sig->getSignal()) << " ";
  }
  PLOG_DEBUG << "\n - disableSignals: " << disable_signals << std::endl;
#endif
  
  // remap
  if (config.contains("remap")) {
    if (config.contains("randomRemap")) {
      throw std::runtime_error("Use either the 'remap' or 'randomRemap' keys, not both.");
    }
    const toml::array* remap_list = config.get("remap")->as_array();
    if (! remap_list) {
      throw std::runtime_error("Expect 'remap' to contain an array of remappings.");
    }
    for (auto& elem : *remap_list) {
      const toml::table* remapping = elem.as_table();
      TOMLReader::checkValid(*remapping, std::vector<std::string>{
	      "from", "to", "no_neg", "to_min", "invert", "threshold", "sensitivity"});
      if (! remapping) {
	      throw std::runtime_error("Gampad signal remapping must be a table");
      }      
      GPInput from = TOMLReader::getSignal(*remapping, "from", true);
      GPInput to = TOMLReader::getSignal(*remapping, "to", true);
      GPInput to_neg = TOMLReader::getSignal(*remapping, "to_neg", false);
      bool to_min = (*remapping)["to_min"].value_or(false);
      bool invert = (*remapping)["invert"].value_or(false);
      int threshold = (*remapping)["threshold"].value_or(0);
      int sensitivity = (*remapping)["sensitivity"].value_or(1);
      remaps.emplace_back(from, to, to_neg, to_min, invert, threshold, sensitivity);
#ifndef NDEBUG
      PLOG_DEBUG << " - remap: from " << GamepadInput::getName(from) << " to " << GamepadInput::getName(to)
		 << " to_neg " << GamepadInput::getName(to_neg) << " to_min: " << to_min << " invert: "
		 << invert << " threshold: " << threshold << " sensitivity: " << sensitivity << std::endl;
#endif
    }
  }

  // Note: Random remapping will break if axes and buttons are included in the same list.
  // Currently we don't check for this.
  if (config.contains("randomRemap")) {
    const toml::array* remap_list = config.get("randomRemap")-> as_array();
    if (! remap_list || !remap_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error("randomRemap must be an array of strings");
    }
    std::vector<std::shared_ptr<GamepadInput>> buttons;
    for (auto& elem : *remap_list) {
      std::optional<std::string> signame = elem.value<std::string>();
      assert(signame);
      std::shared_ptr<GamepadInput> sig = GamepadInput::get(*signame);
      if (! sig) {
	throw std::runtime_error("signal for random remap '" + *signame + "' not defined");
      }
      buttons.push_back(sig);
      remaps.emplace_back(sig->getSignal(), GPInput::NONE, GPInput::NONE, false, false, 0, 1);
    }
#ifndef NDEBUG
    PLOG_DEBUG << " - randomRemap:\n";
#endif    
    for (auto& r : remaps) {
      int index = floor(rng.uniform(0, buttons.size()-0.01));
      auto it = buttons.begin();
      std::advance(it, index);
      r.to_console =  (*it)->getSignal();
      buttons.erase(it);
#ifndef NDEBUG
      PLOG_DEBUG << "    + from " << GamepadInput::getName(r.from_controller) << " to "
		 << GamepadInput::getName(r.to_console) << std::endl;
#endif
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
  for (auto& remap : remaps) {
    GamepadInput::get(remap.from_controller)->setCascadingRemap(remap);
  }
}

// Undo the remapping
void RemapModifier::finish() {
  for (auto& r : remaps) {
    GamepadInput::get(r.from_controller)->setRemap(GPInput::NONE);
  }
  sendFinishSequence();
}

