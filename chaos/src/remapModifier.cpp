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
#include <plog/Log.h>

#include "remapModifier.hpp"
#include "tomlReader.hpp"

using namespace Chaos;

const std::string RemapModifier::name = "remap";

RemapModifier::RemapModifier(const toml::table& config) {
  initialize(config);
  
  TOMLReader::addToVector<GamepadInput>(config, "signals", signals);
  
  disableOnStart = config["disableOnStart"].value_or(false);

  // remap
  if (config.contains("remap")) {
    if (config.contains("random_remap")) {
      throw std::runtime_error("Use either the 'remap' or 'random_remap' keys, not both.");
    }
    const toml::array* remap_list = config.get("remap")->as_array();
    if (! remap_list) {
      throw std::runtime_error("Expect 'remap' to contain an array of remappings.");
    }
    for (auto& elem : * remap_list) {
      const toml::table* remapping = elem.as_table();
      if (! remapping) {
	throw std::runtime_error("Gampad signal remapping must be a table");
      }
      GPInput from = TOMLReader::getSignal(*remapping, "from", true);
      GPInput to = TOMLReader::getSignal(*remapping, "to", true);
      GPInput to_neg = TOMLReader::getSignal(*remapping, "to_neg", false);
      bool to_min = config["to_min"].value_or(false);
      bool invert = config["invert"].value_or(false);
      int threshold = config["threshold"].value_or(0);
      int sensitivity = config["sensitivity"].value_or(1);
      remaps.emplace_back(from, to, to_neg, to_min, invert, threshold, sensitivity);
    }
  }

  if (config.contains("random_remap")) {
    
  }
}

void RemapModifier::begin() {

  apply();
  
  // Send a 0 to the inputs in the list
  if (disableOnStart) {
    DeviceEvent event = {};
    for (auto& s : signals) {
      event.id = s->getButtonType();
      event.type = s->getID((ButtonType) event.id);
      controller->applyEvent(event);
    }
  }
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
}

