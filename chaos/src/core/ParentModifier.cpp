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
#include "ParentModifier.hpp"
#include "TOMLUtils.hpp"
#include <algorithm>
#include <random.hpp>
#include <cmath>
#include <list>
#include <unordered_set>
#include <vector>

using namespace Chaos;

namespace {
std::vector<std::string> parseStringArray(const toml::table& config, const std::string& key) {
  const toml::array* values = config.get(key)->as_array();
  if (!values || !values->is_homogeneous(toml::node_type::string)) {
    throw std::runtime_error("'" + key + "' must be an array of strings");
  }

  std::vector<std::string> parsed;
  parsed.reserve(values->size());
  for (const auto& elem : *values) {
    std::optional<std::string> item = elem.value<std::string>();
    if (item) {
      parsed.push_back(*item);
    }
  }
  return parsed;
}
}

const std::string ParentModifier::mod_type = "parent";

ParentModifier::ParentModifier(toml::table& config, EngineInterface* e) {
  PLOG_VERBOSE << "constructing parent modifier";
  assert(config.contains("name"));
  assert(config.contains("type"));

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "begin_sequence", "finish_sequence",
      "children", "random", "value", "select_from", "select_groups", "unlisted"});
  initialize(config, e);
 
  bool random_selection = config["random"].value_or(false);
  setAllowAsChild(!random_selection);

  if (random_selection) {
    num_randos = config["value"].value_or(1);
    if (num_randos < 0) {
      random_num_randos = true;
      num_randos = static_cast<short>(-num_randos);
      if (num_randos == 1) {
        PLOG_WARNING << "value = -1 for random parent modifier '" << getName()
                     << "': treating as 1";
      }
    }
    if (num_randos < 1) {
      throw std::runtime_error("For random modifiers 'value' must be greater than 0");
    }
  }

  if (config.contains("select_from") && config.contains("select_groups")) {
    throw std::runtime_error("Use either 'select_from' or 'select_groups', not both.");
  }

  if (!random_selection && (config.contains("select_from") || config.contains("select_groups"))) {
    throw std::runtime_error("'select_from' and 'select_groups' can only be used when 'random' is true");
  }

  if (config.contains("select_from")) {
    const auto select_from = parseStringArray(config, "select_from");
    std::unordered_set<std::string> seen_names;
    for (const auto& mod_name : select_from) {
      std::shared_ptr<Modifier> item = e->getModifier(mod_name);
      if (!item) {
        throw std::runtime_error("Unrecognized command: " + mod_name + " in select_from");
      }
      if (item->getModType() == mod_type) {
        throw std::runtime_error("Parent modifier '" + mod_name + "' cannot be listed in select_from");
      }
      if (seen_names.insert(mod_name).second) {
        random_select_from.push_back(item);
      }
    }
  }

  if (config.contains("select_groups")) {
    const auto select_groups = parseStringArray(config, "select_groups");
    for (const auto& group_name : select_groups) {
      random_select_groups.insert(group_name);
    }
  }

  if (config.contains("children")) {
    const auto child_list = parseStringArray(config, "children");
    for (const auto& cmd : child_list) {
      // check that the string matches the name of a previously defined object
      std::shared_ptr<Modifier> item = e->getModifier(cmd);
      if (item) {
        fixed_children.push_back(item);
        PLOG_VERBOSE << "Added '" + cmd + "' to fixed_children.";
      } else {
        throw std::runtime_error("Unrecognized command: " + cmd + " in children");
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
  if (!random_select_from.empty()) {
    eligible.reserve(random_select_from.size());
    for (const auto& mod : random_select_from) {
      if (!mod) {
        continue;
      }
      eligible.emplace_back(mod->getName(), mod);
    }
  } else {
    eligible.reserve(all_mods.size());
    for (const auto& [m_name, mod] : all_mods) {
      if (!mod) {
        continue;
      }

      if (!random_select_groups.empty()) {
        // Group-limited random selection never includes parent modifiers.
        if (mod->getModType() == mod_type) {
          continue;
        }
        bool in_group = false;
        const Json::Value groups = mod->getGroups();
        for (const auto& group : groups) {
          if (group.isString() &&
              random_select_groups.find(group.asString()) != random_select_groups.end()) {
            in_group = true;
            break;
          }
        }
        if (!in_group) {
          continue;
        }
      } else {
        // Exclude parent mods that do random child selection. (This excludes self.)
        if (mod->getModType() == mod_type && !mod->allowAsChild()) {
          continue;
        }
      }
      eligible.emplace_back(m_name, mod);
    }
  }

  std::vector<std::pair<std::string, std::shared_ptr<Modifier>>> filtered;
  filtered.reserve(eligible.size());
  for (const auto& [name, mod] : eligible) {
    if (used_names.find(name) != used_names.end()) {
      continue;
    }
    filtered.emplace_back(name, mod);
  }
  eligible.swap(filtered);

  if (eligible.empty()) {
    PLOG_WARNING << "No eligible modifiers available for random children in " << getName();
    return;
  }

  size_t target = static_cast<size_t>(num_randos);
  if (random_num_randos) {
    target = static_cast<size_t>(
        std::floor(rng.uniform(1.0, static_cast<double>(num_randos) + 0.01)));
  }
  const size_t requested = static_cast<size_t>(num_randos);
  target = std::min(target, eligible.size());
  if (requested > eligible.size()) {
    PLOG_WARNING << "Requested up to " << requested << " random child modifiers for " << getName()
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

bool ParentModifier::remap(DeviceEvent& event) {
  bool rval;
  for (auto& mod : fixed_children) {
    rval = mod->remap(event);
    if (!rval) {
      return false;
    }
  }
  for (auto& mod : random_children) {
    rval = mod->remap(event);
    if (!rval) {
      return false;
    }
  }
  return true;
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
