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
#include "MenuModifier.hpp"
#include "TOMLUtils.hpp"
#include "GameMenu.hpp"
#include "Game.hpp"

using namespace Chaos;

const std::string MenuModifier::mod_type = "menu";

MenuModifier::MenuModifier(toml::table& config, std::shared_ptr<EngineInterface> e) {
  
  TOMLUtils::checkValid(config, std::vector<std::string>{"name", "description", "type", "groups",
             "menu_items", "reset_on_finish", "beginSequence",
             "finishSequence","unlisted"});

  initialize(config, e);

  std::optional<std::string> item = config["item"].value<std::string>();
  if (!config.contains("menu_items")) {
    throw std::runtime_error("Missing menu_items for menu modifier");
  }
  const toml::array* menu_list = config.get("menu_items")->as_array();
  if (! menu_list) {
    throw std::runtime_error("menu_items must be an array of inline tables");
  }
  for (auto& elem : *menu_list) {
    const toml::table* m = elem.as_table();
    TOMLUtils::checkValid(*m, std::vector<std::string>{"entry", "value"},"menu entry");
    if (! m->contains("entry")) {
      throw std::runtime_error("Each table within a menu_item array must contain an 'entry' key");
    }
    std::optional<std::string> item_name = m->get("entry")->value<std::string>();
    std::shared_ptr<MenuItem> item = engine->getMenuItem(*item_name);
    if (! item) {
      throw std::runtime_error("Menu item '" + *item_name + "' not defined");
    }
    int val = (*m)["value"].value_or(0);
    menu_items[item] = val;
    PLOG_VERBOSE << "   entry = " << *item_name << ", value = " << val;
  }

  reset_on_finish = config["reset_on_finish"].value_or(true);
}

void MenuModifier::begin() {
  // Perform any non-menu-related actions
  sendBeginSequence();

  // Execute all menu actions
  for (auto const& [item, val] : menu_items) {
    engine->setMenuState(item, val);
  }
}

void MenuModifier::finish() {
  sendFinishSequence();

  if (reset_on_finish) {
    for (auto const& [item, val] : menu_items) {
      engine->restoreMenuState(item);
    }
  }
}

bool MenuModifier::tweak(DeviceEvent* event) {
  // block all signals if we're busy sending a sequence
  return !in_sequence;
}

