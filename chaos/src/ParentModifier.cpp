/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS file at
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
#include "ParentModifier.hpp"
#include "TOMLUtils.hpp"
#include <algorithm>
#include <random.hpp>
#include <cmath>
#include <list>
#include <unordered_set>
#include <vector>

using namespace Chaos;

const std::string ParentModifier::mod_type = "parent";

ParentModifier::ParentModifier(toml::table& config, EngineInterface* e) {
  PLOG_VERBOSE << "constructing parent modifier";
  assert(config.contains("name"));
  assert(config.contains("type"));

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "begin_sequence", "finish_sequence",
      "children", "random", "value", "unlisted"});
  initialize(config, e);
 
  bool random_selection = config["random"].value_or(false);
  setAllowAsChild(!random_selection);

  if (random_selection) {
    num_randos = config["value"].value_or(1);
    if (num_randos < 1) {
      throw std::runtime_error("For random modifiers 'value' must be greater than 0");
    }
  }

  if (config.contains("children")) {
    const toml::array* cmd_list = config.get("children")->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error("'children' must be an array of modifier names");
   	}
	
    for (auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      assert(cmd);
      // check that the string matches the name of a previously defined object
   	  std::shared_ptr<Modifier> item =  e->getModifier(*cmd);
      if (item) {
        fixed_children.push_back(item);
        PLOG_VERBOSE << "Added '" + *cmd + "' to fixed_children.";
      } else {
        throw std::runtime_error("Unrecognized command: " + *cmd + " in children");
     	}
    }
  }
  if (!random_selection && fixed_children.empty()) {
    throw std::runtime_error("Parent modifier must specify children unless random selection is enabled.");
  }
}

void ParentModifier::buildRandomList() {
  Random rng;
  const auto& all_mods = engine->getModifierMap();
  std::unordered_set<std::string> used_names;

  for (const auto& mod : engine->getActiveMods()) {
    if (mod) {
      used_names.insert(mod->getName());
    }
  }
  for (const auto& mod : fixed_children) {
    if (mod) {
      used_names.insert(mod->getName());
    }
  }
  for (const auto& mod : random_children) {
    if (mod) {
      used_names.insert(mod->getName());
    }
  }

  std::vector<std::pair<std::string, std::shared_ptr<Modifier>>> eligible;
  eligible.reserve(all_mods.size());
  for (const auto& [m_name, mod] : all_mods) {
    if (!mod) {
      continue;
    }
    // Exclude parent mods that do random child selection. (This excludes self.)
    if (mod->getModType() == mod_type && !mod->allowAsChild()) {
      continue;
    }
    if (used_names.find(m_name) != used_names.end()) {
      continue;
    }
    eligible.emplace_back(m_name, mod);
  }

  if (eligible.empty()) {
    PLOG_WARNING << "No eligible modifiers available for random children in " << getName();
    return;
  }

  const size_t target = std::min(static_cast<size_t>(num_randos), eligible.size());
  if (target < static_cast<size_t>(num_randos)) {
    PLOG_WARNING << "Requested " << num_randos << " random child modifiers for " << getName()
                 << " but only " << eligible.size() << " are eligible";
  }

  while (random_children.size() < target) {
    size_t selection = static_cast<size_t>(std::floor(rng.uniform(0, eligible.size() - 0.01)));
    auto [name, mod] = eligible[selection];
    PLOG_INFO << "Selected " << name << " as child mod";
    random_children.push_back(mod);
    eligible.erase(eligible.begin() + selection);
  }
}

void ParentModifier::begin() {
  assert(random_children.empty());
  // Initialize any pre-determined modifiers
  for (auto& mod : fixed_children) {
    mod->setParentModifier(getptr());
    mod->_begin();
  }
  if (num_randos > 0) {
    // Get a new random set of modifiers
    buildRandomList();
    // Initialize what we've selected
    for (auto& mod : random_children) {
      mod->setParentModifier(getptr());
      mod->_begin();
    }
  }
}

void ParentModifier::update() {
  for (auto& mod : fixed_children) {
    mod->_update(engine->isPaused());
  }

  for (auto& mod : random_children) {
    mod->_update(engine->isPaused());
  }
}

void ParentModifier::finish() {
  for (auto& mod : fixed_children) {
    mod->_finish();
  }

  for (auto& mod : random_children) {
    mod->_finish();
  }
  random_children.clear();
}

bool ParentModifier::tweak(DeviceEvent& event) {
  bool rval;
  for (auto& mod : fixed_children) {
    rval = mod->_tweak(event);
    if (! rval) {
      return false;
    }
  }
  for (auto& mod : random_children) {
    rval = mod->_tweak(event);
    if (! rval) {
      return false;
    }
  }
  return true;
}
